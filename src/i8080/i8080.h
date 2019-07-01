#ifndef I_8080_H
#define I_8080_H

#include "../emu_types.h"
#include <stdbool.h>

typedef struct mem_t {
    // the memory space
    word_t * mem;
    // the highest address in this memory space
    addr_t highest_addr;
    // how many bytes of this space is occupied by the program
    addr_t num_prog_bytes;
} mem_t;

typedef struct i8080 {   
    // registers
    word_t a, b, c, d, e, h, l;
    // stack pointer, program counter
    addr_t sp, pc;
    
    // flags: sign, zero, auxiliary carry,
    // carry, parity, interrupt enable
    bool s, z, acy, cy, p, ie;
    
    // Assign the memory before calling init!
    mem_t * memory;
    
} i8080;

#endif // I_8080_H