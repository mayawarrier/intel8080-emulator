/* 
 * File:   i8080.h
 * Author: dhruvwarrier
 *
 * Emulate an i8080.
 * 
 * Created on June 30, 2019, 5:28 PM
 */

#ifndef I_8080_H
#define I_8080_H

#include "i8080_predef.h"

 // Suppress MSVC deprecation errors in stdio functions.
#ifdef I8080_WINDOWS_MIN_VER
	#ifndef _CRT_SECURE_NO_WARNINGS
		#define _CRT_SECURE_NO_WARNINGS
	#endif
	#ifndef _CRT_SECURE_NO_DEPRECATE
		#define _CRT_SECURE_NO_DEPRECATE
	#endif
	#if defined(I8080_CDECL) && !defined(_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES)
		#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
	#endif
#endif

// Determine i8080_mutex_t for different environments.
#ifdef I8080_WINDOWS_MIN_VER
	// Critical sections have better performance than standard
	// Windows mutexes and appear to be compatible with std::thread.
	#include <Windows.h>
	I8080_CDECL typedef CRITICAL_SECTION i8080_mutex_t;
#elif defined I8080_POSIX_MIN_VER
	// Use pthreads, since they are available
	#include <pthreads.h>
	I8080_CDECL typedef pthread_mutex_t i8080_mutex_t;
#elif defined I8080_GNUC_MIN_VER
	// Simulate a mutex using acquire - release semantics on a char.
	I8080_CDECL typedef char i8080_mutex_t;
#else
	// A volatile char ~should~provide the best chance at proper sync, 
	// if nothing else is available.
	I8080_CDECL typedef volatile char i8080_mutex_t;
#endif

/* Below are the i8080 emulator base types, format specifiers and sizes.
 * These can be modified to emulate architectures close to the i8080.
 *
 * emu_word_t: An unsigned type with a bit width equal to the virtual machine's word size.
 * emu_addr_t: An unsigned type with a bit width equal to the virtual machine's address size.
 * emu_buf_t: An unsigned type that is double the size of emu_word_t and at least 1 bit larger than emu_addr_t.
 * emu_large_t: An unsigned large type. Used to count processor cycles taken. If this is not large enough,
 *             the emulator will still run properly, but cycles_taken might overflow.
 *
 * HALF_WORD_SIZE: half the bit width of emu_word_t
 * HALF_ADDR_SIZE: half the bit width of emu_addr_t
 * WORD_T_MAX: max value that can be held by emu_word_t
 * ADDR_T_MAX: max value that can be held by emu_addr_t
 * WORD_T_FORMAT: hexadecimal format specifier for emu_word_t
 * ADDR_T_FORMAT: hexadecimal format specifier for emu_addr_t
 * WORD_T_PRT_FORMAT: format specifier to print word as ASCII
 */

#include <stdint.h>
#include <stddef.h>

I8080_CDECL typedef uint8_t emu_word_t;
I8080_CDECL typedef uint16_t emu_addr_t;
I8080_CDECL typedef uint32_t emu_buf_t;
I8080_CDECL typedef uintmax_t emu_large_t;

I8080_CDECL_EXTERN const size_t HALF_WORD_SIZE;			// = 4
I8080_CDECL_EXTERN const size_t HALF_ADDR_SIZE;			// = 8
I8080_CDECL_EXTERN const emu_word_t WORD_T_MAX;			// = UINT8_MAX
I8080_CDECL_EXTERN const emu_addr_t ADDR_T_MAX;			// = UINT16_MAX

I8080_CDECL_EXTERN const char WORD_T_FORMAT[];			// = "0x%02x"
I8080_CDECL_EXTERN const char ADDR_T_FORMAT[];			// = "0x%04x"
I8080_CDECL_EXTERN const char WORD_T_PRT_FORMAT[];		// = "%c"

// Define types of read/write streams
I8080_CDECL typedef emu_word_t(*emu_read_word_fp)(emu_addr_t);
I8080_CDECL typedef void(*emu_write_word_fp)(emu_addr_t, emu_word_t);

I8080_CDECL typedef struct i8080 {
    // Registers
    emu_word_t a, b, c, d, e, h, l;
    // Stack pointer, program counter
    emu_addr_t sp, pc;
    
    // Flags: sign, zero, auxiliary carry,
    // carry, parity, interrupt enable
    _Bool s, z, acy, cy, p, ie;
    
    // True if in HALT state. Only interrupts
    // and RESET can bring i8080 out of this state. 
    _Bool is_halted;
    
    // provide your own read/write streams
    
    // Read from a memory stream
    emu_read_word_fp read_memory;
    // Write to a memory stream
    emu_write_word_fp write_memory;
    
    // I/O stream in
    emu_read_word_fp port_in;
    // I/O stream out
    emu_write_word_fp port_out;
    
    /* This is called on opcode 0x38. 0x38
     * is actually an undocumented NOP, but is
     * re-purposed for this instead.
     * It pushes the return address to the stack,
     * and provides a reference to the i8080.
     * Return true if i8080 should continue 
     * execution after this call. */
    _Bool (* emu_ext_call)(void * const);
    // Records the last instruction executed.
    // Can be used to debug upon emu_ext_call.
    emu_word_t last_instr_exec;
    
    /* When an interrupt is issued, this
     * is called back when the i8080 is ready
     * to receive the interrupt vector. After
     * the vector has been executed, interrupts 
     * are disabled again. */
    emu_word_t (* interrupt_acknowledge)(void);
    // Whether there is a pending interrupt request to service.
    _Bool pending_interrupt_req;
    
    /* When an interrupt comes on another thread,
     * this mutex is used to sync i8080 status
     * with the interrupt generator, so the
     * interrupt is not accidentally double-serviced
     * or missed by the i8080. */
	i8080_mutex_t i_mutex;
    
    // Cycles taken for last emu_runtime
    emu_large_t cycles_taken;
    
} i8080;

/* Resets the i8080, and performs first time initialization. */
I8080_CDECL void i8080_init(i8080 * const cpu);

/* Resets the i8080. PC is set to 0, i8080 exits halt state, and cycles taken is reset to 0. 
 * No other working registers or flags are affected. Equivalent to pulling RESET low. */
I8080_CDECL void i8080_reset(i8080 * const cpu);

/* Executes the next instruction. If an interrupt is pending, services it.
 * Returns 0 if it isn't safe to continue execution. */
I8080_CDECL _Bool i8080_next(i8080 * const cpu);

/* Executes the opcode on cpu, updating its cycle count, registers and flags.
 * Returns 0 if it isn't safe to continue execution. */
I8080_CDECL _Bool i8080_exec(i8080 * const cpu, emu_word_t opcode);

/* Sends an interrupt to the i8080. This is thread-safe, and it will be serviced when the i8080 is ready. 
 * If your compiler/environment does not support mutexes, this may not work correctly. See i8080.c for implementation.
 * 
 * When ready, the i8080 will call interrupt_acknowledge() which should return the vector to be executed. 
 * Interrupts are disabled every time an interrupt is serviced, so they must be enabled again before the next interrupt. */
I8080_CDECL void i8080_interrupt(i8080 * const cpu);

#include "i8080_predef_undef.h"

#endif /* I_8080_H */