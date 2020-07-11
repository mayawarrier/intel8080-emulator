/*
 * CP/M BIOS helpers.
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

#include "cpm80_types.h"

struct cpm80_vm;

/* Cold boot, initializes the devices and hardware. */
#define BIOS_BOOT		(0)
/* Terminates running program, reloads CPP and BDOS from disk, and calls the command processor/CPP. */
#define BIOS_WBOOT		(1)
/* If a character is waiting to be read from the logical console device, returns 0x00 else 0xFF in register A. */
#define BIOS_CONST		(2)
/* Reads the next character from the logical console device into register A. */
#define BIOS_CONIN		(3)
/* Writes the character in register C to the logical console device. */
#define BIOS_CONOUT		(4)
/* Writes the character in register C to the logical printer/list device. */
#define BIOS_LIST		(5)
/* Writes the character in register C to the logical punch device. */
#define BIOS_PUNCH		(6)
/* Reads the next character from the logical reader device into register A. */
#define BIOS_READER		(7)
/* Moves to track 0 on the selected disk. */
#define BIOS_HOME		(8)
/*
 * Select disk to be used in subsequent disk operations. Pass disk number (0-15) in register C.
 * If LSB of register E is 0, BIOS will perform first-time initialization of the disk.
 * If LSB of register E is 1, BIOS assumes disk is already initialized.
 * If successful, returns address of the Disk Parameter Header (DPH) else 0x00 in {HL}.
 */
#define BIOS_SELDSK		(9)
/* Set the track number for next read/write. Pass track number in {BC}. */
#define BIOS_SETTRK		(10)
/*
 * Set the sector number for next read/write. The physical sector number above may be
 * different from the logical sector number since sectors may be interlaced on the disk to
 * improve performance. If the address of the sector skew table in the DPH is not 0,
 * a call to SECTRAN is required before calling SETSEC. Pass translated sector number in {BC}.
 */
#define BIOS_SETSEC		(11)
/*
 * Set the address of a buffer to be read to or written from by the disk READ and WRITE functions.
 * It must be large enough to hold one standard record/logical sector (128 bytes).
 */
#define BIOS_SETDMA		(12)
/*
 * Transfer one standard record from the selected sector on disk to the DMA buffer. Returns in
 * register A 0 for success, 1 for unrecoverable error, and 0xff if media changed.
 */
#define BIOS_READ		(13)
/* Transfer a standard record from the DMA buffer to the selected sector on disk. If the physical
 * sector size is larger than a standard record (128 bytes), the sector may have to be pre-read
 * before the record is written. Pass disk de-blocking code in register C:
 * - if C = 0, perform deferred write to a potentially allocated sector. If the
 *   sector is already allocated, it is pre-read and the record is inserted
 *   where required. Actual flush to disk is deferred until necessary.
 * - if C = 1, perform directory write, i.e. flush write to disk immediately.
 * - if C = 2, perform deferred write to an unallocated sector, writing to its first
 *   record without pre-reading. Actual flush to disk is deferred until necessary.
 */
#define BIOS_WRITE		(14)
/*
 * If a character is waiting to be read from the logical printer/list device, returns 0x00
 * else 0xFF in register A.
 */
#define BIOS_LISTST		(15)
/*
 * Convert a logical sector number to a physical sector number. Pass sector number in {BC}, and
 * skew table memory address in {DE}. Returns translated sector number in {HL}.
 */
#define BIOS_SECTRAN	(16)

/*
 * Calls a BIOS function with the given code.
 * Returns 0 if successful.
 */
int cpm80_bios_call_function(struct cpm80_vm *const vm, int call_code);

/*
 * Generates disk definitions in BIOS memory.
 *
 * Adapted from the CP/M 2.0 disk re-definition library
 * (http://www.gaby.de/cpm/manuals/archive/cpm22htm/axf.asm)
 *
 * Each element in disk_params is a disk parameter list with 10 params:
 * - dn: Disk number 0,1,2... num_disks-1
 * - fsc: First sector number (usually 0 or 1)
 * - lsc: Last sector number on a track
 * - skf: Skew factor of the disk (see SECTRAN), 0 for no skew
 * - bls: Data block size
 * - dks: Disk size in increments of block size
 * - dir: Number of elements in a directory
 * - cks: Number of directory elements to checksum (file modify check)
 * - ofs: Number of tracks to skip at the beginning (the first 2 tracks on a disk are usually system-reserved)
 * - k16: If 0, forces file extents to 16K each
 *
 * The list may also contain only 2 params dn and dm, which defines disk dn with the same characteristics as disk dm.
 * The first element in every parameter list should be the number of params in that list, followed the params
 * in sequence as described above.
 *
 * disk_defns_begin is the address of a buffer in BIOS memory large enough to accomodate all the disk definitions.
 * This is filled with the following data sequentially:
 * - each disk's Disk Parameter Header (DPH, 16 bytes)
 * - each disk's Disk Parameter Block (DPB, 15 bytes) and Sector Translate Table (XLT, lsc-fsc+1 bytes, generated only when skf is non-zero)
 *   - if disk parameters are shared between disks, their DPBs and XLTs will also be shared 
 *     (see http://www.gaby.de/cpm/manuals/archive/cpm22htm/axa.htm for example layout)
 *
 * disk_ram_begin is the address of a buffer in BIOS memory large enough to contain:
 * - the DIRBUF (directory access buffer, 128 bytes)
 * - each disk's allocation vector (ALV, ciel(dks/8) bytes) and checksum vector (CSV, floor(cks/4) bytes)
 *
 * disk_dph_out will contain the addresses to the DPHs of created disk drives when done. The number of disks is capped at 16.
 * Returns 0 if successful.
 */
int cpm80_bios_redefine_disks(struct cpm80_vm *const vm, const int num_disks,
	const unsigned *const *disk_params, cpm80_addr_t disk_defns_begin, cpm80_addr_t disk_ram_begin, cpm80_addr_t *disk_dph_out);

#endif /* CPM80_BIOS_H */
