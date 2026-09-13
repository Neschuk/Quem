#ifndef BURN_H
#define BURN_H

#include "types.h"


int burn(DiskImage os_list,RemovableDrive usb_list[],int usb_count, bool burn_usbs[PATH_MAX]);

#endif