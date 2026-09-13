#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <stdint.h>

#include "../include/types.h"
#include "../include/usb.h"


static void inspect_drive(const char *name, RemovableDrive *usb_list, int *current_count, int max_count) {
    if (*current_count >= max_count) return;

    FILE *fp;
    int removable = 0;
    char path[PATH_MAX];

//-----------------------------------------------------------

    snprintf(path, PATH_MAX, "/sys/block/%s/removable", name);
    if ((fp = fopen(path, "r")) != NULL) {
        if (fscanf(fp, "%d", &removable) != 1) removable = 0;
        fclose(fp);
    }

//-----------------------------------------------------------


    if (removable == 1) {
        int idx = *current_count; 
    
        snprintf(usb_list[idx].name, 256, "%s", name);
        snprintf(usb_list[idx].device_path, PATH_MAX, "/dev/%s", name);

        snprintf(path, PATH_MAX, "/sys/block/%s/size", name);
        if ((fp = fopen(path, "r")) != NULL) {
            unsigned long long size_blocks = 0;
            if (fscanf(fp, "%llu", &size_blocks) == 1) {
                usb_list[idx].total_size = size_blocks * 512ULL; 
            }
            fclose(fp);
        } else {
            usb_list[idx].total_size = 0;
        }

        snprintf(path, PATH_MAX, "/sys/block/%s/device/model", name);
        if ((fp = fopen(path, "r")) != NULL) {

            if (fgets(usb_list[idx].model, 256, fp) != NULL) {
                usb_list[idx].model[strcspn(usb_list[idx].model, "\n")] = '\0'; 
            } else {
                snprintf(usb_list[idx].model, 256, "Unknown");
            }
            fclose(fp);
        } else {
            snprintf(usb_list[idx].model, 256, "Unknown");
        }
        
        (*current_count)++;
    }
}

//-----------------------------------------------------------

int find_usb_path(RemovableDrive usb_list[], int max_count) {
    DIR *dir;
    struct dirent *directory_entry;
    int drive_count = 0; 
    
    dir = opendir("/sys/block/");
    if (dir != NULL) {
        while ((directory_entry = readdir(dir)) != NULL) {

            if (directory_entry->d_name[0] == '.') continue;
            
            if (strncmp(directory_entry->d_name, "loop", 4) == 0) continue;

            inspect_drive(directory_entry->d_name, usb_list, &drive_count, max_count); 
        }
        closedir(dir);
    }

    return drive_count;
}