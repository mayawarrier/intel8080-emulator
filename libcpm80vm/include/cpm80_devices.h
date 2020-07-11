
#ifndef CPM80_DEVICES_H
#define CPM80_DEVICES_H

#include "cpm80_types.h"

/* CP/M logical serial device */
struct cpm80_serial_device 
{
	/* Handle to underlying device. */
	void *dev;
	/* Return 0 for successful device initialization. */
	int(*init)(void *dev);
	/* Return 0 if device is ready for fetch/send. */
	int(*status)(void *dev);
	/* Fetch character from device. Blocking. */
	char(*in)(void *dev);
	/* Send character to device. Blocking. */
	void(*out)(void *dev, char c);
};

#define CPM80_MAX_DISKS (16)

/* CP/M logical disk drive */
struct cpm80_disk_drive 
{
	/* Handle to underlying device. */
	void *dev;
	/* Address of drive's Disk Parameter Header. */
	cpm80_addr_t dph_addr;

	/* Return 0 for successful device initialization. */
	int(*init)(void *dev);
	/* Move seek to specific track. */
	void(*set_track)(void *dev, int);
	/* Move seek to specific sector. */
	void(*set_sector)(void *dev, int);
	/*
	 * Read 128-byte logical sector from disk.
	 * Return 0 if successful, 1 for unrecoverable error,
	 * and -1 if media changed during read.
	 */
	int(*readl)(void *dev, char buf[128]);
	/*
	 * Write 128-byte logical sector to disk.
	 * Return 0 if successful, 1 for unrecoverable error,
	 * and -1 if media changed during write.
	 */
	int(*writel)(void *dev, char buf[128]);
};

#endif /* CPM80_DEVICES_H */
