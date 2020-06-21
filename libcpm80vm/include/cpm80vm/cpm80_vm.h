
#ifndef CPM80_VM_H
#define CPM80_VM_H

#include "cpm80_defs.h"
#include "cpm80_bios.h"
#include "cpm80_devices.h"

/* 8080-based microcomputer environment compatible with CP/M 2.2 */
CPM80VM_CDECL struct cpm80_vm {
    struct i8080 cpu;
    /*
     * Should load the BIOS into resident memory and set up a jump
     * to automatically launch into a cold boot.
     * Return 0 if successful.
     */
    int(*bootloader)(struct cpm80_vm *);
    /* Logical serial devices */
    struct cpm80_serial_device con; /* console */
    struct cpm80_serial_device lst; /* printer/list */
    struct cpm80_serial_device rdr; /* reader */
    struct cpm80_serial_device pun; /* punch machine */
    /* Up to 16 logical disk drives. */
    struct cpm80_disk_drive drives[16];
};

CPM80VM_CDECL int cpm80_vm_init(struct cpm80_vm * const vm);

/*
 * Configure a VM with default settings: 
 */
CPM80VM_CDECL int cpm80_vm_config_default(struct cpm80_vm * const vm);

/*
 * Starts the VM (cold boot).
 * i.e. Runs the bootloader and begins processor execution from 0x0000. This should:
 * - load a minimum BIOS and automatically jump to the cold boot routine (BOOT/BIOS 00)
 * - The cold boot routine should then:
 *   - initialize hardware/devices as necessary,
 *   - load CP/M BDOS and CCP from the startup disk and give control to CP/M (eq to a
 *     warm boot/WBOOT/BIOS 01, likely to be largely shared with the cold boot routine).
 * If successful, does not return until computer is powered off, else returns -1.
 */
CPM80VM_CDECL int cpm80_vm_start(struct cpm80_vm * const vm);

#undef CPM80VM_CDECL

#endif /* CPM80_VM_H */