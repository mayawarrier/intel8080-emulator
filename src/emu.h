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

/* Define the location of the machine types file relative to i8080.h.
 * It is necessary to define EMU_TYPES_LOC before including i8080.h.*/
#define EMU_TYPES_LOC "../emu_types.h"
#include "i8080/i8080.h"

typedef enum EMU_EXIT_CODES {
    EMU_ERR_MEM_STREAMS,        // A memory stream function (cpu->read_memory, cpu->write_memory) is not initialized.
    EMU_ERR_IO_STREAMS,         // An I/O request was made at runtime but an I/O stream function (cpu->port_in, cpu->port_out) was not initialized.
    EMU_ERR_MEMORY,             // A memory location is not read/writ-able. Location of failure stored in cpu->pc.
                                // This error can only be triggered by the startup check.
    EMU_SUCCESS                 // Successful run.
} EMU_EXIT_CODE;

/* The starting location of the CP/M Transient Program Area i.e. 
 * the first valid location to load a program written for CP/M. 
 * Set this environment with emu_set_cpm_env(). */
static const emu_addr_t CPM_START_OF_TPA = 0x100;
// Port address that CP/M will use to read/write characters from/to a console
static const emu_word_t CPM_CONSOLE_ADDR = 0x00;
/* The first valid location to load a program with the default 
 * environment. Set this environment with emu_set_default_env(). */
static const emu_addr_t DEFAULT_START_PA = 0x40;

// Allocates the largest amount of memory addressable (in this case 64KB)
_Bool memory_init(emu_word_t * const memory_buf);
// Loads a file into memory. Call only after memory_init(). Returns number of words read.
size_t memory_load(const char * file_loc, emu_word_t * memory, emu_addr_t start_loc);

// Initialize an i8080
void emu_init_i8080(i8080 * const cpu);

/* Sets up the environment for some basic CP/M 2.2 BIOS emulation.
 * Some programs written for the 8080 need this environment. At the moment, only 
 * supports CP/M BDOS ops 2 and 9, and WBOOT (which launches into a simple command processor).
 * The command processor has 2 commands:
 * RUN addr: Begins execution of a CP/M program starting at addr.
 * QUIT: Quits the emulator.
 * Call this after the emulator's memory streams have been initialized (cpu->read_memory & cpu->write_memory). */
void emu_set_cpm_env(i8080 * const cpu);
/* Sets up the default emulator environment.
 * Creates an interrupt vector table at the top 64 bytes of memory,
 * and writes a RST 0 sequence that jumps to after the IVT (0x40, DEFAULT_START_PA). 
 * Call this after the emulator's memory streams have been initialized (cpu->read_memory & cpu->write_memory). */
void emu_set_default_env(i8080 * const cpu);

// Begin the emulator. Must have properly set up memory and streams first!
// Returns an error code if the emulator was not initialized properly or failed the startup check.
EMU_EXIT_CODE emu_runtime(i8080 * const cpu, _Bool perform_startup_check);
/* Begins the emulator in debug mode. In this mode, the emulator prints the values of all flags and registers,
 * and dumps the main memory after each instruction is executed. */
EMU_EXIT_CODE emu_runtime(i8080 * const cpu, _Bool perform_startup_check, FILE * debug_out, const char dump_format[]);

// Send an interrupt (INTE) to the i8080. This can be sent on another thread
void emu_i8080_interrupt(i8080 * const cpu);

// Cleans up memory
void emu_cleanup(i8080 * cpu, emu_word_t * memory);

#endif /* EMU_H */