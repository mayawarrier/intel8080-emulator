/* 
 * File:   emu_debug.h
 * Author: dhruvwarrier
 *
 * Contains some functions to help debug.
 * 
 * Created on June 30, 2019, 8:24 PM
 */

#ifndef EMU_DEBUG_H
#define EMU_DEBUG_H

// Forces definition of EMU_TYPES_LOC before including i8080.h
#include "emu.h"
#include <stdio.h>

/* Dumps the contents of memory from start_addr to end_addr to a stream, formatting each word as format.
 * newline_after is the number of words after which an newline should be inserted each time. */
void dump_memory(FILE * stream, const char format[], int newline_after, emu_addr_t start_addr, emu_addr_t end_addr, i8080 * const cpu);
// Writes the values of all flags and registers to the given stream.
void dump_cpu_stats(FILE * stream, i8080 * const cpu);

// Set the options to be used with the next call to i8080_debug_next().
void set_debug_next_options(FILE * stream, const char mem_dump_format[], int mem_dump_newline_after);
/* Performs an i8080_next(), and prints debug information to the given stream:
 * - The instruction executed
 * - State of all registers and flags in the i8080
 * - Dump of the main memory, formatted as given format. A newline is inserted every mem_dump_newline_after words. */
_Bool i8080_debug_next(i8080 * const cpu);

#endif /* EMU_DEBUG_H */