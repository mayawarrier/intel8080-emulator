/*
 * Predefs to detect different machines/compilers.
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