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

/* Cold boot: Initializes devices and hardware then jumps to warm boot. */
#define BIOS_BOOT		(0)
/* Warm boot: Terminates running program, reloads CP/M from disk, and starts processing commands. */
#define BIOS_WBOOT		(1)
/* Console status: Returns 0x00 in A if a character is waiting to be read from console, else 0xFF. */
#define BIOS_CONST		(2)
/* Console in: Reads a character from console into A. */
#define BIOS_CONIN		(3)
/* Console out: Writes the character in C to console. */
#define BIOS_CONOUT		(4)
/* List out: Writes the character in C to the printer/list device. */
#define BIOS_LIST		(5)
/* Punch out: Writes the character in C to the punch device. */
#define BIOS_PUNCH		(6)
/* Reader in: Reads a character from the reader device into A. */
#define BIOS_READER		(7)
/* Disk home: Moves seek to track 0 on the selected disk. */
#define BIOS_HOME		(8)
/*
 * Select disk: 
 * Selects the disk number in C to be used in subsequent disk operations.
 * If LSB of E is 0, the disk goes through first-time initialization.
 * If LSB of E is 1, the BIOS assumes that the disk is already initialized.
 * Returns the address of the disk's Disk Parameter Header in {HL}, else 0x00 if unsuccessful.
 */
#define BIOS_SELDSK		(9)
/* Set track: Sets the current track from {BC} for next read/write. */
#define BIOS_SETTRK		(10)
/*
 * Set sector:
 * Sets the current (logical) sector from {BC} for next read/write. 
 * If the sectors are skewed on disk (i.e. if the address of the sector translate table 
 * in the DPH is non-0), a call to SECTRAN is required first.
 */
#define BIOS_SETSEC		(11)
/*
 * Set DMA address:
 * CP/M follows a direct memory access model for disks. Sets from {BC} the address of 
 * the buffer to be read to or written from by the BIOS READ and WRITE functions.
 * The buffer should be 128 bytes or larger.
 */
#define BIOS_SETDMA		(12)
/*
 * Disk read:
 * Transfers a 128-byte record/logical-sector from the selected sector on disk to the DMA buffer.
 * Returns exit code in A: 0 for success, 1 for unrecoverable error, and 0xff if media changed.
 */
#define BIOS_READ		(13)
/* 
 * Disk write:
 * Transfers a 128-byte record/logical-sector from the DMA buffer to the selected sector on disk.
 * A physical sector may be larger than 128 bytes - if so it may have to be pre-read before
 * the record is written (de-blocking). Modify this behavior by passing a de-blocking code in C:
 * - if C = 0, performs deferred write to a potentially allocated sector. If the
 *   sector is already allocated, it is pre-read and the record is inserted where required.
 * - if C = 1, performs undeferred write, i.e. flushes write to disk immediately.
 * - if C = 2, performs deferred write to an unallocated sector, writing to its first
 *   record without pre-reading.
 * Returns exit code in A: 0 for success, 1 for unrecoverable error, and 0xff if media changed.
 */
#define BIOS_WRITE		(14)
/* List status: Returns 0x00 in A if a character is waiting to be read from the printer/list device, else 0xFF. */
#define BIOS_LISTST		(15)
/*
 * Sector translate:
 * Translates sector numbers to account for skewing of sectors on disk. 
 * Translates logical sector number in {BC} to a physical sector number, using the translate table address in {DE}.
 * Returns translated sector number in {HL}.
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

#endif /* CPM80_BIOS_H */
