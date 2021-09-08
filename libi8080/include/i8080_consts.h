/*
 * libi8080 constants.
 * Format specifiers and bit widths for i8080 types.
 */

#ifndef I8080_CONSTS_H
#define I8080_CONSTS_H

#include <stdint.h>
#include <inttypes.h>

/* HALF_WORD_SIZE: half the bit width of emu_word_t
 * HALF_ADDR_SIZE : half the bit width of emu_addr_t
 * WORD_T_MAX : max value that can be held by emu_word_t
 * ADDR_T_MAX : max value that can be held by emu_addr_t
 * WORD_T_SCN_FORMAT : hexadecimal format specifier for emu_word_t
 * ADDR_T_SCN_FORMAT : hexadecimal format specifier for emu_addr_t
 * WORD_T_PRT_FORMAT : format specifier to print word as ASCII */

#define HALF_WORD_SIZE (4)
#define HALF_ADDR_SIZE (8)
#define WORD_T_MAX UINT8_MAX
#define ADDR_T_MAX UINT16_MAX

#define WORD_T_SCN_FORMAT "0x%" SCNx8
#define ADDR_T_SCN_FORMAT "0x%" SCNx16
#define WORD_T_PRT_FORMAT "0x%02" PRIx8
#define ADDR_T_PRT_FORMAT "0x%04" PRIx16
#define WORD_T_ASCII_FORMAT "%c"

#endif /* I8080_CONSTS_H */