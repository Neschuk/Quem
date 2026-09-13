#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h> // NUEVO: Para poder usar tolower()

#include "../include/render_load.h"
#include "../include/icons.h"

// --- MACROS ANSI ---
#define CLEAR_SCREEN      "\033[2J\033[H"
#define HIDE_CURSOR       "\033[?25l"
#define SHOW_CURSOR       "\033[?25h"
#define MOVE_CURSOR(y, x) printf("\033[%d;%dH", (int)(y), (int)(x))
#define RESET_COLOR       "\033[0m"
#define BORDER_COLOR      "\033[32m"
#define PROGRESS_COLOR    "\033[32m"

static int first_render = 1;
static int last_cols = 0;
static int last_rows = 0;
static struct termios orig_termios;
static int orig_fcntl_flags;

int load(DiskImage os_list, RemovableDrive usb_list[], int usb_count, const bool burn_usbs[PATH_MAX], uint64_t total_end){
    
    // 1. CONFIGURAR TECLADO AL INICIO
    if (first_render) {
        tcgetattr(STDIN_FILENO, &orig_termios);
        struct termios new_termios = orig_termios;
        new_termios.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

        orig_fcntl_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, orig_fcntl_flags | O_NONBLOCK);
    }

    // 2. DETECCIÓN NO-BLOQUEANTE DE 'Q'
    char ch;
    if (read(STDIN_FILENO, &ch, 1) > 0) {
        if (ch == 'q' || ch == 'Q') {
            first_render = 1;
            last_cols = 0; last_rows = 0;
            printf(SHOW_CURSOR);
            tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
            fcntl(STDIN_FILENO, F_SETFL, orig_fcntl_flags);
            return -1;
        }
    }

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int cols = w.ws_col;
    int rows = w.ws_row;

    int force_redraw = 0;
    if (cols != last_cols || rows != last_rows) {
        force_redraw = 1;     
        last_cols = cols;     
        last_rows = rows;
    }

    int percentage = 0;
    if (os_list.file_size > 0) {
        percentage = (int)((total_end * 100) / os_list.file_size);
    }
    if (percentage > 100) percentage = 100;

    int target_count = 0;
    for (int i = 0; i < usb_count; i++) {
        if (burn_usbs[i]) target_count++;
    }

    // ==========================================
    // NUEVO: ICONO DINÁMICO (Magia de Detección)
    // ==========================================
    const char **active_os_icon = OSICON_0; // Icono por defecto
    int os_lines = sizeof(OSICON_0) / sizeof(OSICON_0[0]);

    // Convertimos nombre y ruta a minúsculas para buscar palabras clave
    char lname[256] = {0}, lpath[PATH_MAX] = {0};
    strncpy(lname, os_list.name, 255);
    strncpy(lpath, os_list.image_path, PATH_MAX - 1);
    
    for (int i = 0; lname[i]; i++) lname[i] = tolower((unsigned char)lname[i]);
    for (int i = 0; lpath[i]; i++) lpath[i] = tolower((unsigned char)lpath[i]);

    // Asignamos el icono y calculamos sus líneas matemáticas dinámicamente
    if (strstr(lname, "arch") || strstr(lpath, "arch")) { active_os_icon = ARCH; os_lines = sizeof(ARCH)/sizeof(ARCH[0]); }
    else if (strstr(lname, "cachy") || strstr(lpath, "cachy")) { active_os_icon = CACHYOS; os_lines = sizeof(CACHYOS)/sizeof(CACHYOS[0]); }
    else if (strstr(lname, "ubuntu") || strstr(lpath, "ubuntu")) { active_os_icon = UBUNTU; os_lines = sizeof(UBUNTU)/sizeof(UBUNTU[0]); }
    else if (strstr(lname, "debian") || strstr(lpath, "debian")) { active_os_icon = DEBIAN; os_lines = sizeof(DEBIAN)/sizeof(DEBIAN[0]); }
    else if (strstr(lname, "chrome") || strstr(lpath, "chrome")) { active_os_icon = CHROME; os_lines = sizeof(CHROME)/sizeof(CHROME[0]); }
    else if (strstr(lname, "mac") || strstr(lpath, "mac") || strstr(lname, "darwin")) { active_os_icon = MAC; os_lines = sizeof(MAC)/sizeof(MAC[0]); }
    else if (strstr(lname, "mint") || strstr(lpath, "mint")) { active_os_icon = MINT; os_lines = sizeof(MINT)/sizeof(MINT[0]); }
    else if (strstr(lname, "fedora") || strstr(lpath, "fedora")) { active_os_icon = FEDORA; os_lines = sizeof(FEDORA)/sizeof(FEDORA[0]); }
    else if (strstr(lname, "redhat") || strstr(lpath, "redhat") || strstr(lname, "rhel")) { active_os_icon = REDHAT; os_lines = sizeof(REDHAT)/sizeof(REDHAT[0]); }
    else if (strstr(lname, "win") || strstr(lpath, "win")) { active_os_icon = WINDOWS; os_lines = sizeof(WINDOWS)/sizeof(WINDOWS[0]); }

    int usb_lines = sizeof(USBICON_0) / sizeof(USBICON_0[0]);
    
    int max_icon_lines = (os_lines > usb_lines) ? os_lines : usb_lines;
    if (max_icon_lines < 2) max_icon_lines = 2; 

    // GEOMETRÍA
    int box_width = 80; 
    int box_height = 6 + target_count + max_icon_lines + 2; 
    int minimal_mode = (cols < box_width || rows < box_height) ? 1 : 0;
    int start_y = (rows - box_height) / 2;
    int start_x = (cols - box_width) / 2;

    if (first_render || force_redraw) {
        printf(CLEAR_SCREEN);
        printf(HIDE_CURSOR);
        
        if (!minimal_mode) {
            MOVE_CURSOR(start_y, start_x);
            printf(BORDER_COLOR "╭");
            for(int i = 0; i < box_width - 2; i++) {
                if (i == (box_width - 25) / 2) {
                    printf("[ BURNING IN PROGRESS ]");
                    i += 22; 
                } else {
                    printf("─");
                }
            }
            printf("╮" RESET_COLOR);
            
            for (int i = 1; i < box_height - 1; i++) {
                MOVE_CURSOR(start_y + i, start_x);
                printf(BORDER_COLOR "│" RESET_COLOR);
                MOVE_CURSOR(start_y + i, start_x + box_width - 1);
                printf(BORDER_COLOR "│" RESET_COLOR);
            }
            
            MOVE_CURSOR(start_y + box_height - 1, start_x);
            printf(BORDER_COLOR "╰");
            for(int i = 0; i < box_width - 2; i++) printf("─");
            printf("╯" RESET_COLOR);

            MOVE_CURSOR(start_y + 2, start_x + 4);
            printf("OS Source: %s", os_list.name);
            
            MOVE_CURSOR(start_y + 3, start_x + 4);
            printf("Targets (%d):", target_count);

            int current_line = 0;
            for (int i = 0; i < usb_count; i++) {
                if (burn_usbs[i]) {
                    MOVE_CURSOR(start_y + 4 + current_line, start_x + 6);
                    printf("▪ %s", usb_list[i].name);
                    current_line++;
                }
            }
            
            MOVE_CURSOR(start_y + box_height, start_x + (box_width - 19) / 2);
            printf("\033[90m[ Press 'Q' to abort ]\033[0m");
        }
        first_render = 0; 
    }

    double gb_written = (double)total_end / 1073741824.0;
    double gb_total = (double)os_list.file_size / 1073741824.0;

    if (minimal_mode) {
        int center_y = rows / 2;
        int center_x = (cols - 14) / 2; 
        if (center_x < 1) center_x = 1;
        if (center_y < 1) center_y = 1;
        
        MOVE_CURSOR(center_y, 1);
        printf("\033[2K"); 
        MOVE_CURSOR(center_y, center_x);
        printf(PROGRESS_COLOR "Burning: %3d%%" RESET_COLOR, percentage);

        if (rows > 3) {
            MOVE_CURSOR(center_y + 1, 1);
            printf("\033[2K");
            MOVE_CURSOR(center_y + 1, center_x - 3);
            printf("%.2f / %.2f GB", gb_written, gb_total);
        }
    } else {
        int progress_center_y = start_y + 5 + target_count + (max_icon_lines / 2);

        // APLICAMOS EL ICONO DINÁMICO AQUÍ
        int os_start_y = progress_center_y - (os_lines / 2);
        for (int i = 0; i < os_lines; i++) {
            MOVE_CURSOR(os_start_y + i, start_x + 4);
            printf("%s", active_os_icon[i]); 
        }

        int bar_width = 30; 
        int filled = (percentage * bar_width) / 100;
        int bar_x_offset = 20; 
        
        MOVE_CURSOR(progress_center_y, start_x + bar_x_offset);
        printf("[");
        printf(PROGRESS_COLOR);
        for (int i = 0; i < bar_width; i++) {
            if (i < filled) printf("█"); 
            else printf("░");            
        }
        printf(RESET_COLOR);
        printf("] %3d%%", percentage);

        int usb_start_y = progress_center_y - (usb_lines / 2);
        int usb_x_offset = bar_x_offset + bar_width + 9; 
        
        for (int i = 0; i < usb_lines; i++) {
            MOVE_CURSOR(usb_start_y + i, start_x + usb_x_offset);
            printf("%s", USBICON_0[i]);
        }

        MOVE_CURSOR(progress_center_y + 2, start_x + bar_x_offset + 2);
        printf("Data: %.2f GB / %.2f GB    ", gb_written, gb_total);
    }

    fflush(stdout); 

    // AL TERMINAR EL 100%
    if (total_end >= os_list.file_size) {
        first_render = 1;
        last_cols = 0; 
        last_rows = 0;
        printf(SHOW_CURSOR); 
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
        fcntl(STDIN_FILENO, F_SETFL, orig_fcntl_flags); 
    }

    return 0; 
}