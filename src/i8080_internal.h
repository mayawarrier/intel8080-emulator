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

// Resets the CPU and begins execution from 0x0
void i8080_init(i8080 * const cpu);
// Resets the CPU. PC is set to 0. No other working registers or flags are affected.
void i8080_reset(i8080 * const cpu);

#endif /* I8080_INTERNAL_H */

