/* 
 * File:   types.h
 * Author: dhruvwarrier
 * 
 * Provides word and address types, defines
 * max values, and provides formatting specifiers.
 * 
 * emu_buf_t should be a type that is larger than the 
 * widths of emu_word_t and emu_addr_t.
 * 
 * Modify to port to different architectures.
 *
 * Created on June 30, 2019, 2:56 PM
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint8_t emu_word_t;
typedef uint16_t emu_addr_t;
typedef uint32_t emu_buf_t; 

#define HALF_WORD_SIZE (4)
#define HALF_ADDR_SIZE (8)

#define WORD_T_MAX UINT8_MAX
#define ADDR_T_MAX UINT16_MAX

#define WORD_T_FORMAT "%#04x"
#define ADDR_T_FORMAT "%#06x"

#define WORD_T_PRT_FORMAT "%c"

#endif /* TYPES_H */