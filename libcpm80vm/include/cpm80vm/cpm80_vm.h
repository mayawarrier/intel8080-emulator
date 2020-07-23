/*
 * i8080 microcomputer VM for CP/M 2.2.
 *
 * To bring the VM to a startable state, call cpm80_vm_init() then
 * configure the serial and disk devices below.
 *
 */

#ifndef CPM80_VM_H
#define CPM80_VM_H

#include "cpm80_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct i8080;
struct cpm80_serial_device;
struct cpm80_disk_device;

struct cpm80_vm 
{
	struct i8080 *cpu;
	/* binds C functions to i8080 memory locations */
	struct i8080_monitor cpu_mon;

	/* Memory size in KB (up to 64) */
	int memsize;

	/* Starting address of CP/M in memory.
	 * Usually (memsize - 7) x 1024. */
	cpm80_addr_t cpm_origin;

	/* This is set on cpm80_vm_init() but can be re-assigned
	 * to re-direct/override bios calls. Return 0 if successful. */
	int(*bios_call)(struct cpm80_vm *const, int);

	/* Serial devices.
	 * Set device to NULL if not used.
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

	/* private */
	int sel_disk;
	cpm80_addr_t dma_addr;
};

/* 
 * First-time initialization. 
 * Call this after initializing or modifying cpu, memsize and cpm_origin.
 * - initializes cpu_mon and sets bios_call() handler
 * - binds BIOS entry points in i8080 memory to bios_call()
 * - resets the i8080 and writes a JMP to cold boot at 0x0
 * Returns 0 if successful.
 */
int cpm80_vm_init(struct cpm80_vm *const vm);

/*
 * Start the VM.
 * Call this only after configuring the VM (assigning the serial devices, disks, etc).
 * If successful, returns 0 once computer is powered off, else returns -1 immediately.
 * If not NULL, cpu_exitcode will contain the exit code from i8080.
 */
int cpm80_vm_start(struct cpm80_vm *const vm, int cpu_exitcode[1]);

#ifdef __cplusplus
}
#endif

#endif /* CPM80_VM_H */
