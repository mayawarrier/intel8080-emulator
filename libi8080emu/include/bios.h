
#ifndef CPM_H
#define CPM_H

#include "devices.h"
#include "i8080_predef.h"

/* BIOS 00 BOOT: Initializes the devices and hardware. */
I8080_CDECL int bios_cold_boot(cpm80_computer * const computer);

/* BIOS 01 WBOOT: Terminates running program, reloads CPP 
 * and BDOS from disk, and calls the command processor/CPP. */
I8080_CDECL int bios_warm_boot(cpm80_computer * const computer);

/* BIOS 02 CONST: If a character is waiting to be read from the 
 * logical console device, returns 0x00 else 0xFF in register A. */
I8080_CDECL int bios_console_status(cpm80_computer * const computer);

/* BIOS 03 CONIN: Reads the next character from the logical console
 * device into register A. */
I8080_CDECL int bios_console_in(cpm80_computer * const computer);

/* BIOS 04 CONOUT: Writes the character in register C to the logical
 * console device. */
I8080_CDECL int bios_console_out(cpm80_computer * const computer);

/* BIOS 05 LIST: Writes the character in register C to the logical
 * printer/list device. */
I8080_CDECL int bios_printer_out(cpm80_computer * const computer);

/* BIOS 06 PUNCH: Writes the character in register C to the logical
 * punch device. */
I8080_CDECL int bios_punch_out(cpm80_computer * const computer);

/* BIOS 07 READER: Reads the next character from the logical reader
 * device into register A.  */
I8080_CDECL int bios_reader_in(cpm80_computer * const computer);

/* BIOS 08 HOME: Moves to track 0 on the selected disk. */
I8080_CDECL int bios_disk_home(cpm80_computer * const computer);

/* BIOS 09 SELDSK: Select disk to be used in subsequent disk operations.
 * Pass disk number (0-15) in register C.
 * If LSB of register E is 0, BIOS will perform first-time initialization of the disk.
 * If LSB of register E is 1, BIOS assumes disk is already initialized.
 * If successful, returns address of the Disk Parameter Header (DPH) else 0x00 in {HL}. */
I8080_CDECL int bios_disk_select(cpm80_computer * const computer);

/* BIOS 10 SETTRK: Set the track number for next read/write.
 * Pass track number in {BC}. */
I8080_CDECL int bios_disk_set_track(cpm80_computer * const computer);

/* BIOS 11 SETSEC: Set the sector number for next read/write.
 * The physical sector number above may be different from the 
 * logical sector number since sectors may be interlaced on the disk to 
 * improve performance. If the address of the sector skew table in the DPH is not 0, 
 * a call to SECTRAN is required before calling SETSEC.
 * Pass translated sector number in {BC}. */
I8080_CDECL int bios_disk_set_sector(cpm80_computer * const computer);

/* BIOS 12 SETDMA: Set the address of a buffer to be read to or written from
 * by the disk READ and WRITE functions. It must be large enough to hold one
 * standard record/logical sector (128 bytes). */
I8080_CDECL int bios_disk_set_dma_addr(cpm80_computer * const computer);

/* BIOS 13 READ: Transfer one standard record from the selected sector on disk 
 * to the DMA buffer. Returns in register A 0 for success, 1 for unrecoverable 
 * error, and 0xff if media changed. */
I8080_CDECL int bios_disk_read(cpm80_computer * const computer);

/* BIOS 14 WRITE: Transfer a standard record from the DMA buffer to the selected
 * sector on disk. If the physical sector size is larger than a standard 
 * record (128 bytes), the sector may have to be pre-read before the record is 
 * written. Pass disk de-blocking code in register C:
 * - if C = 0, perform deferred write to a potentially allocated sector. If the 
     sector is already allocated, it is pre-read and the record is inserted 
     where required. Actual flush to disk is deferred until necessary.
   - if C = 1, perform directory write, i.e. flush write to disk immediately.
   - if C = 2, perform deferred write to an unallocated sector, writing to its first 
     record without pre-reading. Actual flush to disk is deferred until necessary. */
I8080_CDECL int bios_disk_write(cpm80_computer * const computer);

/* BIOS 15 LISTST: If a character is waiting to be read from the
 * logical printer/list device, returns 0x00 else 0xFF in register A. */
I8080_CDECL int bios_printer_status(cpm80_computer * const computer);

/* BIOS 16 SECTRAN: Convert a logical sector number to a physical sector number.
 * Pass sector number in {BC}, and skew table memory address in {DE}.
 * Returns translated sector number in {HL}. */
I8080_CDECL int bios_disk_sector_translate(cpm80_computer * const computer);

#include "i8080_predef_undef.h"

#endif