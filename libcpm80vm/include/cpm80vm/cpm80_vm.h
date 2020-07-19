
#ifndef CPM80_VM_H
#define CPM80_VM_H

#include "cpm80_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct i8080;
struct cpm80_serial_device;
struct cpm80_disk_device;

/* 8080-based microcomputer environment for CP/M 2.2 */
struct cpm80_vm 
{
	struct i8080 *cpu;
	/*
	 * Called before cold boot. Load the BIOS
	 * into resident memory and do any pre-boot setup.
	 * Return 0 if successful.
	 */
	int(*bootloader)(struct cpm80_vm *);

	/* Serial devices.
	 * Set ptr to 0 if a device is not used.
	 * A console device is mandatory to operate the system. */
	struct cpm80_serial_device *con; /* console */
	struct cpm80_serial_device *lst; /* printer/list */
	struct cpm80_serial_device *rdr; /* tape reader */
	struct cpm80_serial_device *pun; /* punch machine */

	/* Disk devices.
	 * An array of up to 16 disks.
	 * Disk 0 is the boot disk. */
	int ndisks;
	struct cpm80_disk_device *disks;

	/* Memory size in KB (up to 64) */
	int memsize;

	/* private */
	int sel_disk;
	cpm80_addr_t dma_addr;
	cpm80_addr_t cpm_origin;
};

int cpm80_vm_init(struct cpm80_vm *const vm);

/*
 * Configure a VM with default settings:
 */
int cpm80_vm_config_default(struct cpm80_vm *const vm);

/*
 * Starts the VM from scratch.
 * i.e. Runs the bootloader and jumps into a cold boot.
 * If successful, does not return until computer is powered off, else returns -1.
 */
int cpm80_vm_cold_start(struct cpm80_vm *const vm);

#ifdef __cplusplus
}
#endif

#endif /* CPM80_VM_H */
