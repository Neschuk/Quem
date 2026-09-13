#define _XOPEN_SOURCE 700
#define MIN_IMAGE_SIZE 104857600 // 100 MB

#include <ftw.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>

#include "../include/types.h"
#include "../include/os.h"

static DiskImage *global_os_list;
static int global_image_count = 0;
static int global_max_count = 0;

static int inspect_file(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (global_image_count >= global_max_count) {
        return 1; 
    }

    if (typeflag == FTW_F) { 
        size_t char_count = strlen(fpath); 

        if (char_count >= 4) {

            if (strcmp(fpath + (char_count - 4), ".iso") == 0 || 
                strcmp(fpath + (char_count - 4), ".img") == 0) {
               
               if (sb->st_size >= MIN_IMAGE_SIZE) {
                    

                    snprintf(global_os_list[global_image_count].image_path, PATH_MAX, "%s", fpath);
                    
                    // Asignamos tamaño
                    global_os_list[global_image_count].file_size = sb->st_size;

                    // Extraemos el nombre
                    const char *last_slash = strrchr(fpath, '/');
                    if (last_slash != NULL) {
                        snprintf(global_os_list[global_image_count].name, 256, "%s", last_slash + 1);
                    } else {
                        snprintf(global_os_list[global_image_count].name, 256, "%s", fpath);
                    }

                    global_image_count++;
                }
            }
        }
    }
    return 0;
}

//-----------------------------------------------------------

int find_os_path(DiskImage os_list[], int max_count) {
    global_os_list = os_list;
    global_image_count = 0; 
    global_max_count = max_count;


    const char *search_path = getenv("HOME");

    if (search_path == NULL) {
        struct passwd *pw = getpwuid(getuid());
        if (pw != NULL) {
            search_path = pw->pw_dir;
        } else {
            search_path = "/tmp";
        }
    }

    nftw(search_path, inspect_file, 20, FTW_PHYS);

    return global_image_count;
}