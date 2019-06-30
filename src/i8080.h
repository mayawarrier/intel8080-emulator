#ifndef I_8080_EMU_H
#define I_8080_EMU_H

#include "types.h"
#include <stdbool.h>

typedef struct i8080 {   
    // registers
    word_t a, b, c, d, e, h, l;
    // stack pointer, program counter
    addr_t sp, pc;
    
    // flags: sign, zero, auxiliary carry,
    // carry, parity, interrupt enable
    bool s, z, acy, cy, p, ie;
    
} i8080;

typedef struct i8080_mem {
    // the memory space
    word_t * mem;
    // the highest address in this memory space
    addr_t highest_addr;
    // how many bytes of this space is occupied by the program
    addr_t num_prog_bytes;
} i8080_mem;

// Initializes the CPU
void i8080_init(i8080 * const cpu);

#endif // I_8080_EMU_H