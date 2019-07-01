/*
 * Implement emu_debug.h
 */

#include "emu_debug.h"
#include <stdlib.h>
#include <stdio.h>

static const char * DUMP_LOCATION = "memory_dump.bin";

void dump_memory(const word_t * memory, addr_t highest_addr) {
    
    FILE * dumpf_ptr = fopen(DUMP_LOCATION, "w");
    
    if (dumpf_ptr != NULL) {
        for (size_t i = 0; i <= (size_t)highest_addr; ++i) {
            fprintf(dumpf_ptr, WORD_T_FORMAT "\t", memory[i]);
        }
    }
}
