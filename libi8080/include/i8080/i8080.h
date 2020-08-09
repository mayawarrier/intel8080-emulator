/*
 * Emulate an i8080.
 *
 * - All instructions supported - documented and undocumented.
 */

#ifndef I8080_H
#define I8080_H

#include "i8080_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct i8080_monitor;

struct i8080
{
	/* Registers */
	i8080_word_t a, b, c, d, e, h, l;
	/* Stack pointer, program counter */
	i8080_addr_t sp, pc;

	/* Status flags:
	 * sign, zero, auxiliary carry, carry, parity */
	int s, z, acy, cy, p;

	/* Interrupt enable. Set/cleared by IE/DE. */
	int inte;

	/* If in HALT state (i.e. stopped execution).
	 * Set by HLT. Can only be cleared by an interrupt/RESET. */
	int halt;

	/* Set by i8080_interrupt() to indicate pending
	 * interrupt request. Cleared when serviced. */
	int intr;

	/* io_read(), io_write() and interrupt_read()
	 * are optional. Set them to NULL if unused. */

	i8080_word_t(*memory_read)(const struct i8080 *, i8080_addr_t mem_addr);
	i8080_word_t(*io_read)(const struct i8080 *, i8080_addr_t port_addr);

	void(*memory_write)(const struct i8080 *, i8080_addr_t mem_addr, i8080_word_t word);
	void(*io_write)(const struct i8080 *, i8080_addr_t port_addr, i8080_word_t word);
	
	i8080_word_t(*interrupt_read)(const struct i8080 *);

	/* Clock cycles since init */
	unsigned long cycles;

	/* optional, see i8080_monitor */
	struct i8080_monitor *monitor;

	/* If set to non-zero value by io_read(),
	 * io_write(), interrupt_read() or i8080_monitor, 
	 * i8080_next()/i8080_exec() will exit with -1. */
	int exitcode;

	/* User data */
	void *udata;
};

struct i8080_monitor
{
	/* Attach a routine to be called on RST 7.
	 * exitcode is set to the return value. */
	int(*enter_monitor)(struct i8080 *);
};

/*
 * First-time initialization.
 * - Sets intr, exitcode and cycles to 0.
 * - Sets monitor to NULL.
 */
void i8080_init(struct i8080 *const cpu);

/*
 * Reset the CPU.
 * PC is set to 0, interrupt enable is reset.
 * If halted, leaves the halted state.
 * No other registers or flags are affected.
 * Equivalent to pulling RESET pin low.
 */
void i8080_reset(struct i8080 *const cpu);

/*
 * Execute the next instruction in memory.
 * If an interrupt is pending, services it.
 * Returns 0 if execution is successful, -1 if not.
 */
int i8080_next(struct i8080 *const cpu);

/*
 * Execute an opcode.
 * Updates cycle count, registers and flags.
 * Returns 0 if execution is successful, -1 if not.
 */
int i8080_exec(struct i8080 *const cpu, i8080_word_t opcode);

/*
 * Send an interrupt to the CPU.
 * i8080 will call interrupt_read() and execute the returned opcode.
 */
void i8080_interrupt(struct i8080 *const cpu);

#ifdef __cplusplus
}
#endif

#endif /* I8080_H */
