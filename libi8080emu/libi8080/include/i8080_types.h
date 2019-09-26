
#ifndef I8080_TYPES_H
#define I8080_TYPES_H

#include "i8080_predef.h"

// Suppress MSVC deprecation errors. These must be defined before including Windows.h.
#ifdef I8080_WINDOWS_MIN_VER
    #ifndef _CRT_SECURE_NO_WARNINGS
        #define _CRT_SECURE_NO_WARNINGS
    #endif
    #ifndef _CRT_SECURE_NO_DEPRECATE
        #define _CRT_SECURE_NO_DEPRECATE
    #endif
    #if defined(__cplusplus) && !defined(_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES)
        #define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
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
    #include <pthread.h>
    I8080_CDECL typedef pthread_mutex_t i8080_mutex_t;
#elif defined I8080_GNUC_MIN_VER
    // Simulate a mutex using acquire - release semantics on a char.
    I8080_CDECL typedef char i8080_mutex_t;
#else
    // A volatile char ~should~provide the best chance at proper sync, 
    // if nothing else is available.
    I8080_CDECL typedef volatile char i8080_mutex_t;
#endif

/* Below are the i8080 emulator base types.
 * These can be modified to emulate architectures close to the i8080.
 *
 * emu_word_t: An unsigned type with a bit width equal to the virtual machine's word size.
 * emu_addr_t: An unsigned type with a bit width equal to the virtual machine's address size.
 * emu_buf_t: An unsigned type that is double the size of emu_word_t and at least 1 bit larger than emu_addr_t.
 * emu_large_t: An unsigned large type. Used to count processor cycles taken. If this is not large enough,
 *             the emulator will still run properly, but cycles_taken might overflow.
 */

#include <stdint.h>

I8080_CDECL typedef uint8_t emu_word_t;
I8080_CDECL typedef uint16_t emu_addr_t;
I8080_CDECL typedef uint32_t emu_buf_t;
I8080_CDECL typedef uintmax_t emu_large_t;

I8080_CDECL typedef emu_word_t(*emu_read_word_fp)(emu_addr_t);
I8080_CDECL typedef void(*emu_write_word_fp)(emu_addr_t, emu_word_t);

#include "i8080_predef_undef.h"

#endif /* I8080_TYPES_H */