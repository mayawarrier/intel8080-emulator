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

// Resets the cpu. PC is set to 0. No other working registers or flags are affected.
void i8080_reset(i8080 * const cpu);

// Executes the next instruction. If an interrupt is pending, services it.
void i8080_next(i8080 * const cpu);

// Executes the opcode on cpu, updating its cycle count, registers and flags.
void i8080_exec(i8080 * const cpu, word_t opcode);

#endif /* I8080_INTERNAL_H */

