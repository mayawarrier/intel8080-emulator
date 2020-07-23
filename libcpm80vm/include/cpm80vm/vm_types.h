/*
 * libcpm80vm types.
 */

#ifndef CPM80_TYPES_H
#define CPM80_TYPES_H

#include "i8080_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef i8080_word_t cpm80_byte_t;
typedef i8080_dbl_word_t cpm80_word_t;
typedef i8080_addr_t cpm80_addr_t;

#ifdef __cplusplus
}
#endif

#endif
