/*
 * Provides definitions for portable i8080 base types.
 *
 * - This also searches for a mutex/synchronization primitive on the 
 *   system (unless I8080_DISABLE_ASYNC_INTERRUPTS is defined). If found, the 
 *   type is defined as i8080_mutex_t, and the macro I8080_MUTEX is defined.
 * 
 * The following resources were helpful:
 *
 * Detecting POSIX versions, feature test macros, etc:
 * https://stackoverflow.com/questions/11350878/how-can-i-determine-if-the-operating-system-is-posix-in-c
 * https://stackoverflow.com/questions/54626307/posix-vs-posix-source-vs-posix-c-source
 * http://www.cse.psu.edu/~deh25/cmpsc311/Lectures/Standards/PosixStandard/PosixVersion.c
 * https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
 * https://pubs.opengroup.org/onlinepubs/7908799/xsh/unistd.h.html
 * 
 * https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsection (CRITICAL_SECTION support on Windows)
 * https://github.com/libgit2/libgit2/blob/master/include/git2/stdint.h (defining standard types on MSVC)
 * https://opensource.apple.com/source/Libc/Libc-166/threads.subproj/cthreads.h.auto.html (NeXTSTEP/MACH kernel implementation of threads)
 * https://www.os-book.com/OS9/appendices-dir/b.pdf, pg 8 (NeXTSTEP/MACH system documentation)
 */

#ifndef I8080_TYPES_H
#define I8080_TYPES_H

#include "i8080_predef.h"

/* 
 * Determine portable data types for the i8080.
 *
 * i8080_word_t: unsigned, size at least 8.
 * i8080_addr_t: unsigned, size at least 16.
 * i8080_dbl_word_t: unsigned, at least double the size of i8080_word_t.
 * i8080_uintmax_t: unsigned, counts processor cycles. 
 */

/* 
 * uchar and uint are guaranteed to be at least 8 and 16 bits respectively 
 * on conforming C89 implementations.
 * MSVC doesn't consistently support chars beyond version 1300, so
 * just use Microsoft internal types instead.
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
    #define I8080_SIZE unsigned __int8
    #define I8080_DBL_SIZE unsigned __int16
#else
    #define I8080_SIZE unsigned char
    #define I8080_DBL_SIZE unsigned int
#endif

#if defined(__STDC_VERSION__) || defined (__cplusplus)
    #include <stdint.h>
    #ifdef UINTMAX_MAX
        #define I8080_UINTMAX uintmax_t
    #else
        #define I8080_UINTMAX_FALLBACK
    #endif
#elif defined(_MSC_VER)
    /* 
     * On older versions of MSVC, __int64 should be available,
     * but may not actually be 64 bits
     */
    #define I8080_UINTMAX unsigned __int64
#endif
#if defined(__STDC__) || defined(I8080_UINTMAX_FALLBACK)
    #undef I8080_UINTMAX_FALLBACK
    /* Otherwise use the largest portable type in C89 */
    #define I8080_UINTMAX unsigned long int
#endif

/* 
 * Safe base datatypes 
 */
I8080_CDECL typedef I8080_SIZE i8080_word_t;
I8080_CDECL typedef I8080_DBL_SIZE i8080_addr_t;
I8080_CDECL typedef I8080_DBL_SIZE i8080_dbl_word_t;
I8080_CDECL typedef I8080_UINTMAX i8080_uintmax_t;

/* Read/write handler functions for the i8080 */
I8080_CDECL typedef i8080_word_t(*i8080_read_word_handler)(i8080_addr_t);
I8080_CDECL typedef void(*i8080_write_word_handler)(i8080_addr_t, i8080_word_t);

#undef I8080_SIZE
#undef I8080_DBL_SIZE
#undef I8080_UINTMAX
#undef I8080_UINTMAX_FALLBACK

/*
 * If interrupts are enabled, try to find a mutex/synchronization type.
 * If detected on the system, this type is defined as i8080_mutex_t
 * and the macro I8080_MUTEX is defined.
 */
#ifndef I8080_DISABLE_ASYNC_INTERRUPTS
    #ifdef I8080_WINDOWS
        #include <sdkddkver.h>
        /* Versions from Windows XP and above support critical sections */
        #if (WINVER >= 0x0403) || (_WIN32_WINNT >= 0x0403)
            #include <synchapi.h>
            #define I8080_MUTEX CRITICAL_SECTION
        #endif
    #elif defined(I8080_UNIX)
        #include <unistd.h>
        /* Do we have POSIX? */
        #ifdef _POSIX_VERSION
            /* Pthreads first appeared in POSIX versions >= 199506L.
             * Force POSIX features to be enabled if not available by default: */
            #ifndef _POSIX_SOURCE
                #define _POSIX_SOURCE 199506L
            #elif defined(_POSIX_SOURCE) && (_POSIX_SOURCE < 199506L)
                #warning "_POSIX_SOURCE is less than 199506L, overriding to 199506L..."
                #undef _POSIX_SOURCE
                #define _POSIX_SOURCE 1995096L
            #endif
            #ifndef _POSIX_C_SOURCE
                #define _POSIX_C_SOURCE 199506L
            #elif defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE < 199506L)
                #warning "_POSIX_C_SOURCE is less than 199506L, overriding to 199506L..."
                #undef _POSIX_C_SOURCE
                #define POSIX_C_SOURCE 199506L
            #endif
            /* Are POSIX threads available now? */
            #ifdef _POSIX_THREADS
                /* This *should* also work on OSX/iOS */
                #include <pthread.h>
                #define I8080_MUTEX pthread_mutex_t
            #endif
        #endif
    #else
        /* 
         * I8080_NONSTD
         * Check for more exotic systems.
         * 
         * - The MACH kernel at the core of NeXTSTEP provides mutexes in <cthreads.h>.
         * - GNUC provides atomic synchronization builtins from version 4.7.0 onwards.
         */
        #ifdef __MACH__
            #include <cthreads.h>
            #define I8080_MUTEX mutex_t
        #elif \
            (defined(__GNUC__) || defined(__GNUG__)) && \
            (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 7) || \
            (__GNUC__ == 4 && __GNUC_MINOR__ == 7 && __GNUC_PATCHLEVEL__ >= 0))
            /* any basic type will do */
            #define I8080_MUTEX struct i8080_mutex { int _lock; }
        #endif
    #endif
#endif

#ifdef I8080_MUTEX
    I8080_CDECL typedef I8080_MUTEX i8080_mutex_t;
#endif

#endif /* I8080_TYPES_H */