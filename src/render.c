#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <limits.h>

#include "../include/types.h"
#include "../include/os.h"
#include "../include/usb.h"
#include "../include/icons.h" 

#define CLEAR_SCREEN      "\033[2J\033[H"
#define HIDE_CURSOR       "\033[?25l"
#define SHOW_CURSOR       "\033[?25h"
#define MOVE_CURSOR(y, x) printf("\033[%d;%dH", (int)(y), (int)(x))
#define RESET_COLOR       "\033[0m"
#define HIGHLIGHT         "\033[7m" 
#define BORDER_COLOR      "\033[32m" 
#define WARNING_COLOR     "\033[33m" 
#define DANGER_COLOR      "\033[31m" 
#define ENTER_ALT_SCREEN  "\033[?1049h"
#define EXIT_ALT_SCREEN   "\033[?1049l"

volatile sig_atomic_t terminal_resized = 1; 
struct termios original_termios;

#define MAX_USB_DRIVES 10
#define ITEM_HEIGHT 7    
#define BLOCK_WIDTH 54   
#define FRAME_WIDTH 58   

typedef struct {
    bool is_stacked;
    bool is_too_small;
    int os_x;
    int os_y;
    int os_max_vis;
    int usb_x;
    int usb_y;
    int usb_max_vis;
} LayoutConfig;

static LayoutConfig calculate_layout(int cols, int rows) {
    LayoutConfig cfg = {0};
    
    cfg.is_too_small = (cols < 62 || rows < 18);
    cfg.is_stacked = (cols < 120); 

    if (cfg.is_stacked) {
        cfg.os_x = (cols - BLOCK_WIDTH) / 2; 
        cfg.usb_x = cfg.os_x;
        
        cfg.os_y = 4; 
        
        cfg.os_max_vis = ((rows / 2) - 4) / ITEM_HEIGHT; 
        if (cfg.os_max_vis < 1) cfg.os_max_vis = 1;
        
        int os_frame_end = (cfg.os_y - 2) + (cfg.os_max_vis * ITEM_HEIGHT) + 3;
        cfg.usb_y = os_frame_end + 3; 
        
        cfg.usb_max_vis = (rows - cfg.usb_y - 3) / ITEM_HEIGHT;
        if (cfg.usb_max_vis < 1) cfg.usb_max_vis = 1;
    } else {
        cfg.os_x = (cols / 2) - FRAME_WIDTH + 2; 
        cfg.usb_x = (cols / 2) + 4;              
        
        cfg.os_y = 5;
        cfg.usb_y = 5;
        
        cfg.os_max_vis = (rows - 9) / ITEM_HEIGHT;
        cfg.usb_max_vis = cfg.os_max_vis;
        
        if (cfg.os_max_vis < 1) cfg.os_max_vis = 1;
        if (cfg.usb_max_vis < 1) cfg.usb_max_vis = 1;
    }

    return cfg;
}

static void handle_resize(int sig) {
    (void)sig;
    terminal_resized = 1;
}

static void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &original_termios);
    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf(HIDE_CURSOR);
    printf(ENTER_ALT_SCREEN);
}

static void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
    printf(SHOW_CURSOR);
    printf(EXIT_ALT_SCREEN);
}

static void render_hardcoded_icon(const char **icon, int start_y, int start_x) {
    for (int i = 0; i < ICON_HEIGHT; i++) {
        MOVE_CURSOR(start_y + i, start_x);
        printf("%s", icon[i]);
    }
}

static void string_to_lower(char *dest, const char *src, size_t max_len) {
    for (size_t i = 0; i < max_len - 1 && src[i] != '\0'; i++) {
        dest[i] = tolower((unsigned char)src[i]);
    }
    dest[max_len - 1] = '\0';
}

static const char **get_os_icon(const char *filename, int index) {
    char lower_name[256] = {0};
    string_to_lower(lower_name, filename, sizeof(lower_name));

    if (strstr(lower_name, "arch")) return ARCH;
    if (strstr(lower_name, "cachy")) return CACHYOS;
    if (strstr(lower_name, "ubuntu")) return UBUNTU;
    if (strstr(lower_name, "debian")) return DEBIAN;
    if (strstr(lower_name, "chrome")) return CHROME;
    if (strstr(lower_name, "mac") || strstr(lower_name, "darwin")) return MAC;
    if (strstr(lower_name, "mint")) return MINT;
    if (strstr(lower_name, "fedora")) return FEDORA;
    if (strstr(lower_name, "redhat") || strstr(lower_name, "rhel")) return REDHAT;
    if (strstr(lower_name, "win") || strstr(lower_name, "windows")) return WINDOWS;

    const char **default_icons[] = { OSICON_0, OSICON_1, OSICON_3, OSICON_4 };
    return default_icons[index % 4];
}

static void draw_pane_frame(int y, int x, int items_count, int width, const char *title, bool is_active) {
    int lines = (items_count * ITEM_HEIGHT) + 1; 
    
    MOVE_CURSOR(y, x);
    if (is_active) printf(BORDER_COLOR);
    printf("╭── ");
    if (is_active) printf(HIGHLIGHT);
    printf(" %s ", title);
    if (is_active) printf(RESET_COLOR BORDER_COLOR);
    
    int title_len = strlen(title) + 2; 
    for (int i = 0; i < width - 5 - title_len; i++) printf("─");
    printf("╮" RESET_COLOR);

    for (int i = 1; i <= lines; i++) {
        MOVE_CURSOR(y + i, x);
        if (is_active) printf(BORDER_COLOR);
        printf("│");
        MOVE_CURSOR(y + i, x + width - 1);
        printf("│" RESET_COLOR);
    }

    MOVE_CURSOR(y + lines + 1, x);
    if (is_active) printf(BORDER_COLOR);
    printf("╰");
    for (int i = 0; i < width - 2; i++) printf("─");
    printf("╯" RESET_COLOR);
}

static void draw_ui_box(int y, int x, bool is_selected, bool is_focused, const char *l1, const char *l2, const char *l3) {
    if (is_selected) {
        MOVE_CURSOR(y - 1, x);
        printf(BORDER_COLOR "╭──────────────────────────────────────╮" RESET_COLOR);
    }

    MOVE_CURSOR(y, x);
    if (is_selected) printf(BORDER_COLOR "│ " RESET_COLOR); else printf("  ");
    if (is_focused) printf(HIGHLIGHT);
    printf("%-36s", l1); 
    if (is_focused) printf(RESET_COLOR);
    if (is_selected) printf(BORDER_COLOR " │" RESET_COLOR);

    MOVE_CURSOR(y + 1, x);
    if (is_selected) printf(BORDER_COLOR "│ " RESET_COLOR); else printf("  ");
    if (is_focused) printf(HIGHLIGHT);
    printf("%-36s", l2);
    if (is_focused) printf(RESET_COLOR);
    if (is_selected) printf(BORDER_COLOR " │" RESET_COLOR);

    MOVE_CURSOR(y + 2, x);
    if (is_selected) printf(BORDER_COLOR "│ " RESET_COLOR); else printf("  ");
    if (is_focused) printf(HIGHLIGHT);
    printf("%-36s", l3);
    if (is_focused) printf(RESET_COLOR);
    if (is_selected) printf(BORDER_COLOR " │" RESET_COLOR);

    MOVE_CURSOR(y + 3, x);
    if (is_selected) printf(BORDER_COLOR "│ " RESET_COLOR); else printf("  ");
    printf("%-36s", "");
    if (is_selected) printf(BORDER_COLOR " │" RESET_COLOR);

    if (is_selected) {
        MOVE_CURSOR(y + 4, x); 
        printf(BORDER_COLOR "╰──────────────────────────────────────╯" RESET_COLOR);
    }
}

static void draw_modal(int cols, int rows, DiskImage *os, RemovableDrive usb_list[], int usb_count, bool target_usbs[], int focus) {
    int width = 64;
    
    int sel_count = 0;
    for(int i = 0; i < usb_count; i++) {
        if(target_usbs[i]) sel_count++;
    }
    
    int height = 10 + sel_count; 
    int start_y = (rows - height) / 2;
    int start_x = (cols - width) / 2;
    
    MOVE_CURSOR(start_y, start_x);
    printf(WARNING_COLOR "╭");
    for(int i = 0; i < width - 2; i++) printf("─");
    printf("╮" RESET_COLOR);
    
    for(int i = 1; i < height - 1; i++) {
        MOVE_CURSOR(start_y + i, start_x);
        printf(WARNING_COLOR "│" RESET_COLOR);
        for(int j = 0; j < width - 2; j++) printf(" ");
        MOVE_CURSOR(start_y + i, start_x + width - 1);
        printf(WARNING_COLOR "│" RESET_COLOR);
    }
    
    MOVE_CURSOR(start_y + 1, start_x + (width - 15) / 2);
    printf(WARNING_COLOR HIGHLIGHT " CONFIRM FLASH " RESET_COLOR);
    
    MOVE_CURSOR(start_y + 3, start_x + 4);
    printf("Are you sure you want to flash the following ISO:");
    
    MOVE_CURSOR(start_y + 4, start_x + 6);
    printf(HIGHLIGHT " %.50s " RESET_COLOR, os->name);
    
    MOVE_CURSOR(start_y + 6, start_x + 4);
    printf("To the following USB drive(s):");
    
    int y_offset = 7;
    for(int i = 0; i < usb_count; i++) {
        if(target_usbs[i]) {
            MOVE_CURSOR(start_y + y_offset, start_x + 6);
            printf(DANGER_COLOR "%s (%s)" RESET_COLOR, usb_list[i].name, usb_list[i].device_path);
            y_offset++;
        }
    }
    
    int btn_y = start_y + height - 3;
    
    MOVE_CURSOR(btn_y, start_x + 14);
    if(focus == 1) printf(HIGHLIGHT BORDER_COLOR "  [ YES ]  " RESET_COLOR);
    else printf("  [ YES ]  ");
    
    MOVE_CURSOR(btn_y, start_x + width - 23);
    if(focus == 0) printf(HIGHLIGHT BORDER_COLOR "  [ NO ]  " RESET_COLOR);
    else printf("  [ NO ]  ");
    
    MOVE_CURSOR(start_y + height - 1, start_x);
    printf(WARNING_COLOR "╰");
    for(int i = 0; i < width - 2; i++) printf("─");
    printf("╯" RESET_COLOR);
}

static void render_frame(DiskImage os_list[], int os_count, RemovableDrive usb_list[], int usb_count, 
             int active_pane, int nav_os, int nav_usb, int target_os, bool target_usbs[], 
             int scroll_os, int scroll_usb, LayoutConfig cfg, int cols, int rows,
             bool modal_open, int modal_btn_focus) {
    
    printf(CLEAR_SCREEN);

    if (cfg.is_too_small) {
        const char *msg1 = "[ ERROR: Terminal too small ]";
        const char *msg2 = "Please enlarge the window to use Quem.";
        MOVE_CURSOR(rows / 2, (cols - strlen(msg1)) / 2);
        printf(WARNING_COLOR "%s" RESET_COLOR, msg1);
        MOVE_CURSOR((rows / 2) + 2, (cols - strlen(msg2)) / 2);
        printf("%s", msg2);
        fflush(stdout);
        return;
    }

    const char *title = "QUEM";
    MOVE_CURSOR(1, (cols - strlen(title)) / 2);
    printf(HIGHLIGHT " %s " RESET_COLOR, title);

    draw_pane_frame(cfg.os_y - 2, cfg.os_x - 2, cfg.os_max_vis, FRAME_WIDTH, "Operating Systems", active_pane == 0 && !modal_open);
    draw_pane_frame(cfg.usb_y - 2, cfg.usb_x - 2, cfg.usb_max_vis, FRAME_WIDTH, "USB Drives", active_pane == 1 && !modal_open);

    int os_limit = (scroll_os + cfg.os_max_vis > os_count) ? os_count : scroll_os + cfg.os_max_vis;
    for (int i = scroll_os; i < os_limit; i++) {
        int y_pos = cfg.os_y + ((i - scroll_os) * ITEM_HEIGHT);
        
        bool is_focused = (active_pane == 0 && nav_os == i && !modal_open);
        bool is_selected = (target_os == i);

        char l1[64], l2[64], l3[64];
        snprintf(l1, sizeof(l1), "%s Name: %.20s", is_selected ? "[X]" : "[ ]", os_list[i].name);
        snprintf(l2, sizeof(l2), "    Size: %.2f GB", (double)os_list[i].file_size / (1024*1024*1024));
        snprintf(l3, sizeof(l3), "    Path: %.22s", os_list[i].image_path);

        draw_ui_box(y_pos, cfg.os_x, is_selected, is_focused, l1, l2, l3);

        const char **selected_icon = get_os_icon(os_list[i].image_path, i); 
        render_hardcoded_icon(selected_icon, y_pos - 1, cfg.os_x + 40);
    }

    int usb_limit = (scroll_usb + cfg.usb_max_vis > usb_count) ? usb_count : scroll_usb + cfg.usb_max_vis;
    for (int i = scroll_usb; i < usb_limit; i++) {
        int y_pos = cfg.usb_y + ((i - scroll_usb) * ITEM_HEIGHT);
        
        bool is_focused = (active_pane == 1 && nav_usb == i && !modal_open);
        bool is_selected = target_usbs[i];

        char l1[64], l2[64], l3[64];
        snprintf(l1, sizeof(l1), "%s Name: %.20s", is_selected ? "[X]" : "[ ]", usb_list[i].name);
        snprintf(l2, sizeof(l2), "    Model: %.21s", usb_list[i].model);
        snprintf(l3, sizeof(l3), "    Capacity: %.2f GB", (double)usb_list[i].total_size / (1024*1024*1024));

        draw_ui_box(y_pos, cfg.usb_x, is_selected, is_focused, l1, l2, l3);

        const char **usb_icons[] = { USBICON_0, USBICON_1, USBICON_2, USBICON_3 };
        render_hardcoded_icon(usb_icons[i % 4], y_pos - 1, cfg.usb_x + 40);
    }

    if (!cfg.is_stacked || rows >= 24) {
        const char *footer = "SPACE: Select | ENTER: Burn | R: Reload | Q: Quit | ARROWS: Nav";
        MOVE_CURSOR(rows - 1, (cols - strlen(footer)) / 2);
        printf("%s", footer);
    }
    
    if (modal_open) {
        draw_modal(cols, rows, &os_list[target_os], usb_list, usb_count, target_usbs, modal_btn_focus);
    }

    fflush(stdout); 
}

int ui(DiskImage os_list[], int os_count, RemovableDrive usb_list[], int usb_count, bool usbs_marcados[]) {
    char stdout_buffer[65536];
    setvbuf(stdout, stdout_buffer, _IOFBF, sizeof(stdout_buffer));

    int active_pane = 0; 
    int nav_os = 0, nav_usb = 0;
    int scroll_os = 0, scroll_usb = 0;
    int target_os = -1; 
    
    bool modal_open = false;
    int modal_btn_focus = 0; // 0 = NO, 1 = YES
    
    int running = 1;
    bool needs_redraw = true;

    enable_raw_mode();

    struct sigaction sa;
    sa.sa_handler = handle_resize;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; 
    sigaction(SIGWINCH, &sa, NULL);

    while (running) {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        LayoutConfig cfg = calculate_layout(w.ws_col, w.ws_row);

        
        
        if (nav_os < scroll_os) scroll_os = nav_os;
        if (nav_os >= scroll_os + cfg.os_max_vis) scroll_os = nav_os - cfg.os_max_vis + 1;
        
        if (nav_usb < scroll_usb) scroll_usb = nav_usb;
        if (nav_usb >= scroll_usb + cfg.usb_max_vis) scroll_usb = nav_usb - cfg.usb_max_vis + 1;

        if (terminal_resized || needs_redraw) {
            render_frame(os_list, os_count, usb_list, usb_count, 
                    active_pane, nav_os, nav_usb, target_os, usbs_marcados, 
                    scroll_os, scroll_usb, cfg, w.ws_col, w.ws_row, 
                    modal_open, modal_btn_focus);
            terminal_resized = 0;
            needs_redraw = false;
        }

        char c;
        if (read(STDIN_FILENO, &c, 1) == -1) continue; 

        if (c == 'q' || c == 'Q') {
            if (modal_open) {
                modal_open = false;
                needs_redraw = true;
            } else {
                target_os = -1; 
                running = 0;
            }
        } 

        else if (c == 'r' || c == 'R') {
            if (!modal_open) {
                target_os = -2; 
                running = 0;
            }
        }


        else if (c == ' ' && !cfg.is_too_small && !modal_open) { 
            if (active_pane == 0) {
                if (os_count > 0) {
                    if (target_os == nav_os) target_os = -1; 
                    else target_os = nav_os;
                }
            } else {
                if (usb_count > 0) {
                    usbs_marcados[nav_usb] = !usbs_marcados[nav_usb];
                }
            }
            needs_redraw = true;
        }
        else if (c == '\n' && !cfg.is_too_small) {
            if (!modal_open) {
                bool has_usb_selected = false;
                for (int i = 0; i < usb_count; i++) {
                    if (usbs_marcados[i]) has_usb_selected = true;
                }

                if (target_os != -1 && has_usb_selected) {
                    modal_open = true;
                    modal_btn_focus = 0; 
                    needs_redraw = true;
                }
            } else {
                if (modal_btn_focus == 1) { // YES
                    running = 0; 
                } else { // NO
                    modal_open = false;
                    needs_redraw = true;
                }
            }
        } 
        else if (c == '\033' && !cfg.is_too_small) { 
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 0) continue;
            if (read(STDIN_FILENO, &seq[1], 1) == 0) continue;

            if (seq[0] == '[') {
                if (modal_open) {
                    if (seq[1] == 'C') modal_btn_focus = 0;
                    if (seq[1] == 'D') modal_btn_focus = 1; 
                    needs_redraw = true;
                } else {
                    
                    switch (seq[1]) {
                        case 'A': 
                            if (active_pane == 0 && nav_os > 0) nav_os--;
                            if (active_pane == 1 && nav_usb > 0) nav_usb--;
                            break;
                        case 'B': 
                            if (active_pane == 0 && nav_os < os_count - 1) nav_os++;
                            if (active_pane == 1 && nav_usb < usb_count - 1) nav_usb++;
                            break;
                        case 'C': 
                            if (!cfg.is_stacked) active_pane = 1;
                            break;
                        case 'D': 
                            if (!cfg.is_stacked) active_pane = 0;
                            break;
                    }
                    
                    if (cfg.is_stacked) {
                        if (seq[1] == 'B' && active_pane == 0 && nav_os == os_count - 1) active_pane = 1;
                        if (seq[1] == 'A' && active_pane == 1 && nav_usb == 0) active_pane = 0;
                    }
                    needs_redraw = true; 
                }
            }
        }
    }

    
    disable_raw_mode();
    printf(CLEAR_SCREEN);
    fflush(stdout);

    setvbuf(stdout, NULL, _IOLBF, 0);
    
    return target_os;
}