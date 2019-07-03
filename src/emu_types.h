/* 
 * File:   types.h
 * Author: dhruvwarrier
 * 
 * Provides word and address types, defines
 * max values, and provides formatting specifiers.
 * 
 * Modify to port to different architectures.
 *
 * Created on June 30, 2019, 2:56 PM
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint8_t word_t;
typedef uint16_t addr_t;

#define HALF_WORD_SIZE 4
#define HALF_ADDR_SIZE 8

#define WORD_T_MAX UINT8_MAX
#define ADDR_T_MAX UINT16_MAX

#define WORD_T_FORMAT "%#04x"
#define ADDR_T_FORMAT "%#06x"

#endif /* TYPES_H */