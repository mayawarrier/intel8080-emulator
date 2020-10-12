/*
 * A barebones implementation of cpm80_vm.
 * Depends on: stdio.h, string.h.
 *
 * Emulates a console device and up to 4 IBM 3740 floppy disks (each 243K).
 * Floppy disks are emulated in memory.
 */

#ifndef CPM80_TEST_VM_H
#define CPM80_TEST_VM_H

#include "i8080_types.h"
#define SIMPLEVM_MAX_DISKS (4)

#ifdef __cplusplus
extern "C" {
#endif

struct cpm80_vm_simple_params
{
	/* Ptr to 16K or more of memory to use as RAM (max 64K). */
	i8080_word_t *working_memory;
	/* Highest usable address value i.e. len(working_memory) - 1 */
	i8080_addr_t maxaddr;
	int ndisks;
	/* Ptr to 243K or more of memory to store disk data (max 972K).
	 * ndisks should correspond to the length of the buffer i.e.
	 * if ndisks = 2, the buffer should be 2 x 243K = 486K in length. */
	char *disk_memory;
};

struct cpm80_vm_simple;

/*
 * Initialize the VM.
 * cpmbin is the CP/M 2.2 binary. The binary is loaded into 
 * its default location at 7K before the end of working memory.
 * Returns 0 if successful.
 */
int simple_vm_init(struct cpm80_vm_simple *const,
	const struct cpm80_vm_simple_params *, const i8080_word_t cpmbin[0x1633]);

/*
 * Start the VM.
 * If unsuccessful, returns -1.
 * If successful, returns a cpm80_vm_exitcode once 
 * the VM is powered off or runs into a fatal error.
 * Check that the return value is VM80_POWEROFF
 * to verify that no errors occured during runtime.
 */
int simple_vm_start(struct cpm80_vm_simple *const);

#ifdef __cplusplus
}
#endif

#endif /* CPM80_TEST_VM */
