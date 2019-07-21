#ifndef I_8080_H
#define I_8080_H

#include "../emu_types.h"
#include <stdbool.h>
#include <stdlib.h>

// Define types of read/write streams
typedef emu_word_t (* read_word_fp)(emu_addr_t);
typedef void (* write_word_fp)(emu_addr_t, emu_word_t);

typedef struct emu_mem_t {
    // the memory space
    emu_word_t * mem;
    // the highest address in this memory space
    emu_addr_t highest_addr;
} emu_mem_t;

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
    
    /* This is called on opcode 0x38. 0x38
     * is actually an undocumented NOP, but is
     * re-purposed for this instead.
     * It pushes the return address to the stack. 
     * This fn is called with a ptr to the i8080.
     * It should return true if i8080 should
     * continue execution. */
    bool (* emu_ext_call)(void * const);
    // Records the last instruction executed.
    // Can be used to debug upon emu_ext_call.
    emu_word_t last_instr_exec;
    
    // Cycles taken for last emu_runtime
    size_t cycles_taken;
    
} i8080;

#endif // I_8080_H