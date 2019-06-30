/*
 * Implement i8080 emulation.
 * 
 */

#include "i8080_internal.h"
#include "i8080_opcodes.h"

void i8080_init(i8080 * const cpu) {
    i8080_reset(cpu);
}

void i8080_reset(i8080 * const cpu) {
    // start executing from beginning again
    cpu->pc = 0;
}