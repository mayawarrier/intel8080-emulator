/*
 * Type definitions for libi8080.
 */

#ifndef I8080_TYPES_H
#define I8080_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * I8080_WORD = at least 8 bits unsigned
 * I8080_DBL_WORD = at least 16 bits unsigned
 */
#if (_MSC_VER >= 1300)
#define I8080_WORD unsigned __int8
#define I8080_DBL_WORD unsigned __int16
#else
#define I8080_WORD unsigned char
#define I8080_DBL_WORD unsigned int
#endif

typedef I8080_WORD i8080_word_t;
typedef I8080_DBL_WORD i8080_addr_t;
typedef I8080_DBL_WORD i8080_dbl_word_t;

#undef I8080_WORD
#undef I8080_DBL_WORD

#ifdef __cplusplus
}
#endif

#endif /* I8080_TYPES_H */
