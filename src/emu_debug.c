/*
 * Implement emu_debug.h
 */

#include <stddef.h>
#include <string.h>
#include "emu_debug.h"
#include "i8080/i8080_opcodes.h"

void dump_memory(FILE * stream, const char format[], int newline_after, emu_addr_t start_addr, emu_addr_t end_addr, i8080 * const cpu) {
    if (stream != NULL) {
        fprintf(stream, "**** Memory dump: ****\n");
        for (emu_addr_t i = start_addr; i <= end_addr; ++i) {
            fprintf(stream, format, cpu->read_memory(i));
            // End-line every newline_after words
            if ((i + 1) % newline_after == 0) {
                fprintf(stream, "\n");
            }
        }
    }
}

void dump_cpu_stats(FILE * stream, i8080 * const cpu) {
    if (stream != NULL) {
        fprintf(stream, "**** CPU status: ****\n");
        fprintf(stream, "A: " WORD_T_FORMAT ", ", cpu->a);
        fprintf(stream, "B: " WORD_T_FORMAT ", ", cpu->b);
        fprintf(stream, "C: " WORD_T_FORMAT ",\n", cpu->c);
        fprintf(stream, "D: " WORD_T_FORMAT ", ", cpu->d);
        fprintf(stream, "E: " WORD_T_FORMAT ", ", cpu->e);
        fprintf(stream, "H: " WORD_T_FORMAT ", ", cpu->h);
        fprintf(stream, "L: " WORD_T_FORMAT ",\n", cpu->l);
        fprintf(stream, "PC: " ADDR_T_FORMAT ", ", cpu->pc);
        fprintf(stream, "SP: " ADDR_T_FORMAT "\n", cpu->sp);
        fprintf(stream, "Zero: %d, ", cpu->z);
        fprintf(stream, "Sign: %d, ", cpu->s);
        fprintf(stream, "Parity: %d,\n", cpu->p);
        fprintf(stream, "Auxiliary carry: %d, ", cpu->acy);
        fprintf(stream, "Carry: %d, ", cpu->cy);
        fprintf(stream, "Interrupt enable: %d\n", cpu->ie);
    }
}

// Debug next options, set to default values
static FILE * DEBUG_OUT = NULL;
static const int DUMPF_BUF_SIZE = 128;
static char MEM_DUMPF_BUF[DUMPF_BUF_SIZE] = WORD_T_FORMAT;
static int MEM_NEWLINE_AFTER = 16;

/* i8080_debug_next() is expected to have the signature _Bool (*)(i8080 * const),
 * so the options have to be passed in separately. */
void set_debug_next_options(FILE * stream, const char mem_dump_format[], int mem_dump_newline_after) {
    // Get size of string along with trailing NULL
    size_t format_size = sizeof(mem_dump_format);
    if (format_size > DUMPF_BUF_SIZE) {
        printf("Error: Debug next format too large.\n");
    } else {
        DEBUG_OUT = stream;
        strncpy(MEM_DUMPF_BUF, mem_dump_format, format_size);
        MEM_NEWLINE_AFTER = mem_dump_newline_after;
    }
}

_Bool i8080_debug_next(i8080 * const cpu) {
    // Print the instruction
    emu_word_t opcode = cpu->read_memory(cpu->pc++);
    printf("%s\n", DEBUG_DISASSEMBLY_TABLE[opcode]);
    // execute the instruction
    _Bool exec = i8080_exec(cpu, opcode);
    // Dump stats and memory
    dump_cpu_stats(DEBUG_OUT, cpu);
    dump_memory(DEBUG_OUT, MEM_DUMPF_BUF, MEM_NEWLINE_AFTER, cpu);
    return exec;
}