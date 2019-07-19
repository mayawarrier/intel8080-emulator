#ifndef I_8080_H
#define I_8080_H

#include "../emu_types.h"
#include <stdbool.h>
#include <stdlib.h>

// Define types of read/write streams
typedef emu_word_t (* read_word_fp)(emu_addr_t);
typedef void (* write_word_fp)(emu_addr_t, emu_word_t);

typedef struct mem_t {
    // the memory space
    emu_word_t * mem;
    // the highest address in this memory space
    emu_addr_t highest_addr;
} mem_t;

typedef struct i8080 {   
    // registers
    emu_word_t a, b, c, d, e, h, l;
    // stack pointer, program counter
    emu_addr_t sp, pc;
    
    // flags: sign, zero, auxiliary carry,
    // carry, parity, interrupt enable
    bool s, z, acy, cy, p, ie;
    
    // True if in HALT state. Only interrupts
    // and RESET can bring i8080 out of this state. 
    bool is_halted;
    
    // provide your own read/write streams
    
    // Read and write to a memory stream
    read_word_fp read_memory;
    write_word_fp write_memory;
    
    // I/O stream in and out
    read_word_fp port_in;
    write_word_fp port_out;
    
    // Cycles taken for last emu_runtime
    size_t cycles_taken;
    
} i8080;

#endif // I_8080_H