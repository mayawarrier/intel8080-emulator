/*
 * A barebones implementation of cpm80_vm.
 * Depends on: stdio.h, string.h.
 *
 * Emulates a console device and up to 4 IBM 3740 floppy disks (each 243K).
 * Floppy disks are emulated in memory.
 */

#include "i8080.h"
#include "vm.h"
#include "vm_devices.h"

#ifndef CPM80_VM_SIMPLE_H
#define CPM80_VM_SIMPLE_H

#define SIMPLEVM_MAX_DISKS (4)

#ifdef __cplusplus
extern "C" {
#endif

struct cpm80_vm_simple
{
	struct vm_vconsole {
		/* C FILE streams */
		void *sin;
		void *sout;
	} vcon_device;

	struct vm_vibm3740 {
		unsigned sector, track;
		char *disk; /* 243KB */
	}
	vdisk_devices[SIMPLEVM_MAX_DISKS];

	struct i8080 cpu;
	struct cpm80_vm base_vm; /* usage of base_vm->udata is reserved */
	struct cpm80_serial_ldevice lcon;
	struct cpm80_disk_ldevice ldisks[SIMPLEVM_MAX_DISKS];
};

struct cpm80_vm_simple_params
{
	/* Ptr to 16K or more of memory to use as RAM (max 64K). */
	i8080_word_t *working_memory;
	/* Highest valid address value i.e. len(working_memory) - 1 */
	i8080_addr_t maxaddr;
	int ndisks;
	/* Ptr to 243K or more of memory to store disk data (max 972K).
	 * ndisks should correspond with the length of the buffer i.e.
	 * if ndisks = 2, the buffer should be 2 x 243K = 486K in length. */
	char *disk_memory;
};

/*
 * Initialize the VM.
 * cpmbin is the CP/M 2.2 binary (CCP + BDOS), which must be exactly 0x1633 (5683) bytes in size.
 * The binary is loaded into its default location at 7K before the end of working memory.
 * Returns 0 if successful.
 */
int simple_vm_init(struct cpm80_vm_simple *const,
	const struct cpm80_vm_simple_params *, const i8080_word_t cpmbin[0x1633]);

/*
 * Start the VM.
 * If unsuccessful, returns -1.
 * If successful, returns a cpm80_vm_exitcode once 
 * the VM is powered off or runs into fatal error.
 * Assert that the return value is VM80_POWEROFF
 * to verify that no errors occured during runtime.
 */
int simple_vm_start(struct cpm80_vm_simple *const);

#ifdef __cplusplus
}
#endif

#endif /* CPM80_VM_SIMPLE_H */
