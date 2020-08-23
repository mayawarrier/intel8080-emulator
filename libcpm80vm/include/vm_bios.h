/*
 * CP/M BIOS.
 *
 * The following resources were useful:
 * - http://www.gaby.de/cpm/manuals/archive/cpm22htm/axa.asm (Intel MDS-800 I/O drivers in 8080 assembly)
 * - https://www.seasip.info/Cpm/bios.html (description of CP/M BIOS functions)
 * - https://www.seasip.info/Cpm/format22.html (the Disk Parameter Block and CP/M filesystem)
 * - https://www.seasip.info/Cpm/dph.html (the Disk Parameter Header, drive allocation and checksum vectors)
 * - http://www.gaby.de/cpm/manuals/archive/cpm22htm/axg.asm (sector de-blocking for CP/M 2.0 in 8080 assembly)
 * - http://manpages.ubuntu.com/manpages/xenial/man5/cpm.5.html (Ubuntu manpage on CP/M filesystem package)
 * - https://obsolescence.wixsite.com/obsolescence/cpm-internals (Detailed description of CP/M memory map, filesystem, and File Control Block)
 */

#ifndef CPM80_BIOS_H
#define CPM80_BIOS_H

#include "vm_types.h"

#define CPM80_MAX_DISKS (16)
#define CPM80_BIOS_VERSION "2.2"

#ifdef __cplusplus
extern "C" {
#endif

struct cpm80_vm;

/*
 * BIOS implementation.
 *
 * This does not implement any device drivers!
 * See vm.h; implement those before getting here.
 * cpm80_vm.bios_call() points to this function by default.
 *
 * Call a BIOS function with a call number (0-16).
 * Returns 0 if successful.
 */
int cpm80_bios_call_function(struct cpm80_vm *const vm, int callno);

/*
 * Utlity function for disk device implementations.
 * vm->cpu must be initialized first.
 *
 * Generate disk definitions in BIOS memory.
 *
 * Adapted from the CP/M 2.0 disk re-definition library
 * (http://www.gaby.de/cpm/manuals/archive/cpm22htm/axf.asm)
 *
 * Each element in disk_params is a disk parameter list with 10 params:
 * - dn: Disk number 0,1,2... num_disks-1
 * - fsc: First sector number (usually 0 or 1)
 * - lsc: Last sector number on a track
 * - skf: Skew factor of the disk, 0 for no skew
 * - bls: Block size
 * - dks: Disk size in increments of block size
 * - dir: Number of directory elements
 * - cks: Number of directory elements to checksum (file modify check)
 * - ofs: Number of tracks to skip at the beginning (the first 2 tracks on a disk are usually system-reserved)
 * - k16: If 0, forces file extents to 16K
 *
 * The list may also contain only 2 params: dn and dm, which defines disk dn with the same params as disk dm (i.e. a shared defn).
 * The first element in every parameter list should be the number of params in that list, followed the params in sequence as 
 * described above.
 *
 * disk_defns_begin is a ptr to a buffer in BIOS memory large enough to accomodate all the definitions.
 * This is filled with the following data sequentially:
 * - each disk's Disk Parameter Header (DPH, 16 bytes)
 * - each disk's Disk Parameter Block (DPB, 15 bytes) and Sector Translate Table (XLT, lsc-fsc+1 bytes, generated only when skf is non-zero)
 *   - if disk parameters are shared, their DPBs and XLTs will also be shared 
 *     (see http://www.gaby.de/cpm/manuals/archive/cpm22htm/axa.htm for example layout)
 *
 * disk_ram_begin is a ptr to a buffer in BIOS memory large enough to contain:
 * - the DIRBUF (directory access buffer, 128 bytes)
 * - each disk's allocation vector (ALV, ciel(dks/8) bytes) and checksum vector (CSV, cks/4 bytes)
 *
 * disk_dph_out will contain the addresses to the DPHs of the disk drives when done. The number of disks is capped at 16.
 * Returns 0 if successful.
 */
int cpm80_bios_redefine_disks(struct cpm80_vm *const vm, const int num_disks,
	const unsigned *const *disk_params, cpm80_addr_t disk_defns_begin, cpm80_addr_t disk_ram_begin, cpm80_addr_t *disk_dph_out);

#ifdef __cplusplus
}
#endif

#endif /* CPM80_BIOS_H */
