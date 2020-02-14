
#ifndef CPM80_DEVICES_H
#define CPM80_DEVICES_H

#include "i8080_predef.h"

/* CP/M logical serial device */
I8080_CDECL struct cpm80_serial_device {
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
I8080_CDECL struct cpm80_disk_drive {
    /* Handle to the underlying drive/device. */
    void * dev;
    /* Address of the drive's Disk Parameter Header */
    unsigned int dph_addr;
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

/*
 * Add disk drives. At least 1 drive is required to boot from.
 *
 * disk_dph_arr is an array of the addresses to each drive's DPH (Disk Parameter Header).
 * The structure of a drive's DPH is as follows:
 * - 0000: Address of its XLT (Sector Translation Table), 2 bytes
 * - 0002: Free space, 6 bytes
 * - 0008: Address of its directory access buffer, 2 bytes
 * - 000A: Address of its DPB (Disk Parameter Block), 2 bytes
 * - 000C: Address of its CSV (Directory Checksum Vector), 2 bytes
 * - 000E: Address of its ALV (Disk Allocation Vector), 2 bytes.
 *
 * See bios.h/bios_define_disk_drives() to automatically generate DPHs for a set of drive formats.
 * Returns 0 if successful.
 */

#endif /* CPM80_DEVICES_H */