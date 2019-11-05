/*
 * Provides definitions for all i8080 base types and function pointers,
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

/* Try to find a mutex/synchronization primitive */
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

/* Use stdint fixed width types if available */
#if (defined(__STDC_VERSION__) || defined(__cplusplus) || defined(I8080_WINDOWS_MIN_VER)) /* TODO: Use libgit2's stdint.h for bad msvc configs */
    #include <stdint.h>
#endif

/* Determine safe base types __i8080_size_t__ and __i8080_dbl_size_t__ 
 * These are guaranteed to hold at least 8 and 16 bits respectively */
#if defined(UINT8_MAX) && defined(UINT16_MAX)
    typedef uint8_t __i8080_size_t__;
    typedef uint16_t __i8080_dbl_size_t__;
#else
    /* C89 guarantees uchar and uint to be 8 bits and 16 bits at least */
    typedef unsigned char __i8080_size_t__;
    typedef unsigned int __i8080_dbl_size_t__;
#endif

/* i8080-emu base types
 *
 * i8080_word_t: unsigned, size at least 8.
 * i8080_addr_t: unsigned, size at least 16.
 * i8080_dbl_word_t: unsigned, at least double the size of i8080_word_t.
 * i8080_uintmax_t: unsigned, counts processor cycles.
 */
I8080_CDECL typedef __i8080_size_t__ i8080_word_t;
I8080_CDECL typedef __i8080_dbl_size_t__ i8080_addr_t;
I8080_CDECL typedef __i8080_dbl_size_t__ i8080_dbl_word_t;
#ifdef UINTMAX_MAX
    I8080_CDECL typedef uintmax_t i8080_uintmax_t;
#else
    /* Largest portable type in C89 */
    I8080_CDECL typedef unsigned long int i8080_uintmax_t;
#endif

/* Read/write streams for the i8080 */
I8080_CDECL typedef i8080_word_t(*i8080_read_word_fp)(i8080_addr_t);
I8080_CDECL typedef void(*i8080_write_word_fp)(i8080_addr_t, i8080_word_t);

#include "i8080_predef_undef.h"

#endif /* I8080_TYPES_H */