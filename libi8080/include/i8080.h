/*
 * Emulate an i8080.
 *
 * - Supports all instructions, documented and undocumented.
 * - Supports asynchronous interrupts (given your build environment supports
 *   some form of thread synchronization - see i8080_interrupt_async() below)
 * 
 * Define I8080_DISABLE_ASYNC_INTERRUPTS before including this header to
 * disable async interrupts if necessary.
 */

#ifndef I8080_H
#define I8080_H

#include "i8080_types.h"
#ifdef I8080_MUTEX
    /* i8080_mutex_t is available */
    #undef I8080_MUTEX
    #define I8080_ASYNC_INTERRUPTS_AVAILABLE
#endif

#include "i8080_predef.h"

I8080_CDECL struct i8080 {
    /* Registers */
    i8080_word_t a, b, c, d, e, h, l;
    /* Stack pointer, program counter */
    i8080_addr_t sp, pc;

    /* Flags: sign, zero, auxiliary carry,
     * carry, parity, interrupt enable */
    int s, z, acy, cy, p, ie;

    /* True if i8080 is HALTed. Only interrupts
     * and RESET can bring it out of this state. */
    int is_halted;

    /* Read from a memory stream */
    i8080_read_word_handler read_memory;
    /* Write to a memory stream */
    i8080_write_word_handler write_memory;

    /* I/O stream in */
    i8080_read_word_handler port_in;
    /* I/O stream out */
    i8080_write_word_handler port_out;

    struct i8080_hardware {
        /* Called when the processor is ready to service
         * an interrupt request. Return the word to be 
         * executed next. */
        i8080_word_t(*interrupt_acknowledge)(void);

#ifdef I8080_ASYNC_INTERRUPTS_AVAILABLE
        /* 
         * If async interrupts are available, this is
         * used to sync with the interrupting thread.
         * See i8080_interrupt_async() below. 
         */
        struct intr_sync {
            int _intr_pending;
            i8080_mutex_t _intr_lock;
        } _intr_sync;
#endif
    } hardware;

    struct i8080_state {
        /* Last instruction executed. */
        i8080_word_t last_instr;
        /* Number of cycles taken since startup */
        i8080_uintmax_t cycle_count;
    } state;
};

/* 
 * Resets the i8080 and performs first time initialization. 
 * No working registers or flags are affected. 
 */
I8080_CDECL void i8080_init(struct i8080 * const cpu);

/* 
 * Resets the i8080. 
 * PC is set to 0, i8080 comes out of HALT, and cycles taken is reset to 0.
 * No working registers or flags are affected. 
 * Equivalent to pulling the RESET pin low. 
 */
I8080_CDECL void i8080_reset(struct i8080 * const cpu);

/* 
 * Executes the next instruction in memory.
 * If an interrupt is pending, services it.
 * Returns 0 if execution is successful. 
 */
I8080_CDECL int i8080_next(struct i8080 * const cpu);

/* 
 * Executes a given opcode.
 * Updates cycle count, registers and flags.
 * Returns 0 if execution is successful.
 */
I8080_CDECL int i8080_exec(struct i8080 * const cpu, i8080_word_t opcode);

/* 
 * Async interrupts are made available if your build environment supports some form
 * of thread synchronization. See i8080_sync.c for how this is implemented.
 * You can force async interrupts to be disabled by defining I8080_DISABLE_ASYNC_INTERRUPTS 
 * before including this header.
 */
#ifdef I8080_ASYNC_INTERRUPTS_AVAILABLE
/* 
 * Sends an interrupt asynchronously to the i8080 (i.e. is thread-safe).
 * When ready, the i8080 will call interrupt_acknowledge(), and execute the returned opcode.
 */
I8080_CDECL void i8080_interrupt(struct i8080 * const cpu);

/* 
 * Destroys any internal resources held by the OS for the _intr_lock, if any.
 */
I8080_CDECL void i8080_destroy(struct i8080 * const cpu);

#endif

/*
 * Sends an interrupt synchronously (i.e. is NOT thread-safe).
 * Use this if async interrupts are not available (see above).
 * The i8080 will immediately call interrupt_acknowledge(), and execute the returned opcode.
 */
I8080_CDECL void i8080_interrupt_sync(struct i8080 * const cpu);

#undef I8080_ASYNC_INTERRUPTS_AVAILABLE

/* 
 * Undef any macros used internally to prevent exporting them by default.
 * If you want access to these macros, include i8080_predef.h
 */
#undef I8080_WINDOWS
#undef I8080_UNIX
#undef I8080_NONSTD
#undef I8080_CDECL

#endif /* I8080_H */