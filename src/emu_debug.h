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

#include "emu_types.h"

// Dumps the contents of memory to file
void dump_memory(const word_t * memory, addr_t highest_addr);

#endif /* EMU_DEBUG_H */