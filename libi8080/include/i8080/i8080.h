/*
 * Emulate an Intel 8080.
 *
 * All instructions (documented and undocumented) are emulated.
 */

#ifndef I8080_H
#define I8080_H

#include "i8080_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct i8080_monitor 
{
	/* If assigned, is called on RST 7.
	 * mexitcode is set to the return value. */
	int(*on_rst7)(struct i8080 *);
	/* If assigned, is called when
	 * cpu enters or exits halt. */
	void(*on_halt_changed)(struct i8080 *, int val);
};

struct i8080
{
	/* Registers */
	i8080_word_t a, b, c, d, e, h, l;
	/* Stack pointer, program counter */
	i8080_addr_t sp, pc;

	/* sign, zero, carry, aux carry, parity */
	int s, z, cy, acy, p;

	/* Interrupt enable, Interrupt pending */
	int inte, intr;

	/* Flag for halt/idle mode.
	 * Set by executing opcode HLT.
	 * No instructions execute until
	 * the next interrupt or RESET. */
	int halt;

	i8080_word_t(*memory_read)(const struct i8080 *, i8080_addr_t addr);
	void(*memory_write)(const struct i8080 *, i8080_addr_t addr, i8080_word_t word);

	/* called on opcode IN */
	i8080_word_t(*io_read)(const struct i8080 *, i8080_addr_t port);
	/* called on opcode OUT */
	void(*io_write)(const struct i8080 *, i8080_addr_t port, i8080_word_t word); 
	/* called on i8080_interrupt() */
	i8080_word_t(*interrupt_read)(const struct i8080 *);

	/* optional monitor/debugger */
	struct i8080_monitor *monitor; 

	/* Monitor exit code.
	 * Checked after io_read, io_write, interrupt_read
	 * and monitor->on_rst7. If non-zero, execution
	 * fails for one instruction cycle. */
	int mexitcode;

	/* Clock cycles since init */
	unsigned long cycles;

	/* User data */
	void *udata;
};

/*
 * First-time initialization.
 * - Sets intr, mexitcode, halt and cycles to 0.
 * - Sets io_read, io_write, interrupt_read
 *   and monitor to NULL.
 */
void i8080_init(struct i8080 *const cpu);

/*
 * Reset i8080.
 * Sets PC to 0, resets
 * interrupt enable, clears halt.
 * No other registers or flags are affected.
 * Has the same effect as the RESET pin.
 */
void i8080_reset(struct i8080 *const cpu);

/*
 * Execute the next instruction in
 * memory or service a pending interrupt.
 * Returns 0 if successful, else -1.
 */
int i8080_next(struct i8080 *const cpu);

/*
 * Execute an opcode.
 * Returns 0 if successful, else -1.
 */
int i8080_exec(struct i8080 *const cpu, i8080_word_t opcode);

/*
 * Send an interrupt to the i8080.
 * interrupt_read() will be called
 * on the next instruction cycle.
 * Clears halt if set.
 */
void i8080_interrupt(struct i8080 *const cpu);

#ifdef __cplusplus
}
#endif

#endif /* I8080_H */
