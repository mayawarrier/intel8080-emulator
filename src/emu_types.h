/* 
 * File:   emu_types.h
 * Author: dhruvwarrier
 * 
 * Provides word and address types, defines
 * max values, and provides formatting specifiers.
 * 
 * See i8080.h for the requirements on these types and macros.
 * 
 * Modify to port to different host/virtual machines.
 *
 * Created on June 30, 2019, 2:56 PM
 */

#ifndef EMU_TYPES_H
#define EMU_TYPES_H

#include <stdint.h>

typedef uint8_t emu_word_t;
typedef uint16_t emu_addr_t;
typedef uint32_t emu_buf_t;
typedef size_t emu_size_t;

#define HALF_WORD_SIZE (4)
#define HALF_ADDR_SIZE (8)

#define WORD_T_MAX UINT8_MAX
#define ADDR_T_MAX UINT16_MAX

#define WORD_T_FORMAT "%#04x"
#define ADDR_T_FORMAT "%#06x"

#define WORD_T_PRT_FORMAT "%c"

#endif /* EMU_TYPES_H */