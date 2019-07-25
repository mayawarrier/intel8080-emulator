/*
 * Implement emu_debug.h
 */

#include <stdlib.h>
#include <stdio.h>
#include "emu_debug.h"
#include "i8080/i8080.h"

static const char * DUMP_LOCATION = "dumps/memory_dump.bin";
static const char * CPU_STATS_DUMP_LOCATION = "dumps/cpu_stats_dump.bin";

void dump_memory(const emu_word_t * memory, emu_addr_t highest_addr) {
    
    FILE * dumpf_ptr = fopen(DUMP_LOCATION, "w");
    
    if (dumpf_ptr != NULL) {
        for (size_t i = 0; i <= (size_t)highest_addr; ++i) {
            fprintf(dumpf_ptr, WORD_T_FORMAT ",", memory[i]);
            // End-line every 8 bytes
            if ((i + 1) % 8 == 0) {
                fprintf(dumpf_ptr, "\n");
            }
        }
    }
}

void dump_cpu_stats(i8080 * const cpu) {
    
    FILE * dumpf_ptr = fopen(CPU_STATS_DUMP_LOCATION, "w");
    
    if (dumpf_ptr != NULL) {
        fprintf(dumpf_ptr, "A: " WORD_T_FORMAT "\n", cpu->a);
        fprintf(dumpf_ptr, "B: " WORD_T_FORMAT "\n", cpu->b);
        fprintf(dumpf_ptr, "C: " WORD_T_FORMAT "\n", cpu->c);
        fprintf(dumpf_ptr, "D: " WORD_T_FORMAT "\n", cpu->d);
        fprintf(dumpf_ptr, "E: " WORD_T_FORMAT "\n", cpu->e);
        fprintf(dumpf_ptr, "H: " WORD_T_FORMAT "\n", cpu->h);
        fprintf(dumpf_ptr, "L: " WORD_T_FORMAT "\n", cpu->l);
        fprintf(dumpf_ptr, "PC: " ADDR_T_FORMAT "\n", cpu->pc);
        fprintf(dumpf_ptr, "SP: " ADDR_T_FORMAT "\n", cpu->sp);
        fprintf(dumpf_ptr, "Zero: %d\n", cpu->z);
        fprintf(dumpf_ptr, "Sign: %d\n", cpu->s);
        fprintf(dumpf_ptr, "Parity: %d\n", cpu->p);
        fprintf(dumpf_ptr, "Auxiliary carry: %d\n", cpu->acy);
        fprintf(dumpf_ptr, "Carry: %d\n", cpu->cy);
        fprintf(dumpf_ptr, "Interrupt enable: %d\n", cpu->ie);
    }
}