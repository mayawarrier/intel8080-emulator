/*
 * Implement emu_debug.h
 */

#include "i8080_types.h"
#include "i8080_opcodes.h"
#include "i8080_consts.h"
#include "i8080_sync.h"
#include "emu_debug.h"
#include <stddef.h>
#include <string.h>

/* Enable compilation on C89 */
#include "i8080_predef.h"

 /* This disassembly table sourced from (suitably modified):
  * https://github.com/superzazu/8080/blob/master/i8080.c
  * Modifications made: Changed all "ill" to "undocumented", replaced 0x38 with "Emulator external call" */

  /* Pretty print disassembly, indexed by opcode
   * e.g. DEBUG_DISASSEMBLY_TABLE[RST_7] */
static const char * const DEBUG_DISASSEMBLY_TABLE[] = {
    "nop", "lxi b,#", "stax b", "inx b", "inr b", "dcr b", "mvi b,#", "rlc",
    "undocumented", "dad b", "ldax b", "dcx b", "inr c", "dcr c", "mvi c,#", "rrc",
    "undocumented", "lxi d,#", "stax d", "inx d", "inr d", "dcr d", "mvi d,#", "ral",
    "undocumented", "dad d", "ldax d", "dcx d", "inr e", "dcr e", "mvi e,#", "rar",
    "undocumented", "lxi h,#", "shld", "inx h", "inr h", "dcr h", "mvi h,#", "daa",
    "undocumented", "dad h", "lhld", "dcx h", "inr l", "dcr l", "mvi l,#", "cma",
    "undocumented", "lxi sp,#","sta $", "inx sp", "inr M", "dcr M", "mvi M,#", "stc",
    "Emulator external call", "dad sp", "lda $", "dcx sp", "inr a", "dcr a", "mvi a,#", "cmc",
    "mov b,b", "mov b,c", "mov b,d", "mov b,e", "mov b,h", "mov b,l",
    "mov b,M", "mov b,a", "mov c,b", "mov c,c", "mov c,d", "mov c,e",
    "mov c,h", "mov c,l", "mov c,M", "mov c,a", "mov d,b", "mov d,c",
    "mov d,d", "mov d,e", "mov d,h", "mov d,l", "mov d,M", "mov d,a",
    "mov e,b", "mov e,c", "mov e,d", "mov e,e", "mov e,h", "mov e,l",
    "mov e,M", "mov e,a", "mov h,b", "mov h,c", "mov h,d", "mov h,e",
    "mov h,h", "mov h,l", "mov h,M", "mov h,a", "mov l,b", "mov l,c",
    "mov l,d", "mov l,e", "mov l,h", "mov l,l", "mov l,M", "mov l,a",
    "mov M,b", "mov M,c", "mov M,d", "mov M,e", "mov M,h", "mov M,l", "hlt",
    "mov M,a", "mov a,b", "mov a,c", "mov a,d", "mov a,e", "mov a,h",
    "mov a,l", "mov a,M", "mov a,a", "add b", "add c", "add d", "add e",
    "add h", "add l", "add M", "add a", "adc b", "adc c", "adc d", "adc e",
    "adc h", "adc l", "adc M", "adc a", "sub b", "sub c", "sub d", "sub e",
    "sub h", "sub l", "sub M", "sub a", "sbb b", "sbb c", "sbb d", "sbb e",
    "sbb h", "sbb l", "sbb M", "sbb a", "ana b", "ana c", "ana d", "ana e",
    "ana h", "ana l", "ana M", "ana a", "xra b", "xra c", "xra d", "xra e",
    "xra h", "xra l", "xra M", "xra a", "ora b", "ora c", "ora d", "ora e",
    "ora h", "ora l", "ora M", "ora a", "cmp b", "cmp c", "cmp d", "cmp e",
    "cmp h", "cmp l", "cmp M", "cmp a", "rnz", "pop b", "jnz $", "jmp $",
    "cnz $", "push b", "adi #", "rst 0", "rz", "ret", "jz $", "undocumented", "cz $",
    "call $", "aci #", "rst 1", "rnc", "pop d", "jnc $", "out p", "cnc $",
    "push d", "sui #", "rst 2", "rc", "undocumented", "jc $", "in p", "cc $", "undocumented",
    "sbi #", "rst 3", "rpo", "pop h", "jpo $", "xthl", "cpo $", "push h",
    "ani #", "rst 4", "rpe", "pchl", "jpe $", "xchg", "cpe $", "undocumented", "xri #",
    "rst 5", "rp", "pop psw", "jp $", "di", "cp $", "push psw","ori #",
    "rst 6", "rm", "sphl", "jm $", "ei", "cm $", "undocumented", "cpi #", "rst 7"
};

/* Checks if format spec has a max of 128 chars, and can be applied to an i8080_word_t. */
static int is_valid_format_spec(const char * f) {
    /* Check if f is a valid string and format specifier */
    int is_valid_str = 0;
    /* Maximum size is 128, including trailing null */
    size_t i;
    for (i = 0; i < 128; ++i) {
        if (f[i] == '\0') {
            is_valid_str = 1;
            break;
        }
    }
    /* check if this format can actually be applied to a word */
    i8080_word_t out_word;
    int is_valid_format = (sscanf("0x38", f, &out_word) == 1);

    return is_valid_str && is_valid_format;
}

static inline int in_range(size_t num, size_t lt_bound, size_t rt_bound, int inclusive) {
    if (inclusive) return (num >= lt_bound && num <= rt_bound);
    else return (num > lt_bound && num < rt_bound);
}

static inline int is_valid_args(const emu_debug_args * args) {
    return (args->debug_out != NULL &&
        is_valid_format_spec(args->mem_dump_format) &&
        in_range(args->mem_dump_start_addr, 0, ADDR_T_MAX, 1) &&
        in_range(args->mem_dump_end_addr, 0, ADDR_T_MAX, 1) &&
        args->mem_dump_end_addr >= args->mem_dump_start_addr &&
        in_range(args->mem_dump_newline_after, 1, ADDR_T_MAX, 1) &&
        (args->should_dump_memory == 1 || args->should_dump_memory == 0) &&
        (args->should_dump_stats == 1 || args->should_dump_stats == 0));
}

/* Dump memory without validation checks */
static void dump_memory_raw(i8080 * const cpu, const emu_debug_args * args) {
    fprintf(args->debug_out, "**** Memory dump: ****\n");
    i8080_addr_t i;
    for (i = args->mem_dump_start_addr; i <= args->mem_dump_end_addr; ++i) {
        fprintf(args->debug_out, args->mem_dump_format, cpu->read_memory(i));
        /* End-line every newline_after words */
        if ((i + 1) % args->mem_dump_newline_after == 0) {
            fprintf(args->debug_out, "\n");
        }
    }
}

void dump_memory(i8080 * const cpu, const emu_debug_args * args) {
    if (is_valid_args(args)) dump_memory_raw(cpu, args);
}

void dump_cpu_stats(i8080 * const cpu, FILE * const stream) {
    if (stream != NULL) {
        fprintf(stream, "**** CPU status: ****\n");
        fprintf(stream, "A: " WORD_T_PRT_FORMAT ", ", cpu->a);
        fprintf(stream, "B: " WORD_T_PRT_FORMAT ", ", cpu->b);
        fprintf(stream, "C: " WORD_T_PRT_FORMAT ",\n", cpu->c);
        fprintf(stream, "D: " WORD_T_PRT_FORMAT ", ", cpu->d);
        fprintf(stream, "E: " WORD_T_PRT_FORMAT ", ", cpu->e);
        fprintf(stream, "H: " WORD_T_PRT_FORMAT ", ", cpu->h);
        fprintf(stream, "L: " WORD_T_PRT_FORMAT ",\n", cpu->l);
        fprintf(stream, "PC: " ADDR_T_PRT_FORMAT ", ", cpu->pc);
        fprintf(stream, "SP: " ADDR_T_PRT_FORMAT "\n", cpu->sp);
        fprintf(stream, "Zero: %d, ", cpu->z);
        fprintf(stream, "Sign: %d, ", cpu->s);
        fprintf(stream, "Parity: %d,\n", cpu->p);
        fprintf(stream, "Auxiliary carry: %d, ", cpu->acy);
        fprintf(stream, "Carry: %d, ", cpu->cy);
        fprintf(stream, "Interrupt enable: %d\n", cpu->ie);
    }
}

/* Debug args to be used with debug next */
static emu_debug_args * DEBUG_ARGS;

/* i8080_debug_next() is expected to have the signature int (*)(i8080 * const),
 * so the options have to be passed in separately. */
void set_debug_next_options(emu_debug_args * args) {
    if (is_valid_args(args)) DEBUG_ARGS = args;
    else printf("Error: Bad mem_dump_format.\n");
}

unsigned i8080_debug_next(i8080 * const cpu) {
    i8080_mutex_lock(&cpu->i_mutex);
    i8080_word_t opcode = 0;
    if (cpu->ie && cpu->pending_interrupt_req && cpu->interrupt_acknowledge != NULL) {
        opcode = cpu->interrupt_acknowledge();
        cpu->ie = 0;
        cpu->pending_interrupt_req = 0;
        cpu->is_halted = 0;
    }
    else if (!cpu->is_halted) {
        opcode = cpu->read_memory(cpu->pc++);
    }
    i8080_mutex_unlock(&cpu->i_mutex);

    /* Execute opcode */
    unsigned success = 0;
    if (!cpu->is_halted) {
        success = i8080_exec(cpu, opcode);
        /* Print disassembly */
        printf("%s\n", DEBUG_DISASSEMBLY_TABLE[opcode]);
        /* Dump cpu stats and main memory */
        if (DEBUG_ARGS->should_dump_stats) dump_cpu_stats(cpu, DEBUG_ARGS->debug_out);
        if (DEBUG_ARGS->should_dump_memory) dump_memory_raw(cpu, DEBUG_ARGS);
    }
    else {
        /* indicate success but stay halted */
        success = 1;
    }
    return success;
}