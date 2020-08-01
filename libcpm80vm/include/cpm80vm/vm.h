/*
 * i8080 microcomputer VM for CP/M 2.2.
 *
 * To start running cpm80_vm:
 * - Call cpm80_vm_init(),
 * - configure the serial and disk devices,
 * - then call cpm80_vm_poweron().
 *
 * When powered on, you can run CPU instructions in a loop like so:
 * while(!i8080_next(vm->cpu)) {...}
 *
 * To power off, call cpm80_vm_poweroff() within the
 * body of the loop above or on another thread. The next
 * call to i8080_next() will exit with -1. 
 */

#ifndef CPM80_VM_H
#define CPM80_VM_H

#include "vm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct i8080;
struct cpm80_serial_device;
struct cpm80_disk_device;

/* cpu->exitcode is 
 * set to one of these 
 * when the VM quits */
enum cpm80_vm_exitcode
{
	VM80_UNHANDLED_IO = 1,
	VM80_UNHANDLED_INTR,
	VM80_UNEXPECTED_MONITOR_CALL,
	VM80_POWEROFF
}

struct cpm80_vm 
{
	/* CPU.
	 * VM reserves usage of cpu->udata,
	 * cpu->monitor and RST 7 */
	struct i8080 *cpu;

	/* Memory size in KB (up to 64) */
	int memsize;

	/* Starting address of CP/M in memory.
	 * Usually (memsize - 7) x 1024. */
	cpm80_addr_t cpm_origin;

	/* Serial devices.
	 * Set device to NULL if not used.
	 * CP/M requires at least a console device. */
	struct cpm80_serial_device *con; /* console */
	struct cpm80_serial_device *lst; /* printer/list */
	struct cpm80_serial_device *rdr; /* tape reader */
	struct cpm80_serial_device *pun; /* punch machine */

	/* Disk devices.
	 * An array of up to 16 disks.
	 * At least 1 disk is required (to boot from).
	 * Disk 0 is always the boot disk. */
	int ndisks;
	struct cpm80_disk_device *disks;

	/* Set automatically by cpm80_vm_init(). 
	 * Re-assign to override certain bios
	 * calls if necessary. See vm_callno.h.
	 * Return 0 on success, -1 otherwise. */
	int(*bios_call)(struct cpm80_vm *const, int);

	/* ----------------- private ----------------- */

	int sel_disk;
	cpm80_addr_t dma_addr;
	int is_poweron;
	struct i8080_monitor cpu_mon;
};

/*
 * Call after initializing cpu, memsize and cpm_origin.
 * - initializes cpu_mon and sets bios_call() handler
 * - binds BIOS entry points in i8080 memory to bios_call()
 * - writes a JMP to cold boot at 0x0
 * Returns 0 if successful.
 */
int cpm80_vm_init(struct cpm80_vm *const vm);

/*
 * Power on the VM.
 * Call after configuring the VM (assigning
 * serial devices, disks, and so on).
 * Returns 0 if successful, -1 if badly configured.
 */
int cpm80_vm_poweron(struct cpm80_vm *const vm);

/*
 * Power off the VM.
 * Sends an interrupt to i8080 and makes it quit immediately.
 * The next call to i8080_next() exits with -1.
 * If called on another thread, ensure mutual exclusion between
 * i8080_next() and cpm80_vm_poweroff().
 */
void cpm80_vm_poweroff(struct cpm80_vm *const vm);

#ifdef __cplusplus
}
#endif

#endif /* CPM80_VM_H */
