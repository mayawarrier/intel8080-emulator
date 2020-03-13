
#ifndef CPM80_DEVICES_H
#define CPM80_DEVICES_H

#include "cpm80_defs.h"

/* CP/M logical serial device */
CPM80VM_CDECL struct cpm80_serial_device {
    /* Handle to underlying device. */
    void * dev;
    /* Initialize device, return 0 if successful. */
    int(*init)(void * dev);
    /* Return 0 if device is ready. */
    int(*status)(void * dev);
    /* Fetch character from device. Blocking. */
    char(*in)(void * dev);
    /* Send character to device. Blocking. */
    void(*out)(void * dev, char c);
};

/* CP/M logical disk drive */
CPM80VM_CDECL struct cpm80_disk_drive {
    /* Handle to the underlying drive/device. */
    void * dev;
    /* Address of the drive's Disk Parameter Header */
    cpm80_addr_t dph_addr;
    /* Initialize drive, return 0 if successful. */
    int(*init)(void * dev);
    /* Move the seek to a specific track */
    void(*set_track)(void * dev, int);
    /* Move the seek to a specific sector */
    void(*set_sector)(void * dev, int);
    /* 
     * Read 128-byte logical sector from the disk. 
     * Return 0 if successful, 1 for unrecoverable error, 
     * and -1 if media changed. 
     */
    int(*readl)(void * dev, char buf[128]);
    /* 
     * Write 128-byte logical sector to the disk. 
     * Return 0 if successful, 1 for unrecoverable error, 
     * and -1 if media changed. 
     */
    int(*writel)(void * dev, char buf[128]);
};

#endif /* CPM80_DEVICES_H */