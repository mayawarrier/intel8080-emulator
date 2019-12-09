/*
 * Emulate an i8080.
 *
 * - Supports all instructions, documented and undocumented.
 * - Supports asynchronous interrupts (given that the build environment supports mutexes or 
 *   atomic synchronization, see i8080_sync.c)
 * - Instruction 0x38 (undocumented NOP) is repurposed to act as a debugging/inspection opcode. 
 *   Upon hitting 0x38, the PC is pushed to the stack, and i8080.emu_ext_call() is called with a
 *   pointer to the i8080 instance.
 */

#ifndef I_8080_H
#define I_8080_H

#include "i8080_types.h"
#include "i8080_predef.h"

I8080_CDECL typedef struct i8080 {
    /* Registers */
    i8080_word_t a, b, c, d, e, h, l;
    /* Stack pointer, program counter */
    i8080_addr_t sp, pc;

    /* Flags: sign, zero, auxiliary carry,
     * carry, parity, interrupt enable */
    unsigned s, z, acy, cy, p, ie;

    /* True if i8080 is HALTed. Only interrupts
     * and RESET can bring it out of this state. */
    unsigned is_halted;

    /* Read from a memory stream */
    i8080_read_word_fp read_memory;
    /* Write to a memory stream */
    i8080_write_word_fp write_memory;

    /* I/O stream in */
    i8080_read_word_fp port_in;
    /* I/O stream out */
    i8080_write_word_fp port_out;

    struct i8080_hardware {
        /* Called when the i8080 is ready to
         * to service the interrupt. Should return
         * the word to be executed next. */
        i8080_word_t(*interrupt_acknowledge)(void);
        /* If a pending interrupt request exists. */
        unsigned interrupt_pending;
        /* Spin-lock to synchronize access to
         * interrupt critical variables. */
        i8080_mutex_t interrupt_lock;
    } hardware;

    /* Last instruction successfully executed. */
    i8080_word_t last_instr;
    /* Cycles taken for last emu_runtime */
    i8080_uintmax_t cycle_count;

    /* User data */
    void * udata;
} i8080;

/* Resets the i8080, and performs first time initialization. */
I8080_CDECL void i8080_init(i8080 * const cpu);

/* Destroys any internal resources held by the OS for i8080.i_mutex. */
I8080_CDECL void i8080_destroy(i8080 * const cpu);

/* Resets the i8080. PC is set to 0, i8080 exits halt state, and cycles taken is reset to 0.
 * No other working registers or flags are affected. Equivalent to pulling RESET low. */
I8080_CDECL void i8080_reset(i8080 * const cpu);

/* Executes the next instruction. If an interrupt is pending, services it.
 * Returns 0 if it isn't safe to continue execution. */
I8080_CDECL unsigned i8080_next(i8080 * const cpu);

/* Executes the opcode on cpu, updating its cycle count, registers and flags.
 * Returns 0 if it isn't safe to continue execution. */
I8080_CDECL unsigned i8080_exec(i8080 * const cpu, i8080_word_t opcode);

/* Sends an interrupt to the i8080. This is thread-safe, and it will be serviced when the i8080 is ready.
 * If your compiler/environment does not support mutexes, this may not work correctly. See i8080_sync.c for implementation.
 *
 * When ready, the i8080 will call interrupt_acknowledge() which should return the vector to be executed.
 * Interrupts are disabled every time an interrupt is serviced, so they must be enabled again before the next interrupt. */
I8080_CDECL void i8080_interrupt(i8080 * const cpu);

#include "i8080_predef_undef.h"

#endif /* I_8080_H */