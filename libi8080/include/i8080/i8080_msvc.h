/*
 * Suppress stdio deprecation errors from the Windows CRT,
 * and overload with safe versions if available.
 * These must be defined before including <Windows.h>, <stdio.h> or <cstdio> to have effect.
 */

#include "i8080_predef.h"

#ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_SECURE_NO_DEPRECATE
    #define _CRT_SECURE_NO_DEPRECATE
#endif
#if defined(__cplusplus) && !defined(_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES)
    #define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#endif

/* 
 * Prevent exporting these macros by default.
 * If you want access to these, include i8080_predef.h 
 */
#undef I8080_WINDOWS
#undef I8080_UNIX
#undef I8080_NONSTD