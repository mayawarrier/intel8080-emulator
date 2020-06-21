/*
 * Emulate an i8080.
 *
 * - Emulates all instructions, documented and undocumented.
 * - External interrupts are thread-safe if build env supports it
	 (see i8080_intr_lock_create() and i8080_intr_lock_destroy()).
 */

#if defined(_MSC_VER) && (_MSC_VER > 1000)
#pragma once
#endif

#ifndef I8080_H
#define I8080_H

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * I8080_WORD = at least 8 bits 
 * I8080_DBL_WORD = at least 16 bits
 * MSVC doesn't consistently support chars beyond 
 * version 1300, so fall back on Microsoft types instead.
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
    #define I8080_WORD unsigned __int8
    #define I8080_DBL_WORD unsigned __int16
#else
    #define I8080_WORD unsigned char
    #define I8080_DBL_WORD unsigned int
#endif

typedef I8080_WORD i8080_word_t;
typedef I8080_DBL_WORD i8080_addr_t;
typedef I8080_DBL_WORD i8080_dbl_word_t;

#undef I8080_WORD
#undef I8080_DBL_WORD

struct i8080_mutex;

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
	 * Set by HLT. Can only be cleared by an external interrupt.  */
    int halt;

    i8080_word_t(*memory_read)(i8080_addr_t mem_addr);
    i8080_word_t    (*io_read)(i8080_addr_t port_addr);

    void(*memory_write)(i8080_addr_t mem_addr, i8080_word_t word);
    void    (*io_write)(i8080_addr_t port_addr, i8080_word_t word);

    i8080_word_t(*interrupt_read)(void);

	/* Set by i8080_interrupt() to indicate pending
	 * interrupt request. Cleared when serviced. */
	int intr;
	/* Synchronize with interrupt thread.
	 * See i8080_intr_lock_create() and i8080_intr_lock_destroy(). */
	struct i8080_mutex intr_lock;

	/* Clock cycles since reset */
	unsigned long cycles;

    /* User data */
    void *udata;
};

/*
 * Resets the CPU. 
 * PC is set to 0, CPU comes out of HALT, cycles is reset.
 * No working registers or flags are affected.
 * Equivalent to pulling the RESET pin low.
 */
void i8080_reset(struct i8080 *const cpu);

/*
 * Executes the next instruction in memory.
 * If an interrupt is pending, services it.
 * Returns 0 if execution is successful. 
 */
int i8080_next(struct i8080 *const cpu);

/*
 * Executes a given opcode.
 * Updates cycle count, registers and flags.
 * Returns 0 if execution is successful.
 */
int i8080_exec(struct i8080 *const cpu, i8080_word_t opcode);

/*
 * Creates i8080::intr_lock.
 * Returns 0 if successful, -1 if no lock primitive could be found.
 */
int i8080_intr_lock_create(struct i8080 *const cpu);

/*
 * Destroys i8080::intr_lock.
 */
void i8080_intr_lock_destroy(struct i8080 *const cpu);

/*
 * Sends an interrupt to the CPU.
 * Is thread-safe if a lock primitive is available. See i8080::intr_lock.
 * When ready, i8080 will call interrupt_read() and execute the returned opcode.
 */
void i8080_interrupt(struct i8080 *const cpu);

#ifdef __cplusplus
}
#endif

#endif /* I8080_H */
