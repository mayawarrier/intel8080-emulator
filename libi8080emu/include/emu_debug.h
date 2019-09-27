/* 
 * Functions to debug emulator state.
 */

#ifndef EMU_DEBUG_H
#define EMU_DEBUG_H

#include "emu.h"
#include <stdio.h>
#include "i8080_predef.h"

/* Dumps the contents of memory from start_addr to end_addr to a stream, formatting each word as format.
 * mem_dump_newline_after is the number of words after which an newline should be inserted each time. */
I8080_CDECL void dump_memory(i8080 * const cpu, const emu_debug_args * args);
// Writes the values of all flags and registers to the given stream.
I8080_CDECL void dump_cpu_stats(i8080 * const cpu, FILE * const stream);

// Set the options to be used with the next call to i8080_debug_next().
I8080_CDECL void set_debug_next_options(emu_debug_args * args);
/* Performs an i8080_next(), and prints debug information to the given stream:
 * - The instruction executed
 * - State of all registers and flags in the i8080
 * - Dump of the main memory, formatted as mem_dump_format. A newline is inserted every mem_dump_newline_after words. */
I8080_CDECL int i8080_debug_next(i8080 * const cpu);

#include "i8080_predef_undef.h"

#endif /* EMU_DEBUG_H */