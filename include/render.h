#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>
#include "types.h"

int ui(DiskImage os_list[],int os_count, RemovableDrive usb_list[], int usb_count, bool usbs_marcados[]);

#endif