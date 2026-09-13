#ifndef RENDER_LOAD_H
#define RENDER_LOAD_H

#include "types.h"

int load(DiskImage os_list, RemovableDrive usb_list[], int usb_count, const bool burn_usbs[PATH_MAX], uint64_t total_end);

#endif