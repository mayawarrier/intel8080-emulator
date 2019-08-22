/*
 * Implement emu_debug.h
 */

#include "emu_debug.h"
#include "i8080/i8080_opcodes.h"
#include <stddef.h>
#include <string.h>

void dump_memory(i8080 * const cpu, const emu_debug_args * args) {
    if (args->debug_out != NULL) {
        fprintf(args->debug_out, "**** Memory dump: ****\n");
        for (emu_addr_t i = args->mem_dump_start_addr; i <= args->mem_dump_end_addr; ++i) {
            fprintf(args->debug_out, args->mem_dump_format, cpu->read_memory(i));
            // End-line every newline_after words
            if ((i + 1) % args->mem_dump_newline_after == 0) {
                fprintf(args->debug_out, "\n");
            }
        }
    }
}

void dump_cpu_stats(i8080 * const cpu, FILE * const stream) {
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

// Debug args to be used with debug next
static emu_debug_args * DEBUG_ARGS;

/* i8080_debug_next() is expected to have the signature _Bool (*)(i8080 * const),
 * so the options have to be passed in separately. */
void set_debug_next_options(emu_debug_args * args) {
    // Check if mem_dump_format is a valid string and format specifier
    _Bool is_valid_str = 0;
    for (int i = 0; i < EMU_DEBUG_FMAX; ++i) {
        if (args->mem_dump_format[i] == '\0') {
            is_valid_str = 1;
        }
    }
    emu_word_t out_word;
    _Bool is_valid_format = (sscanf("0x38", args->mem_dump_format, &out_word) == 1);
    
    if (is_valid_str && is_valid_format) {
        DEBUG_ARGS = args;
    } else {
        printf("Error: Bad mem_dump_format.\n");
    }
}

_Bool i8080_debug_next(i8080 * const cpu) {
    // Get the opcode
    emu_mutex_lock(&cpu->i_mutex);
    emu_word_t opcode;
    if (cpu->ie && cpu->pending_interrupt_req && cpu->interrupt_acknowledge != NULL) {
        opcode = cpu->interrupt_acknowledge();
        cpu->ie = 0;
        cpu->pending_interrupt_req = 0;
        cpu->is_halted = 0;
    } else {
        if (!cpu->is_halted) {
            opcode = cpu->read_memory(cpu->pc++);
        }
    }
    emu_mutex_unlock(&cpu->i_mutex);
    
    _Bool success = 0;
    
    // Execute the instruction
    if (!cpu->is_halted) {
        success = i8080_exec(cpu, opcode);

        // Print disassembly
        printf("%s\n", DEBUG_DISASSEMBLY_TABLE[opcode]);
        // Dump cpu stats and main memory
        if (DEBUG_ARGS->should_dump_stats) {
            dump_cpu_stats(cpu, DEBUG_ARGS->debug_out);
        }
        if (DEBUG_ARGS->should_dump_memory) {
            dump_memory(cpu, DEBUG_ARGS);
        }
    } else {
        // indicate success but stay halted
        success = 1;
    }
    
    return success;
}