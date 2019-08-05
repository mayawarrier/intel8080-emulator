/*
 * Implement emu_debug.h
 */

#include "emu_debug.h"
#include "i8080/i8080_opcodes.h"

static const char * DUMP_LOCATION = "dumps/memory_dump.bin";
static const char * CPU_STATS_DUMP_LOCATION = "dumps/cpu_stats_dump.bin";

void dump_memory(FILE * stream, const emu_word_t * memory, emu_addr_t highest_addr, const char format[]) {
    
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

_Bool i8080_debug_next(i8080 * const cpu, FILE * stream, const char mem_dump_format[]) {
    emu_word_t opcode = i8080_advance_read_word(cpu);
    if (opcode == EMU_EXT_CALL) {
        printf("Emulator external call.\n");
    } else {
        printf("%s\n", DEBUG_DISASSEMBLY_TABLE[opcode]);
    }
    return i8080_exec(cpu, opcode);
}