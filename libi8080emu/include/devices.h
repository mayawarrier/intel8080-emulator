
#ifndef _DEVICES_H_
#define _DEVICES_H_

#include "i8080.h"
#include "i8080_predef.h"

/* Abstraction of a serial device. */
I8080_CDECL struct logical_serial_device {
    /* Handle to the underlying hardware device. */
    void * device_context;
    char * device_code;
    /* Fetch character from device. Blocking. */
    char(*in)(struct logical_serial_device *);
    /* Send character to device. Blocking. */
    void(*out)(struct logical_serial_device *, char);
    /* Return 1 if device is ready to be read from/written to,
     * 0 otherwise. */
    int(*status)(struct logical_serial_device *);
};

/* Abstraction of a disk drive. */
I8080_CDECL struct logical_disk_device {
    /* Handle to the underlying disk drive/device. */
    void * device_context;
};

/* Handle to a CP/M 2.2 compatible microcomputer environment. */
I8080_CDECL typedef struct microcomputer {
    i8080 cpu;
    /* Logical console device, input & output.
     * Can be mapped to TTY, CRT, BAT or UC1. */
    struct logical_serial_device con;
    /* Logical printer device, output only.
     * Can be mapped to TTY, CRT, LPT or UL1. */
    struct logical_serial_device lst;
    /* Logical reader device, input only.
     * Can be mapped to TTY, PTR, UR1 or UR2. */
    struct logical_serial_device rdr;
    /* Logical punch device, output only.
     * Can be mapped to TTY, PTP, UR1 or UR2. */
    struct logical_serial_device pun;
    /* Up to 16 logical disk drives. At least one
     * disk must be available to boot from. See boot.c. */
    struct logical_disk_device[16] disks;
} cpm80_computer;

/* Resets/initializes a new microcomputer handle. */
I8080_CDECL int reset_microcomputer(struct microcomputer * const computer);

#include "i8080_predef_undef.h"

#endif