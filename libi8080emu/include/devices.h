
#ifndef _CPM_DEVICES_H_
#define _CPM_DEVICES_H_

#include "i8080.h"
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

/* 8080-based microcomputer environment compatible with CP/M 2.2 */
I8080_CDECL struct cpm80_computer {
    struct i8080 cpu;
    /* 
     * Should load the BIOS into resident memory and set up a jump
     * to automatically launch into a cold boot.
     * Return 0 if successful. 
     */
    int(*bootloader)(struct cpm80_computer *);
    /* Logical serial devices */
    struct cpm80_serial_device con; /* console */
    struct cpm80_serial_device lst; /* printer/list */
    struct cpm80_serial_device rdr; /* reader */
    struct cpm80_serial_device pun; /* punch machine */
    /* Up to 16 logical disk drives. */
    struct cpm80_disk_drive drives[16];
};

I8080_CDECL int cpm80_computer_init(struct cpm80computer * computer);

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
I8080_CDECL int cpm80_computer_add_disk_drives(const int num_disks, unsigned int * disk_dph_arr);

/* 
 * Performs a cold boot (powers on the system).
 * i.e. Runs the bootloader and begins processor execution from 0x0000. This should:
 * - load a minimum BIOS and automatically jump to the cold boot routine (BOOT/BIOS 00)
 * - The cold boot routine should then:
 *   - initialize hardware/devices as necessary,
 *   - load CP/M BDOS and CCP from the startup disk and give control to CP/M (eq to a
 *     warm boot/WBOOT/BIOS 01, likely to be largely shared with the cold boot routine).
 * Returns 0 if successful.
 */
I8080_CDECL int cpm80_computer_boot(struct cpm80_computer * const computer);



#include "i8080_predef_undef.h"

#endif