#ifndef I_8080_EMU_H
#define I_8080_EMU_H

#include <stdint.h>
#include <stdbool.h>

// highest memory address allowed
#define HIGHEST_ADDR 65535

// abbreviate these types
typedef uint8_t u8;
typedef uint16_t u16;

typedef struct i8080 {   
    // registers
    u8 a, b, c, d, e, h, l;
    // stack pointer, program counter
    u16 sp, pc;
    
    // flags: sign, zero, auxiliary carry,
    // carry, parity, interrupt enable
    bool s, z, acy, cy, p, ie;
    
} i8080;

typedef struct i8080_mem {
    // the memory space
    u8 * mem;
    // the highest address in this memory space
    u16 highest_addr;
    // how many bytes of this space is occupied by the program
    u16 num_prog_bytes;
} i8080_mem;

// Initializes the CPU
void i8080_init(i8080 * const cpu);

#endif // I_8080_EMU_H