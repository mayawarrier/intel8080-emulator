/* 
 * File:   i8080.h
 * Author: dhruvwarrier
 *
 * Defines the emulated i8080, and provides fns to execute instructions on it.
 * EMU_TYPES_LOC must be defined before including this. See below for requirements.
 * 
 * Created on June 30, 2019, 5:28 PM
 */

#ifndef I_8080_H
#define I_8080_H

/* Location of user-configurable file that contains emulator base types, format specifiers, sizes etc.
 * 
 * In this file, the following types must be defined:
 * emu_word_t: An unsigned type with a bit width equal to the virtual machine's word size.
 * emu_addr_t: An unsigned type with a bit width equal to the virtual machine's address size.
 * emu_buf_t: An unsigned type that is at least 1 bit larger than emu_word_t and emu_addr_t.
 * emu_size_t: An unsigned large type. Used to count processor cycles taken. If this is not large enough,
 *             the emulator will still run properly, but the cycles_taken might overflow.
 * 
 * In this file, the following macros must be defined:
 * HALF_WORD_SIZE: uint, half the bit width of emu_word_t for eg. (4)
 * HALF_ADDR_SIZE: uint, half the bit width of emu_addr_t
 * WORD_T_MAX: uint, max value that can be held by emu_word_t for eg. (255) 
 * ADDR_T_MAX: uint, max value that can be held by emu_addr_t
 * WORD_T_FORMAT: quote-enclosed string, hexadecimal format specifier for emu_word_t. for eg. "%#04x"
 * ADDR_T_FORMAT: quote-enclosed string, hexadecimal format specifier for emu_addr_t.
 * WORD_T_PRT_FORMAT: quote-enclosed string, format specifier to print word as ASCII eg. "%c"
 */
#ifdef EMU_TYPES_LOC
#include EMU_TYPES_LOC
#else
// If EMU_TYPES_LOC doesn't exist, attempt to find it in its default location
#include "../emu_types.h"
#endif

// Define types of read/write streams
typedef emu_word_t (* read_word_fp)(emu_addr_t);
typedef void (* write_word_fp)(emu_addr_t, emu_word_t);

typedef struct i8080 {
    // registers
    emu_word_t a, b, c, d, e, h, l;
    // stack pointer, program counter
    emu_addr_t sp, pc;
    
    // flags: sign, zero, auxiliary carry,
    // carry, parity, interrupt enable
    _Bool s, z, acy, cy, p, ie;
    
    // True if in HALT state. Only interrupts
    // and RESET can bring i8080 out of this state. 
    _Bool is_halted;
    
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
     * It pushes the return address to the stack,
     * and provides a reference to the i8080.
     * It should return true if i8080 should
     * continue execution. */
    _Bool (* emu_ext_call)(void * const);
    // Records the last instruction executed.
    // Can be used to debug upon emu_ext_call.
    emu_word_t last_instr_exec;
    
    // Cycles taken for last emu_runtime
    emu_size_t cycles_taken;
    
} i8080;

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

// Resets the i8080. PC is set to 0, i8080 exits halt state, and cycles taken is reset to 0. 
// No other working registers or flags are affected.
void i8080_reset(i8080 * const cpu);

// Reads a word and advances PC by 1.
inline emu_word_t i8080_advance_read_word(i8080 * const cpu);

// Executes the next instruction. If an interrupt is pending, services it.
// Returns 0 if it isn't safe to continue execution.
_Bool i8080_next(i8080 * const cpu);

// Executes the opcode on cpu, updating its cycle count, registers and flags.
// Returns 0 if it isn't safe to continue execution.
_Bool i8080_exec(i8080 * const cpu, emu_word_t opcode);

#endif // I_8080_H