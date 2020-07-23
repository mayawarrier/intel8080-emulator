
#ifndef CPM80_BIOS_CALLCODES_H
#define CPM80_BIOS_CALLCODES_H

/* Cold boot:
 * The first routine to get control after the bootloader.
 * Initializes devices, prints a signon message, selects boot disk 0, then falls into warm boot. */
#define BIOS_BOOT		(0)
/* Warm boot:
 * Reloads CP/M from selected disk (terminating the current program), sets up system parameters
 * and starts processing commands. */
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

#endif /* CPM80_BIOS_CALLCODES_H */
