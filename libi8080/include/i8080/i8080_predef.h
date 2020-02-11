/*
 * Predefs to detect different machines/compilers, and some compatibility
 * definitions. This is included in every file in libi8080.
 *
 * On Windows, this should also be included at the top of every 
 * source file in your project, or before including <Windows.h>, <stdio.h> or <cstdio>.
 * 
 * The following resources were useful:
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
 * https://sourceforge.net/p/predef/wiki/Standards/
 * https://web.archive.org/web/20190803041644/http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system
 */

/*
 * Detect system.
 */
#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
    #ifndef I8080_WINDOWS
        #define I8080_WINDOWS
    #endif
#elif defined(unix) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
    #ifndef I8080_UNIX
        #define I8080_UNIX
    #endif
#else
    #ifndef I8080_NONSTD
        #define I8080_NONSTD
    #endif
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
#ifndef I8080_CDECL
    #ifdef __cplusplus
        #define I8080_CDECL extern "C"
    #else
        #define I8080_CDECL
    #endif
#endif