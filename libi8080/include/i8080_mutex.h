/*
 * Detect platform and mutex type on the system.
 *
 * If found, macro I8080_MUTEX is defined as the type name, and one of
 * I8080_PLATFORM_WIN32, I8080_PLATFORM_UNIX, or I8080_PLATFORM_OTHER will also be defined.
 *
 * Version macros should be defined before including this header.
 * For eg. on Windows:
 * - WINVER or _WIN32_WINNT should be defined >= 0x0403. 
 * - Include windows.h, stdafx.h, winsdkver.h, targetver.h or sdkddkver.h.
 * On POSIX or XOPEN:
 * - define _POSIX_SOURCE/_POSIX_C_SOURCE >= 199506L or _XOPEN_SOURCE >= 500.
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
 * Detecting other systems:
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
 * https://sourceforge.net/p/predef/wiki/Standards/
 * https://web.archive.org/web/20190803041644/http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system
 * 
 * https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsection (CRITICAL_SECTION support on Windows)
 * https://github.com/libgit2/libgit2/blob/master/include/git2/stdint.h (defining standard types on MSVC)
 * https://opensource.apple.com/source/Libc/Libc-166/threads.subproj/cthreads.h.auto.html (NeXTSTEP/MACH kernel implementation of threads)
 * https://www.os-book.com/OS9/appendices-dir/b.pdf, pg 8 (NeXTSTEP/MACH system documentation)
 */

#if defined(_MSC_VER) && (_MSC_VER > 1000)
#pragma once
#endif

#ifndef I8080_DEFS_H
#define I8080_DEFS_H

#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)

/* Windows XP and above */
#if (WINVER >= 0x0403) || (_WIN32_WINNT >= 0x0403)
    #include <synchapi.h>
	#define I8080_PLATFORM_WIN32
    #define I8080_MUTEX CRITICAL_SECTION
#endif

#elif defined(unix) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))

/* POSIX/XOPEN/OSX/iOS */

#ifndef _POSIX_SOURCE
    #define _POSIX_SOURCE 199506L
#endif
#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 199506L
#endif
#ifndef _XOPEN_SOURCE
    #define _XOPEN_SOURCE 500
#endif

#include <unistd.h>
#if ((defined(__APPLE__) && defined(__MACH__)) || (defined(_POSIX_VERSION) && _POSIX_VERSION >= 199506L)) && defined(_POSIX_THREADS)
    #include <pthread.h>
	#define I8080_PLATFORM_PTHREAD
    #define I8080_MUTEX pthread_mutex_t
#endif

#else
    /* 
     * Check for more exotic systems.
     * - NeXTSTEP/MACH kernel has <cthreads.h>.
     * - GNUC has atomic synchronization builtins from version 4.7.0.
     */
    #ifdef __MACH__
        #include <cthreads.h>
		#define I8080_PLATFORM_OTHER
        #define I8080_MUTEX mutex_t
    #elif \
        (defined(__GNUC__) || defined(__GNUG__)) && \
        (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 7) || \
        (__GNUC__ == 4 && __GNUC_MINOR__ == 7 && __GNUC_PATCHLEVEL__ >= 0))
		#define I8080_PLATFORM_OTHER
        #define I8080_MUTEX int
    #endif
#endif

#endif /* I8080_DEFS_H */
