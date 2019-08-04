/* 
 * File:   emu.h
 * Author: dhruvwarrier
 *
 * Starts up the emulator and handles boot.
 * 
 * Created on June 30, 2019, 5:28 PM
 */

#ifndef EMU_H
#define EMU_H

#include <stdbool.h>
#include <stdlib.h>
#include "emu_types.h"
#include "i8080/i8080.h"

/* The starting location of the CP/M Transient Program Area i.e. 
 * the first valid location to load a program written for CP/M. 
 * Set this environment with emu_set_cpm_env(). */
static const emu_addr_t CPM_START_OF_TPA = 0x100;
// Port address that CP/M will treat as its console
static const emu_word_t CPM_CONSOLE_ADDR = 0x00;
/* The first valid location to load a program with the default 
 * environment. Set this environment with emu_set_default_env(). */
static const emu_addr_t DEFAULT_START_PA = 0x40;

// Allocates the largest amount of memory addressable (in this case 64KB)
bool memory_init(emu_word_t * const memory_buf);
// Loads a file into memory. Call only after memory_init(). Returns number of words read.
size_t memory_load(const char * file_loc, emu_word_t * memory, emu_addr_t start_loc);

// Initialize an i8080
void emu_init_i8080(i8080 * const cpu);
// Setup i8080 memory and I/O streams. Returns true if memory stream functions are valid. 
// I/O read/write are allowed to be NULL, but the emulator will crash if an I/O request is made.
bool emu_setup_streams(i8080 * const cpu, read_word_fp read_memory, write_word_fp write_memory, 
        read_word_fp io_port_in, write_word_fp io_port_out);

/* Sets up the environment for some basic CP/M 2.2 BIOS emulation.
 * Some programs written for the 8080 need this environment. At the moment, only 
 * supports CP/M BDOS ops 2 and 9, and WBOOT (which simply quits the emulator).
 * Call this only after the streams have been set. */
void emu_set_cpm_env(i8080 * const cpu);

/* Sets up the default emulator environment.
 * Creates an interrupt vector table at the top 64 bytes of memory,
 * and writes a RST 0 sequence that jumps to after the IVT (0x40, DEFAULT_START_PA). */
void emu_set_default_env(i8080 * const cpu);

// Begin the emulator. Must have properly set up memory and streams first!
bool emu_runtime(i8080 * const cpu);

void emu_i8080_interrupt(i8080 * const cpu);

// Cleans up memory
void emu_cleanup(i8080 * cpu, emu_mem_t * memory_handle);

#endif /* EMU_H */

