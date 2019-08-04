/* 
 * File:   i8080_internal.h
 * Author: dhruvwarrier
 * 
 * For emulator internal use.
 *
 * Created on June 30, 2019, 6:04 PM
 */

#ifndef I8080_INTERNAL_H
#define I8080_INTERNAL_H

#include "i8080.h"

// Reserved locations for interrupt vector table
static const emu_addr_t INTERRUPT_TABLE[] = {
    0x00, // RESET, RST 0
    0x08, // RST 1
    0x10, // RST 2
    0x18, // RST 3
    0x20, // RST 4
    0x28, // RST 5
    0x30, // RST 6
    0x38 // RST 7
};

static const int NUM_IVT_VECTORS = 8;

// Resets the cpu. PC is set to 0. No other working registers or flags are affected.
void i8080_reset(i8080 * const cpu);

// Executes the next instruction. If an interrupt is pending, services it.
// Returns 0 if it isn't safe to continue execution.
_Bool i8080_next(i8080 * const cpu);

// i8080_next(), but also pretty prints the instr executed.
_Bool i8080_debug_next(i8080 * const cpu);

// Executes the opcode on cpu, updating its cycle count, registers and flags.
// Returns 0 if it isn't safe to continue execution.
_Bool i8080_exec(i8080 * const cpu, emu_word_t opcode);

#endif /* I8080_INTERNAL_H */

