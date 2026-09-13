#ifndef TYPES_H
#define TYPES_H

#include <limits.h>
#include <stdint.h>   
#include <sys/types.h> 

typedef struct {
    uint64_t total_size; 
    char name[256];              
    char model[256];
    char device_path[PATH_MAX];
} RemovableDrive;

typedef struct {
    uint64_t file_size;
    char name[256];
    char image_path[PATH_MAX];
} DiskImage;

#endif