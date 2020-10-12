/*
 * Abstractions of CP/M serial and disk devices.
 */

#ifndef CPM80_VM_DEVICES_H
#define CPM80_VM_DEVICES_H

#include "vm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CP/M logical serial device */
struct cpm80_serial_ldevice 
{
	/* Handle to underlying device. */
	void *dev;
	/* Return 0 for successful initialization. */
	int(*init)(void *dev);
	/* Return 0 if device is ready for fetch/send. */
	int(*status)(void *dev);
	/* Fetch character from device. Blocking (i.e. must succeed). */
	char(*in)(void *dev);
	/* Send character to device. Blocking (i.e. must succeed). */
	void(*out)(void *dev, char c);
};

/* CP/M logical disk drive */
struct cpm80_disk_ldevice
{
	/* Handle to underlying device. */
	void *dev;
	/* Address of drive's Disk Parameter Header. */
	cpm80_addr_t dph_addr;

	/* Initialize the device and generate its
	 * Disk Parameter Header, if possible.
	 * Assign dph_addr before return.
	 * Return 0 if successful. */
	int(*init)(void *dev);

	/* Move seek to specific track. */
	void(*set_track)(void *dev, unsigned);
	/* Move seek to specific sector. */
	void(*set_sector)(void *dev, unsigned);
	/*
	 * Read 128-byte logical sector from disk.
	 * Return 0 if successful, 1 for unrecoverable error,
	 * and -1 if media changed during read.
	 */
	int(*readl)(void *dev, char buf[128]);
	/*
	 * Write 128-byte logical sector to disk.
	 * See vm_callno.h for details on deblock_code.
	 * Return 0 if successful, 1 for unrecoverable error,
	 * and -1 if media changed during write.
	 */
	int(*writel)(void *dev, char buf[128], int deblock_code);
};

#ifdef __cplusplus
}
#endif

#endif /* CPM80_VM_DEVICES_H */
