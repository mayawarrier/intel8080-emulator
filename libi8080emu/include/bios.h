/*
 * CP/M BIOS helpers.
 *
 * - bios_define_disk_drives() has been adapted from the CP/M 2.0 disk re-definition library
 *   (mirrored here: http://www.gaby.de/cpm/manuals/archive/cpm22htm/axf.asm)
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

#ifndef _CPM_BIOS_H_
#define _CPM_BIOS_H_

#include "devices.h"
#include "i8080_predef.h"

/*
 * Generates logical disk drive definitions in BIOS memory.
 * Each element in disk_params is a parameter list used to create a disk definition.
 * Parameters are as follows:
 * - dn: Disk number 0,1,2... (num_disks - 1)
 * - fsc: First sector number (usually 0 or 1)
 * - lsc: Last sector number on a track
 * - skf: Skew factor of the disk to speed up access (see SECTRAN), 0 for no skew
 * - bls: Data block size
 * - dks: Disk size in increments of block size (bls)
 * - dir: Number of elements in a directory
 * - cks: Number of dir elements to checksum (used as a file modify check)
 * - ofs: Number of tracks to skip (first 2 tracks on a disk are usually system-reserved)
 * - k16: If 0, forces all file extents to 0 i.e. prevents files from being larger than 16K each
 *
 * At least dn and fsc are required to define a disk.
 * If lsc is 0 for any disk definition, its remaining parameters are assumed to be the same as the previous disk.
 *
 * disk_defns_begin is the address of a buffer in BIOS memory large enough to accomodate all the disk definitions.
 * This area will be filled with the following data sequentially:
 * - each disk's DPH (Disk Parameter Header, 16 bytes)
 * - each disk's DPB (Disk Parameter Block, 15 bytes) optionally followed by its XLT 
 *   (Sector Translate Table, (lsc-fsc+1) bytes, created only when skf is non-zero)
 *
 * disk_ram_begin is the address of a buffer in BIOS memory large enough to contain the following data:
 * - the DIRBUF (directory access buffer, 128 bytes)
 * - each disk's ALV (allocation vector, ciel(dks/8) bytes) and CSV (checksum vector, floor(cks/4) bytes)
 *
 * disk_dph_arr_out will contain the addresses to the DPHs of the created disk drives when done. A maximum of 16 drives can be defined.
 * Returns 0 if successful.
 */
I8080_CDECL int bios_define_disk_drives(const int num_disks, const int ** disk_params, i8080_addr_t disk_defns_begin, i8080_addr_t disk_ram_begin, i8080_addr_t * disk_dph_arr_out);

/* Calls a BIOS function with the given code.
 * BIOS 00 BOOT
   Cold boot, initializes the devices and hardware.
 * BIOS 01 WBOOT
   Terminates running program, reloads CPP and BDOS from disk, and calls the command processor/CPP.
 * BIOS 02 CONST
   If a character is waiting to be read from the logical console device, returns 0x00 else 0xFF in register A.
 * BIOS 03 CONIN
   Reads the next character from the logical console device into register A.
 * BIOS 04 CONOUT
   Writes the character in register C to the logical console device.
 * BIOS 05 LIST
   Writes the character in register C to the logical printer/list device.
 * BIOS 06 PUNCH
   Writes the character in register C to the logical punch device.
 * BIOS 07 READER
   Reads the next character from the logical reader device into register A. 
 * BIOS 08 HOME
   Moves to track 0 on the selected disk.
 * BIOS 09 SELDSK
   Select disk to be used in subsequent disk operations. Pass disk number (0-15) in register C.
   If LSB of register E is 0, BIOS will perform first-time initialization of the disk.
   If LSB of register E is 1, BIOS assumes disk is already initialized.
   If successful, returns address of the Disk Parameter Header (DPH) else 0x00 in {HL}.
 * BIOS 10 SETTRK: 
   Set the track number for next read/write. Pass track number in {BC}.
 * BIOS 11 SETSEC
   Set the sector number for next read/write. The physical sector number above may be 
   different from the logical sector number since sectors may be interlaced on the disk to
   improve performance. If the address of the sector skew table in the DPH is not 0,
   a call to SECTRAN is required before calling SETSEC. Pass translated sector number in {BC}.
 * BIOS 12 SETDMA
   Set the address of a buffer to be read to or written from by the disk READ and WRITE functions.
   It must be large enough to hold one standard record/logical sector (128 bytes).
 * BIOS 13 READ
   Transfer one standard record from the selected sector on disk to the DMA buffer. Returns in 
   register A 0 for success, 1 for unrecoverable error, and 0xff if media changed.
 * BIOS 14 WRITE
   Transfer a standard record from the DMA buffer to the selected sector on disk. If the physical 
   sector size is larger than a standard record (128 bytes), the sector may have to be pre-read 
   before the record is written. Pass disk de-blocking code in register C:
   - if C = 0, perform deferred write to a potentially allocated sector. If the
     sector is already allocated, it is pre-read and the record is inserted
     where required. Actual flush to disk is deferred until necessary.
   - if C = 1, perform directory write, i.e. flush write to disk immediately.
   - if C = 2, perform deferred write to an unallocated sector, writing to its first
     record without pre-reading. Actual flush to disk is deferred until necessary.
 * BIOS 15 LISTST
   If a character is waiting to be read from the logical printer/list device, returns 0x00 
   else 0xFF in register A.
 * BIOS 16 SECTRAN
   Convert a logical sector number to a physical sector number. Pass sector number in {BC}, and 
   skew table memory address in {DE}. Returns translated sector number in {HL}.
 *
 * Returns 0 if successful.
 */
I8080_CDECL int bios_call_function(struct cpm80_computer * const computer, int call_code);

#include "i8080_predef_undef.h"

#endif