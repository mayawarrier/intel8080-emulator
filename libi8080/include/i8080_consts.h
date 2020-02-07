/*
 * libi8080 constants.
 * Format specifiers and bit widths for i8080 types.
 */

#ifndef I8080_CONSTS_H
#define I8080_CONSTS_H

/* WORD_SIZE: bit width of an i8080 word
 * ADDR_SIZE: bit width of an i8080 address
 * WORD_MAX: max value that should be held by an i8080_word_t
 * ADDR_MAX: max value that should be held by an i8080_addr_t
 * WORD_SCN_FORMAT/WORD_PRT_FORMAT: hexadecimal format specifiers for i8080_word_t
 * ADDR_SCN_FORMAT/ADDR_PRT_FORMAT: hexadecimal format specifiers for i8080_addr_t
 * WORD_CONSOLE_FORMAT: format specifier to print word as ASCII */

/* i8080_word_t and i8080_addr_t are 8 and 16-bit uint types respectively */

#define WORD_SIZE (8)
#define ADDR_SIZE (16)
#define WORD_MAX (255)
#define ADDR_MAX (65535)

#define WORD_SCN_FORMAT "0x%02x"
#define ADDR_SCN_FORMAT "0x%04x"
#define WORD_PRT_FORMAT "0x%02x"
#define ADDR_PRT_FORMAT "0x%04x"
#define WORD_CONSOLE_FORMAT "%c"

#endif /* I8080_CONSTS_H */