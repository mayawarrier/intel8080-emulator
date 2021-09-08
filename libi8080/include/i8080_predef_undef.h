/*
 * Include this to unndef the macros defined in i8080_predef.h.
 */

#undef I8080_WINDOWS
#undef I8080_UNIX
#undef I8080_WINDOWS_MIN_VER
#undef I8080_POSIX_MIN_VER
#undef I8080_GNUC_MIN_VER
#undef I8080_UNIDENTIFIED
#undef I8080_CDECL
#undef I8080_PREDEF_H

#ifdef __STDC__
    #undef inline
    #undef volatile
#endif