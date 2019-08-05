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

// Dumps the contents of memory to file
void dump_memory(const emu_word_t * memory, emu_addr_t highest_addr);
// Dumps the values of all registers and flags
void dump_cpu_stats(i8080 * const cpu);

/* Performs an i8080_next(), and prints debug information to the given stream:
 * - The instruction executed
 * - State of all registers and flags in the i8080
 * - Dump of the main memory, formatted as given format. */
_Bool i8080_debug_next(i8080 * const cpu, FILE * stream, const char mem_dump_format[]);

#endif /* EMU_DEBUG_H */