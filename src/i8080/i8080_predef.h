/* 
 * File:   i8080_predef.h
 * Author: dhruvwarrier
 * 
 * Modify this file to port emulator.
 * See i8080.h for the requirements on these types and macros.
 *
 * Created on June 30, 2019, 2:56 PM
 */

#ifndef I8080_PREDEF_H
#define I8080_PREDEF_H

#include <stdint.h>
#include <stdlib.h>

typedef uint8_t emu_word_t;
typedef uint16_t emu_addr_t;
typedef uint32_t emu_buf_t;
typedef size_t emu_size_t;

#define HALF_WORD_SIZE (4)
#define HALF_ADDR_SIZE (8)

#define WORD_T_MAX UINT8_MAX
#define ADDR_T_MAX UINT16_MAX

#define WORD_T_FORMAT "0x%02x"
#define ADDR_T_FORMAT "0x%04x"
#define WORD_T_PRT_FORMAT "%c"

// Suppress deprecation errors with scanf, printf, etc
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#endif

#endif /* I8080_PREDEF_H */