/*
 * Emulate an Intel 8080 microprocessor.
 * All opcodes (0x00-0xff) are supported.
 * 
 * Example usage:
 *
 * void run_8080(uint64_t num_clk_cycles) 
 * {
 *     i8080 cpu;
 *     cpu.mem_read = my_mem_read_cb;
 *     cpu.mem_write = my_mem_write_cb;
 *     cpu.io_read = my_io_read_cb;      // optional
 *     cpu.io_write = my_io_write_cb;    // optional
 *     cpu.intr_read = my_intr_read_cb;  // optional
 *     
 *     i8080_reset(&cpu);
 *     
 *     while (i8080_next(&cpu) == 0 && cpu.cycles < num_clk_cycles) {
 *         // some code
 *         // call i8080_interrupt(&cpu) here
 *     }
 *     printf("Done!");
 * }
 */

#ifndef I8080_H
#define I8080_H

#include "i8080_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct i8080
{
    /* Working registers */
    i8080_word_t a, b, c, d, e, h, l;

    i8080_addr_t sp; /* Stack pointer */
    i8080_addr_t pc; /* Program counter */

    i8080_word_t int_rq; /* Interrupt request */

    i8080_word_t s : 1;  /* Sign flag */
    i8080_word_t z : 1;  /* Zero flag */
    i8080_word_t cy : 1; /* Carry flag */
    i8080_word_t ac : 1; /* Aux carry flag */
    i8080_word_t p : 1;  /* Parity flag */

    i8080_word_t halt : 1; /* See HLT. */

    i8080_word_t int_en : 1; /* Interrupts enabled (INTE pin) */
    i8080_word_t int_ff : 1; /* Interrupt latch */

    /* Clock cycles */
    i8080_cycles_t cycles;

    /* ---------- user-defined ---------- */

    i8080_word_t(*mem_read)(const struct i8080*, i8080_addr_t addr);
    void(*mem_write)(const struct i8080*, i8080_addr_t addr, i8080_word_t word);

    /* Handles opcode IN. */
    i8080_word_t(*io_read)(const struct i8080*, i8080_word_t port);
    /* Handles opcode OUT. */
    void(*io_write)(const struct i8080*, i8080_word_t port, i8080_word_t word);

    /* Handles an interrupt. */
    i8080_word_t(*intr_read)(const struct i8080*);

    /* User data */
    void* udata;
};

enum i8080_err
{
    /* Missing I/O or interrupt handler. */
    i8080_EHNDLR = 1,
    /* Unrecognized opcode.
     * Only possible if opcode can be > 0xff. */
    i8080_EOPCODE = 2
};

/* Reset chip. Eq to low on RESET pin. */
void i8080_reset(struct i8080* const cpu);

/* Run one instruction. */
/* Returns 0 on success. */
int i8080_next(struct i8080* const cpu);

/* Send an interrupt request. */
/* If interrupts are enabled, intr_read() will be */
/* invoked by i8080_next() and the returned opcode */
/* will be executed. */
void i8080_interrupt(struct i8080* const cpu);

#ifdef __cplusplus
}
#endif

#endif /* I8080_H */
