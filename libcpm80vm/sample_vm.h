/*
 * A barebones implementation of cpm80_vm.
 *
 * Emulates a console device and up to 4 IBM 3740 floppy disks (each 243K).
 * Requires at least 259K of memory (16K of RAM for CP/M + 243K for a floppy).
 */

#ifndef CPM80_VM_SAMPLE_H
#define CPM80_VM_SAMPLE_H

#include "i8080_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SAMPLEVM_MAX_DISKS (4)

struct i8080;
struct cpm80_vm;
struct cpm80_serial_ldevice;
struct cpm80_disk_ldevice;

struct cpm80_sample_vm
{
	struct vm_vconsole {
		void *sin;
		void *sout;
	} vcon_device;

	struct vm_vibm3740 {
		unsigned sector, track;
		char *disk; /* 243KB */
	}
	vdisk_devices[SAMPLEVM_MAX_DISKS];

	struct i8080 cpu;
	struct cpm80_vm base_vm; /* usage of base_vm->udata is reserved for sample_vm */
	struct cpm80_serial_ldevice lcon;
	struct cpm80_disk_ldevice ldisks[SAMPLEVM_MAX_DISKS];
};

struct cpm80_sample_vm_params
{
	/* Buffer to act as VM's RAM (min 16K, max 64K) */
	i8080_word_t *working_memory;
	/* Highest valid address value i.e. len(working_memory) - 1 */
	i8080_addr_t maxaddr;
	int ndisks;
	/* Buffer to act as  VM's floppy disk memory (min 243K, max 972K).
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
int sample_vm_init(struct cpm80_sample_vm *const,
	const struct cpm80_sample_vm_params *, const i8080_word_t cpmbin[0x1633]);

/*
 * Power on and start running instructions.
 * Returns -1 if poweron is unsuccessful, else returns only
 * on poweroff or fatal error (see cpm80_vm_exitcode).
 * On exit, assert that cpu->exitcode is VM80_POWEROFF to
 * verify that no errors occured during runtime.
 * A poweroff is triggered by raising SIGINT
 * (Ctrl+C on the command line).
 */
int sample_vm_start(struct cpm80_sample_vm *const);

#ifdef __cplusplus
}
#endif

#endif /* CPM80_VM_SAMPLE_H */
