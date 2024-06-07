
#ifndef I8080_TYPES_H
#define I8080_TYPES_H

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#if _MSC_VER >= 1300
typedef unsigned __int8 i8080_word_t;
typedef unsigned __int16 i8080_dword_t;
typedef unsigned __int16 i8080_addr_t;
typedef unsigned __int64 i8080_cycles_t;

#define I8080_WORD_T_MAX 0xff
#define I8080_DWORD_T_MAX 0xffff

#else
typedef unsigned char i8080_word_t;
typedef unsigned short i8080_dword_t;
typedef unsigned short i8080_addr_t;

#define I8080_WORD_T_MAX UCHAR_MAX
#define I8080_DWORD_T_MAX USHRT_MAX

#ifdef ULLONG_MAX
typedef unsigned long long i8080_cycles_t;
#else
typedef unsigned long i8080_cycles_t;
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* I8080_TYPES_H */
