/*
 * Predefs for different machines/compilers.
 * Undef the macros defined here by including i8080_predef_undef.h
 *
 * The following resources were useful:
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
 * https://sourceforge.net/p/predef/wiki/Standards/
 * http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system
 */

#ifndef I8080_PREDEF_H
#define I8080_PREDEF_H

#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
    #define I8080_WINDOWS
#elif defined(unix) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
    #define I8080_UNIX
#else
    #define I8080_UNIDENTIFIED
#endif

 /* Detect minimum supported version for Windows. */
#ifdef I8080_WINDOWS
    #include <sdkddkver.h>
    /* Versions from Windows XP and above support critical sections with spin counts:
     * https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsectionandspincount */
    #if (WINVER >= 0x0403) || (_WIN32_WINNT >= 0x0403)
        #define I8080_WINDOWS_MIN_VER
    #endif
#endif

/* Detect minimum supported version for POSIX. */
#ifdef I8080_UNIX
    /* Defining this forces features from 199506 to be defined, if available */
    #ifndef _POSIX_C_SOURCE 
        #define _POSIX_C_SOURCE 199506L
    #endif
    #include <unistd.h>
    /* Pthreads first appeared in the POSIX standard in IEEE 1003.1c-1995, published in 06/1995:
     * https://standards.ieee.org/standard/1003_1c-1995.html */
    #if defined(_POSIX_VERSION) && (_POSIX_VERSION >= 199506L)
        #define I8080_POSIX_MIN_VER
    #endif
#endif

/* If being compiled with GNUC and mutex primitives are not available, GNUC provides
 * atomic synchronization functions from version 4.7.0, that can be used to simulate mutexes. */
#if (defined(__GNUC__) || defined(__GNUG__)) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 7) || (__GNUC__ == 4 && __GNUC_MINOR__ == 7 && __GNUC_PATCHLEVEL__ >= 0))
    #define I8080_GNUC_MIN_VER
#endif

 /* Allow C++ compilers to link to C headers */
#ifdef __cplusplus
    /* if compiled with C++, extern as C */
    #define I8080_CDECL extern "C"
#else
    /* if compiled with C, no extern required */
    #define I8080_CDECL
#endif

#endif /* I8080_PREDEF_H */