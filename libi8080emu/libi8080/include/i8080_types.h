/*
 * Provides definitions for all i8080 types and function pointers,
 * and suppresses warnings & errors from MSVC. If including this in an MSVC project,
 * it should be included before any Windows header or before including <stdio.h>.
 */

#ifndef I8080_TYPES_H
#define I8080_TYPES_H

#include "i8080_predef.h"

/* Suppress MSVC deprecation errors. These must be defined before including Windows.h. */
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

/* Determine i8080_mutex_t for different environments. */
#ifdef I8080_WINDOWS_MIN_VER
    /* Critical sections have better performance than standard
     * Windows mutexes and appear to be compatible with std::thread. */
    #include <Windows.h>
    I8080_CDECL typedef CRITICAL_SECTION i8080_mutex_t;
#elif defined I8080_POSIX_MIN_VER
    /* Use pthreads, since they are available */
    #include <pthread.h>
    I8080_CDECL typedef pthread_mutex_t i8080_mutex_t;
#elif defined I8080_GNUC_MIN_VER
    /* Simulate a mutex using acquire - release semantics on a char. */
    I8080_CDECL typedef char i8080_mutex_t;
#else
    /* A volatile char ~should~ provide the best chance at proper sync, 
     * if nothing else is available. */
    I8080_CDECL typedef volatile char i8080_mutex_t;
#endif

/* i8080-emu base types follow.
 * These can be modified for the desired host/virtual machine.
 *
 * emu_word_t: An unsigned type with a bit width equal to the virtual machine's word size.
 * emu_addr_t: An unsigned type with a bit width equal to the virtual machine's address size.
 * emu_buf_t: An unsigned type that is double the size of emu_word_t and at least 1 bit larger than emu_addr_t.
 * emu_large_t: An unsigned large type. Used to count processor cycles taken. If this is not large enough,
 *             the emulator will still run properly, but cycles_taken might overflow.
 */

#if (defined(__STDC_VERSION__) || defined(__cplusplus) || (defined(I8080_WINDOWS) && _MSC_VER > 0000)) /* TODO: Placeholder, add real version check for MSVC! */
    /* If stdint is available, we may get some fixed width types from there */
    #include <stdint.h>
#else
    /* If CHAR_BIT is 8, we can construct types from a uchar */
    #include <limits.h>
    #if (CHAR_BIT == 8)
        typedef unsigned char __i8080_byte_t__;
    #else
        #error "CHAR_BIT is not 8! Provide your own byte-sized type."
    #endif
#endif

#ifdef UINT8_MAX
    typedef uint8_t __i8080_uint8_t__;
#else
    typedef __i8080_byte_t__ __i8080_uint8_t__;
#endif

#ifdef UINT16_MAX
    typedef uint16_t __i8080_uint16_t__;
#else
    typedef struct {
        __i8080_byte_t__ b0;
        __i8080_byte_t__ b1;
    } __i8080_uint16_t__;
#endif

#ifdef UINT32_MAX
    typedef uint32_t __i8080_uint32_t__;
#else
    typedef struct {
        __i8080_byte_t__ b0;
        __i8080_byte_t__ b1;
        __i8080_byte_t__ b2;
        __i8080_byte_t__ b3;
    } __i8080_uint32_t__;
#endif

#ifdef UINT64_MAX
    typedef uint64_t __i8080_uint64_t__;
#else
    typedef struct {
        __i8080_byte_t__ b0;
        __i8080_byte_t__ b1;
        __i8080_byte_t__ b2;
        __i8080_byte_t__ b3;
        __i8080_byte_t__ b4;
        __i8080_byte_t__ b5;
        __i8080_byte_t__ b6;
        __i8080_byte_t__ b7;
    } __i8080_uint64_t__;
#endif

/* Construct base types from safe types above */
I8080_CDECL typedef __i8080_uint8_t__ emu_word_t;
I8080_CDECL typedef __i8080_uint16_t__ emu_addr_t;
I8080_CDECL typedef __i8080_uint32_t__ emu_buf_t;
I8080_CDECL typedef __i8080_uint64_t__ emu_large_t;

/* */

I8080_CDECL typedef emu_word_t(*emu_read_word_fp)(emu_addr_t);
I8080_CDECL typedef void(*emu_write_word_fp)(emu_addr_t, emu_word_t);

#include "i8080_predef_undef.h"

#endif /* I8080_TYPES_H */