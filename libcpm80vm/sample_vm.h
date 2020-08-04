/*
 * A barebones implementation of cpm80_vm.
 *
 * Emulates a console device and up to 4 IBM 3740 floppy disks (each 243K).
 * Requires at least 259K of memory (16K of RAM for CP/M + 243K for a floppy).
 */

#ifndef CPM80_VM_EXAMPLE_H
#define CPM80_VM_EXAMPLE_H

#include "i8080_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Set a buffer to act as the VM's RAM (min 16K, max 64K).
 * maxaddr is the highest valid address value i.e. len(memory) - 1. 
 */
void sample_vm_set_working_memory(i8080_word_t *memory, i8080_addr_t maxaddr);

/*
 * Set a buffer to act as the VM's floppy disk memory (min 243K, max 972K).
 * ndisks should correspond with the length of the buffer i.e.
 * if ndisks = 2, the buffer should be 2 x 243K = 486K in length. 
 */
void sample_vm_set_disk_memory(char *memory, int ndisks);

/*
 * Initialize the VM.
 * Call this only after set_working_memory() and set_disk_memory().
 * Returns 0 if successful.
 */
int sample_vm_init(void);

/*
 * Power on the VM and start running instructions.
 * If power on was unsuccessful, returns -1. Else returns only
 * after poweroff or on fatal error, with one of the exit codes in
 * cpm80_vm_exitcode. On exit, you should assert that this code is
 * always VM80_POWEROFF. A poweroff is triggered by raising SIGINT
 * (Ctrl+C on the command line).
 */
int sample_vm_start(void);

#ifdef __cplusplus
}
#endif

#endif /* CPM80_VM_EXAMPLE_H */
