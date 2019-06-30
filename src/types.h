/* 
 * File:   types.h
 * Author: dhruvwarrier
 * 
 * Provides word and address types, and defines
 * max values.
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

#define WORD_T_MAX UINT8_MAX
#define ADDR_T_MAX UINT16_MAX

#endif /* TYPES_H */