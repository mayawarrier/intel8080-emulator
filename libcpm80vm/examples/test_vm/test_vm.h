/*
 * A barebones implementation of cpm80_vm.
 *
 * Emulates the console device and up to 16 8" SSSD 250K
 * IBM-PC compatible floppy disks.
 *
 * See https://jeffpar.github.io/kbarchive/kb/075/Q75131/
 * for more details on the the disk format.
 */

#ifndef CPM80_TEST_VM_H
#define CPM80_TEST_VM_H

#include "i8080_types.h"

/* Actual size in bytes of the 
 * 250K floppy disk (250.25K) */
#define IBM_PC_250K (256256)

#ifdef __cplusplus
extern "C" {
#endif

struct cpm80_test_vm_params
{
	/* VM memory (max 64K) */
	i8080_word_t *memory;
	/* Max address value i.e. len(memory)-1 */
	unsigned maxaddr;
	int ndisks;
	/* Array of up to 16 floppy disk buffers (250.25K each).
	 * Indices correspond to CP/M drives i.e. disk 0 will be
	 * CP/M drive A, disk 1 will be drive B and so on. */
	char **disks;
	/* CP/M 2.2 binary. Will be copied into its default
	 * location (7K before the end of memory). */
	i8080_word_t *cpmbin;
	unsigned cpmbin_size;
};

struct cpm80_test_vm;

/* Create and initialize cpm80_test_vm. */
struct cpm80_test_vm *test_vm_create(const struct cpm80_test_vm_params *);

/* Free cpm80_test_vm. */
void test_vm_destroy(struct cpm80_test_vm *);

/*
 * Start cpm80_test_vm.
 * If unsuccessful returns -1, else
 * returns cpu->mexitcode once VM exits.
 * Return value should be cpm80_vm_exitcode::VM80_POWEROFF
 * if VM quit gracefully (i.e. no errors occured during runtime).
 */
int test_vm_start(struct cpm80_test_vm *const);

#ifdef __cplusplus
}
#endif

#endif /* CPM80_TEST_VM_H */
