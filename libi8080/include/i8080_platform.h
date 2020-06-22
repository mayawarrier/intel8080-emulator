/*
 * Determine platform type.
 *
 * Defines I8080_PLATFORM as one of I8080_PLATFORM_WIN32, I8080_PLATFORM_PTHREAD, I8080_PLATFORM_MACH, or I8080_PLATFORM_GNU.
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
 * POSIX systems & feature test macros:
 * https://stackoverflow.com/questions/11350878/how-can-i-determine-if-the-operating-system-is-posix-in-c
 * https://stackoverflow.com/questions/54626307/posix-vs-posix-source-vs-posix-c-source
 * http://www.cse.psu.edu/~deh25/cmpsc311/Lectures/Standards/PosixStandard/PosixVersion.c
 * https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
 * https://pubs.opengroup.org/onlinepubs/7908799/xsh/unistd.h.html
 *
 * Less common systems (XOPEN, Solaris, etc):
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
 * https://sourceforge.net/p/predef/wiki/Standards/
 * https://web.archive.org/web/20190803041644/http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system
 *
 * https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsection
 * https://www.os-book.com/OS9/appendices-dir/b.pdf, pg 8 (NeXTSTEP/MACH system documentation)
 * gcc has atomic synchronization built-in >= 4.7.0. 
 * Depends on target archictecture; is not guaranteed to work!
 * https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/_005f_005fatomic-Builtins.html#g_t_005f_005fatomic-Builtins
 */

#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)

/* Windows XP and above */
#if (WINVER >= 0x0403) || (_WIN32_WINNT >= 0x0403)
	#include <synchapi.h>
	#define I8080_PLATFORM I8080_PLATFORM_WIN32
#endif

#elif defined(unix) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))

 /* POSIX/XOPEN/Cygwin?/OSX/iOS */
#include <unistd.h>
#if ((defined(__APPLE__) && defined(__MACH__)) || defined(_POSIX_THREADS)
	#include <pthread.h>
	#define I8080_PLATFORM I8080_PLATFORM_PTHREAD
#endif

#elif defined(__MACH__)
	/* NeXTSTEP/MACH kernel */
	#include <cthreads.h>
	#define I8080_PLATFORM I8080_PLATFORM_MACH
#elif \
    (defined(__GNUC__) || defined(__GNUG__)) && \
    (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 7) || \
    (__GNUC__ == 4 && __GNUC_MINOR__ == 7 && __GNUC_PATCHLEVEL__ >= 0))
	#define I8080_PLATFORM I8080_PLATFORM_GNU
#endif
