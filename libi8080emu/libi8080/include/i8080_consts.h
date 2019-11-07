/*
 * libi8080 constants.
 * Format specifiers and bit widths for i8080 types.
 */

#ifndef I8080_CONSTS_H
#define I8080_CONSTS_H

#include <inttypes.h>

/* HALF_WORD_SIZE: half the bit width of emu_word_t
 * HALF_ADDR_SIZE : half the bit width of emu_addr_t
 * WORD_T_MAX : max value that can be held by emu_word_t
 * ADDR_T_MAX : max value that can be held by emu_addr_t
 * WORD_T_SCN_FORMAT : hexadecimal format specifier for emu_word_t
 * ADDR_T_SCN_FORMAT : hexadecimal format specifier for emu_addr_t
 * WORD_T_PRT_FORMAT : format specifier to print word as ASCII */

#define WORD_SIZE (8)
#define ADDR_SIZE (16)
#define WORD_T_MAX (255)
#define ADDR_T_MAX (65535)

#define WORD_T_SCN_FORMAT "0x%" SCNx8
#define ADDR_T_SCN_FORMAT "0x%" SCNx16
#define WORD_T_PRT_FORMAT "0x%02" PRIx8
#define ADDR_T_PRT_FORMAT "0x%04" PRIx16
#define WORD_T_ASCII_FORMAT "%c"

#endif /* I8080_CONSTS_H */