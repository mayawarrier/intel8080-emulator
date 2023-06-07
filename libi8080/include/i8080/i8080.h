/*
 * Emulate an Intel 8080 microprocessor.
 *
 * All opcodes (0x00-0xff) are supported.
 */

#ifndef I8080_H
#define I8080_H

#include "i8080_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct i8080
{
    /* Working register */
    i8080_word_t a, b, c, d, e, h, l;

    i8080_addr_t sp; /* Stack pointer */
    i8080_addr_t pc; /* Program counter */

    i8080_word_t s : 1;  /* Sign flag */
    i8080_word_t z : 1;  /* Zero flag */
    i8080_word_t cy : 1; /* Carry flag */
    i8080_word_t ac : 1; /* Aux carry flag */
    i8080_word_t p : 1;  /* Parity flag */

    i8080_word_t inte : 1; /* Interrupt enable */
    i8080_word_t intr : 1; /* Interrupt pending */
    i8080_word_t halt : 1; /* See HLT. */

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

/* Reset chip. */
/* PC <= 0, interrupts are disabled, */
/* halt cleared, and cycles set to 0. */
void i8080_reset(struct i8080* const cpu);

/* Run the next instruction in memory */
/* or service a pending interrupt. */
/* Returns 0 on success. */
int i8080_next(struct i8080* const cpu);

/* Send an interrupt. */
/* If interrupts are enabled, intr_read() is called */
/* shortly after to acknowledge the interrupt.*/
void i8080_interrupt(struct i8080* const cpu);

#ifdef __cplusplus
}
#endif

#endif /* I8080_H */
