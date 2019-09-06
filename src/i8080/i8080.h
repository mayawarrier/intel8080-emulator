/* 
 * File:   i8080.h
 * Author: dhruvwarrier
 *
 * Emulate an i8080.
 * 
 * Created on June 30, 2019, 5:28 PM
 */

#ifndef I_8080_H
#define I_8080_H

#include "internal/i8080_types.h"
#include "internal/i8080_predef.h"

I8080_CDECL typedef struct i8080 {
    // Registers
    emu_word_t a, b, c, d, e, h, l;
    // Stack pointer, program counter
    emu_addr_t sp, pc;
    
    // Flags: sign, zero, auxiliary carry,
    // carry, parity, interrupt enable
    int s, z, acy, cy, p, ie;
    
    // True if in HALT state. Only interrupts
    // and RESET can bring i8080 out of this state. 
    int is_halted;
    
    // provide your own read/write streams
    
    // Read from a memory stream
    emu_read_word_fp read_memory;
    // Write to a memory stream
    emu_write_word_fp write_memory;
    
    // I/O stream in
    emu_read_word_fp port_in;
    // I/O stream out
    emu_write_word_fp port_out;
    
    /* This is called on opcode 0x38. 0x38
     * is actually an undocumented NOP, but is
     * re-purposed for this instead.
     * It pushes the return address to the stack,
     * and provides a reference to the i8080.
     * Return true if i8080 should continue 
     * execution after this call. */
    int (* emu_ext_call)(void * const);
    // Records the last instruction executed.
    // Can be used to debug upon emu_ext_call.
    emu_word_t last_instr_exec;
    
    /* When an interrupt is issued, this
     * is called back when the i8080 is ready
     * to receive the interrupt vector. After
     * the vector has been executed, interrupts 
     * are disabled again. */
    emu_word_t (* interrupt_acknowledge)(void);
    // Whether there is a pending interrupt request to service.
    int pending_interrupt_req;
    
    /* When an interrupt comes on another thread,
     * this mutex is used to sync i8080 status
     * with the interrupt generator, so the
     * interrupt is not accidentally double-serviced
     * or missed by the i8080. */
	i8080_mutex_t i_mutex;
    
    // Cycles taken for last emu_runtime
    emu_large_t cycles_taken;
    
} i8080;

/* Resets the i8080, and performs first time initialization. */
I8080_CDECL void i8080_init(i8080 * const cpu);

/* Resets the i8080. PC is set to 0, i8080 exits halt state, and cycles taken is reset to 0. 
 * No other working registers or flags are affected. Equivalent to pulling RESET low. */
I8080_CDECL void i8080_reset(i8080 * const cpu);

/* Executes the next instruction. If an interrupt is pending, services it.
 * Returns 0 if it isn't safe to continue execution. */
I8080_CDECL int i8080_next(i8080 * const cpu);

/* Executes the opcode on cpu, updating its cycle count, registers and flags.
 * Returns 0 if it isn't safe to continue execution. */
I8080_CDECL int i8080_exec(i8080 * const cpu, emu_word_t opcode);

/* Sends an interrupt to the i8080. This is thread-safe, and it will be serviced when the i8080 is ready. 
 * If your compiler/environment does not support mutexes, this may not work correctly. See i8080.c for implementation.
 * 
 * When ready, the i8080 will call interrupt_acknowledge() which should return the vector to be executed. 
 * Interrupts are disabled every time an interrupt is serviced, so they must be enabled again before the next interrupt. */
I8080_CDECL void i8080_interrupt(i8080 * const cpu);

#include "internal/i8080_predef_undef.h"

#endif /* I_8080_H */