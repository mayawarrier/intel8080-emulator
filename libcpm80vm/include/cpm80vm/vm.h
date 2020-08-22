/*
 * 8-bit virtual machine for CP/M 2.2 based on the Intel 8080.
 *
 * To get the machine running:
 * - implement its serial and disk devices,
 * - initalize the members of cpm80_vm as required,
 * - call cpm80_vm_init()
 * - then cpm80_vm_poweron().
 *
 * When powered on, you can run CPU instructions in a loop like so:
 * while(!i8080_next(vm->cpu)) {...}
 *
 * To power off, call cpm80_vm_poweroff() within the body of
 * the loop above or from another thread. The next call to
 * i8080_next() will exit with -1. 
 * 
 * See sample_vm.c for an example.
 */

#ifndef CPM80_VM_H
#define CPM80_VM_H

#include "vm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct i8080;
struct cpm80_serial_ldevice;
struct cpm80_disk_ldevice;

/* 
 * cpu->exitcode is set to one
 * of these when i8080_next() fails.
 * The first 3 are fatal errors that
 * cause the VM to quit instantly. 
 */
enum cpm80_vm_exitcode
{
	/* The program made an I/O request but cpu->io_read()
	 * or cpu->io_write() is NULL. */
	VM80_UNHANDLED_IO = 1,
	/* The cpu received an interrupt but cpu->interrupt_read()
	 * is NULL. */
	VM80_UNHANDLED_INTR,
	/* A bios call was made from an unexpected memory location.
	 * This should almost never happen. If it does, there might
	 * be a fault in the 8080 binary. */
	VM80_UNEXPECTED_MONITOR_CALL,
	/* Disk direct memory access was out of bounds. */
	VM80_DMA_OUT_OF_BOUNDS,
	/* The VM quit normally i.e. on poweroff. */
	VM80_POWEROFF
};

struct cpm80_vm 
{
	/* CPU.
	 * The usage of cpu->udata,
	 * cpu->monitor and RST 7 is 
	 * reserved for the VM.
	 * A valid CP/M 2.2 CCP + BDOS binary
	 * should be loaded into the CPU's memory
	 * at cpm_origin (see below). */
	struct i8080 *cpu;

	/* Memory size in KB (max 64) */
	int memsize;

	/* Starting address of CP/M in memory.
	 * Usually (memsize - 7) x 1024. */
	cpm80_addr_t cpm_origin;

	/* Serial devices.
	 * Set device to NULL if not used.
	 * CP/M requires at least a console device. */
	struct cpm80_serial_ldevice *con; /* console */
	struct cpm80_serial_ldevice *lst; /* printer/list */
	struct cpm80_serial_ldevice *rdr; /* tape reader */
	struct cpm80_serial_ldevice *pun; /* punch machine */

	/* Disk devices.
	 * An array of up to 16 disks.
	 * At least 1 disk is required (to boot from).
	 * Disk 0 is always the boot disk. */
	int ndisks;
	struct cpm80_disk_ldevice *disks;

	/* Set automatically by cpm80_vm_init(). 
	 * Re-assign to override one or more bios
	 * calls if necessary. See vm_callno.h.
	 * Return 0 on success, -1 otherwise. */
	int(*bios_call)(struct cpm80_vm *const, int);

	/* User data */
	void *udata;

	/* ----------------- private ----------------- */

	int sel_disk;
	cpm80_addr_t dma_addr;
	int is_poweron;
	struct i8080_monitor cpu_mon;
	i8080_word_t(*prev_ih)(const struct i8080 *);
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
 * Sends an interrupt to the CPU, setting cpu->exitcode 
 * to VM80_POWEROFF. The next call to i8080_next() will
 * exit with -1. If called asynchronously, mutually exclude 
 * calls to this and i8080_next().
 */
void cpm80_vm_poweroff(struct cpm80_vm *const vm);

#ifdef __cplusplus
}
#endif

#endif /* CPM80_VM_H */
