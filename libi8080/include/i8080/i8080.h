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

    unsigned s : 1;  /* Sign flag */
	unsigned z : 1;  /* Zero flag */
	unsigned cy : 1; /* Carry flag */
	unsigned ac : 1; /* Aux carry flag */
	unsigned p : 1;  /* Parity flag */

	unsigned inte : 1; /* Interrupt enable */
	unsigned intr : 1; /* Interrupt pending */

	/* True if in WAIT state. See opcode HLT. */
	/* Cleared by next interrupt or reset. */
	unsigned halt : 1;

	unsigned long /* long */ cycles; /* Clock cycles */

	/* ---------- user-defined ---------- */
	
	unsigned enable : 1; /* Enable/disable */

	i8080_word_t(*memory_read)(const struct i8080 *, i8080_addr_t addr);
	void(*memory_write)(const struct i8080 *, i8080_addr_t addr, i8080_word_t word);
	
    /* Handles opcode IN. */
	i8080_word_t(*io_read)(const struct i8080 *, i8080_addr_t port);
    /* Handles opcode OUT. */
	void(*io_write)(const struct i8080 *, i8080_addr_t port, i8080_word_t word); 

	/* Handles an interrupt. */
	i8080_word_t(*interrupt_read)(const struct i8080 *);

	/* User data */
	void *udata;
};

/* Reset chip (low on RESET pin). */
/* PC is set to 0, interrupts are */
/* disabled, halt is cleared, and */
/* cycles set to 0. */
void i8080_reset(struct i8080 *const cpu);

/* Execute the next instruction in */
/* memory or process a pending interrupt. */
/* Returns 0 on success, else -1. */
int i8080_next(struct i8080 *const cpu);

/* Send an interrupt. */
void i8080_interrupt(struct i8080 *const cpu);

#ifdef __cplusplus
}
#endif

#endif /* I8080_H */
