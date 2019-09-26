/* 
 * File:   emu.h
 * Author: dhruvwarrier
 *
 * This is the emulator layer over the base i8080 implementation.
 * 
 * It provides debugging functionality, and limited emulation of the 
 * CP/M 2.2 BIOS and memory environment, which is required by some 8080
 * programs.
 * 
 * See below for what is supported in the CP/M 2.2 environment.
 * 
 * Created on June 30, 2019, 5:28 PM
 */

#ifndef EMU_H
#define EMU_H

#include "i8080/i8080.h"
#include <stdio.h>

#include "i8080/internal/i8080_predef.h"

I8080_CDECL typedef enum EMU_EXIT_CODE {
    EMU_ERR_MEM_STREAMS,        // A memory stream function (cpu->read_memory, cpu->write_memory) is not initialized.
    EMU_ERR_IO_STREAMS,         // An I/O request was made at runtime but an I/O stream function (cpu->port_in, cpu->port_out) was not initialized.
    EMU_EXIT_SUCCESS            // Successful run.
} emu_exit_code_t;

// Args for emu debug mode.
I8080_CDECL typedef struct {
    FILE * debug_out;                       // Stream that debug output should go to.
    int should_dump_stats;                // Whether or not to dump status of all registers and flags after each instruction.
    int should_dump_memory;               // Whether or not to dump contents of the memory after each instruction.
    const char * mem_dump_format;           // The format specifier applied to each word in the memory dump. Max length is 127 chars.
    int mem_dump_newline_after;             // The number of words after which a newline is inserted in the memory dump.
    emu_addr_t mem_dump_start_addr;         // The start address from which to dump memory.
    emu_addr_t mem_dump_end_addr;           // The end address until which to dump memory.
} emu_debug_args;

/* Loads a file into memory. Returns number of words read. */
I8080_CDECL size_t memory_load(const char * file_loc, emu_word_t * memory, const emu_addr_t start_loc);
// Checks all locations from start_addr to end_addr for read/write errors, using cpu.read_memory() and cpu.write_memory().
// Returns if failure occured, with location stored in cpu->pc.
I8080_CDECL int memory_check_errors(i8080 * const cpu, const emu_addr_t start_addr, const emu_addr_t end_addr);

// Initialize an i8080
I8080_CDECL void emu_init_i8080(i8080 * const cpu);

/* Sets up the environment for some basic CP/M 2.2 BIOS emulation.
 * Some programs written for the 8080 need this environment.
 *
 * At the moment, this only supports CP/M BDOS ops 2 and 9, 
 * and WBOOT at 0x0000 (which launches into a simple command processor).
 * The command processor at WBOOT has 2 commands:
 * RUN addr: Begins execution of a CP/M program starting at addr.
 * QUIT: Quits the emulator.
 *
 * It can be disabled by setting enable_cmd_proc = 0. When disabled, WBOOT jumps
 * to 0x0100 (CPM_START_OF_TPA) instead, and quits the emulator if called again.
 *
 * Call this after the emulator's memory streams have been initialized (cpu.read_memory() & cpu.write_memory()).
 * Returns 1 on success, 0 on failure. */
I8080_CDECL int emu_set_cpm_env(i8080 * const cpu, int enable_cmd_proc);
/* Sets up the default emulator environment.
 * Creates an interrupt vector table at the top 64 bytes of memory,
 * and writes a RST 0 sequence that jumps to after the IVT (0x40, emu_consts.h/DEFAULT_START_PA). 
 * Call this after the emulator's memory streams have been initialized (cpu.read_memory() & cpu.write_memory()). 
 * Returns 1 on success, 0 on failure. */
I8080_CDECL int emu_set_default_env(i8080 * const cpu);

/* Begin the emulator. Must have properly set up memory and streams first!
 * Returns an error code if the emulator was not initialized properly.
 * 
 * If debug_args is not NULL, emulator will start in debug mode. In this mode, the emulator can print
 * the values of all flags and registers, and can dump the main memory after each instruction is executed to debug_out. 
 * See emu_debug_args to set options on how the output is presented. */
I8080_CDECL emu_exit_code_t emu_runtime(i8080 * const cpu, emu_debug_args * debug_args);

#include "i8080/internal/i8080_predef_undef.h"

#endif /* EMU_H */