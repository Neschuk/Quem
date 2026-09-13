#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>

#include "../include/types.h"
#include "../include/os.h"
#include "../include/usb.h"
#include "../include/render.h" 
#include "../include/burn.h"

#define MAX_ITEMS 100

#define UI_QUIT   -1
#define UI_RELOAD -2

int main(int argc, char *argv[]) {
    
//-----------------------------------------------------------

    if (geteuid() != 0) {
        printf("\033[33m[!] QUEM requires root privileges to access raw hardware.\033[0m\n");
        printf("\033[90mRequesting sudo -E elevation...\033[0m\n");
        
        execlp("sudo", "sudo", "-E", argv[0], NULL);
        
        fprintf(stderr, "\033[31m[!] ERROR:\033[0m Failed to elevate privileges. Please run as root.\n");
        return EXIT_FAILURE;
    }

    while (true) {
        
        DiskImage os_list[MAX_ITEMS];
        RemovableDrive usb_list[MAX_ITEMS];
        int os_count = 0, usb_count = 0, target_os = -1;
        bool burn_usbs[PATH_MAX] = {false};

        os_count = find_os_path(os_list, MAX_ITEMS);
        usb_count = find_usb_path(usb_list, MAX_ITEMS);

        target_os = ui(os_list, os_count, usb_list, usb_count, burn_usbs);

        if (target_os == UI_QUIT) {
            printf("\n\033[32mExiting QUEM. Safe travels!\033[0m\n");
            break; 
        }

        if (target_os == UI_RELOAD) {
            continue; 
        }

        bool capacity_error = false;
        for (int i = 0; i < usb_count; i++) {
            if (burn_usbs[i] == true) {
                if (os_list[target_os].file_size > usb_list[i].total_size) {
                    fprintf(stderr, "\033[31m[!] ERROR:\033[0m Image size exceeds capacity of target drive '%s'.\n", usb_list[i].name);
                    capacity_error = true;
                }
            }
        }

        if (capacity_error) {
            fprintf(stderr, "\033[33mAborting operation to prevent data corruption.\033[0m\n");
            printf("\033[90m[ Press ENTER to return to main menu ]\033[0m\n");
            tcflush(STDIN_FILENO, TCIFLUSH);
            getchar();
            continue; 
        }

//-----------------------------------------------------------

        int burn_result = burn(os_list[target_os], usb_list, usb_count, burn_usbs);

//-----------------------------------------------------------

        printf("\033[2J\033[H"); 
        printf("\033[?25l");    

        struct termios term, orig_term;
        tcgetattr(STDIN_FILENO, &orig_term);
        term = orig_term;
        term.c_lflag &= ~ECHO; 
        tcsetattr(STDIN_FILENO, TCSANOW, &term);

        if (burn_result == -1) {
            printf("\n\n\n\033[31m       ╭──────────────────────────────────────────────╮\n");
            printf("       │             [ X ] FLASHING ABORTED           │\n");
            printf("       ╰──────────────────────────────────────────────╯\033[0m\n\n");
            printf("       The operation was cancelled by the user.\n");
            printf("       Drives were wiped safely (Zero-Filled) to prevent corruption.\n\n");
        } else {
            printf("\n\n\n\033[32m       ╭──────────────────────────────────────────────╮\n");
            printf("       │      [ OK ] FLASHING COMPLETED SUCCESSFULLY  │\n");
            printf("       ╰──────────────────────────────────────────────╯\033[0m\n\n");
            printf("       All target USB drives are now ready to boot.\n\n");
        }

        printf("\033[90m       [ Press ENTER to return to main menu ]\033[0m\n\n");
        
        fflush(stdout); 
        tcflush(STDIN_FILENO, TCIFLUSH); 
        getchar();

        tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
        printf("\033[?25h"); 
    }

    return EXIT_SUCCESS;
}