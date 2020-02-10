/*
 * Predefs to detect different machines/compilers, and some compatibility
 * definitions. This is included at the top of every file in libi8080.
 *
 * Undef the macros defined here by including "i8080_predef_undef.h"
 *
 * On Windows, this should also be included at the top of every 
 * source file in your project, or before including <Windows.h>, <stdio.h> or <cstdio>.
 * 
 * The following resources were useful:
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
 * https://sourceforge.net/p/predef/wiki/Standards/
 * https://web.archive.org/web/20190803041644/http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system
 */

#ifndef I8080_PREDEF_H
#define I8080_PREDEF_H

/*
 * Detect system.
 */
#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
    #define I8080_WINDOWS
#elif defined(unix) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
    #define I8080_UNIX
#else
    #define I8080_NONSTD
#endif

#ifdef I8080_WINDOWS
    /*
     * Suppress stdio deprecation errors from the Windows CRT,
     * and overload with safe versions if available.
     * These must be defined before including <Windows.h> to have effect.
     */
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

/* Allow C++ compilers to link to C headers */
#ifdef __cplusplus
    #define I8080_CDECL extern "C"
#else
    #define I8080_CDECL
#endif

#endif /* I8080_PREDEF_H */