#include <string.h>
#include <stdio.h>
#include <fcntl.h> 
#include <unistd.h>
#include <sys/types.h>
#include <termios.h>

#include "../include/types.h"
#include "../include/os.h"
#include "../include/usb.h"
#include "../include/render_load.h"

int burn(DiskImage os_list, RemovableDrive usb_list[], int usb_count, bool burn_usbs[PATH_MAX]) {
    int fd_iso;
    int fd_usb[PATH_MAX];
    char buffer[4194304]; // 4MB
    ssize_t bytes_leidos;
    uint64_t total_end = 0;
    int aborted = 0; 

    for (int i = 0; i < PATH_MAX; i++) {
        fd_usb[i] = -1;
    }

//-----------------------------------------------------------

    fd_iso = open(os_list.image_path, O_RDONLY);
    if (fd_iso == -1) {
        fprintf(stderr, "\033[31m[!] ERROR:\033[0m Failed to open ISO file: '%s'. Are you running with sudo?\n", os_list.image_path);
        return -1; 
    }
    
    for (int i = 0; i < usb_count; i++) {
        if (burn_usbs[i] == true) {
            fd_usb[i] = open(usb_list[i].device_path, O_WRONLY | O_SYNC);
                
            if (fd_usb[i] == -1) {
                fprintf(stderr, "\033[31m[!] ERROR:\033[0m Failed to open USB drive '%s' (%s).\n", 
                        usb_list[i].name, usb_list[i].device_path);
                
                close(fd_iso);
                for (int j = 0; j < i; j++) {
                    if (fd_usb[j] != -1) close(fd_usb[j]);
                }
                return -1; 
            }
        }
    }

//-----------------------------------------------------------
    
    if (load(os_list, usb_list, usb_count, burn_usbs, total_end) == -1) {
        aborted = 1;
    }

    if (!aborted) {
        while ((bytes_leidos = read(fd_iso, buffer, sizeof(buffer))) > 0) {
            
            for (int i = 0; i < usb_count; i++) {
                if (burn_usbs[i] == true && fd_usb[i] != -1) {
                    write(fd_usb[i], buffer, bytes_leidos);
                }
            }
            
            total_end = total_end + bytes_leidos;
            
            if (load(os_list, usb_list, usb_count, burn_usbs, total_end) == -1) {
                aborted = 1;
                break; 
            }
        }    
    }


//-----------------------------------------------------------


    if (aborted) {
        struct termios term, orig_term;
        tcgetattr(STDIN_FILENO, &orig_term);
        term = orig_term;
        term.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSANOW, &term);

        printf("\033[?25l"); 
        printf("\033[2J\033[H"); 
        printf("\n\n\033[33m       [!] CANCELING OPERATION...\033[0m\n");
        printf("       Zero-filling drive headers to prevent corruption. Please wait");
        fflush(stdout);

        memset(buffer, 0, 65536); 
        
        for (int d = 0; d < 3; d++) {
            printf(".");
            fflush(stdout);

            for (int i = 0; i < usb_count; i++) {
                if (burn_usbs[i] == true && fd_usb[i] != -1) {
                    lseek(fd_usb[i], d * 65536, SEEK_SET);
                    write(fd_usb[i], buffer, 65536); 
                }
            }

            usleep(250000); 
        }

        tcflush(STDIN_FILENO, TCIFLUSH); 
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_term); 
        printf("\033[?25h"); 
    }

//-----------------------------------------------------------
    
    for (int i = 0; i < usb_count; i++) {
        if (burn_usbs[i] == true && fd_usb[i] != -1) {
            close(fd_usb[i]);
        }
    }
    
    close(fd_iso);

    return aborted ? -1 : 0; 
}