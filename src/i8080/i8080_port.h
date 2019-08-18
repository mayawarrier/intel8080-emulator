/* 
 * File:   i8080_port.h
 * Author: dhruvwarrier
 * 
 * Modify this file to port to different host/virtual machines.
 * See i8080.h for the requirements on these types and macros.
 *
 * Created on June 30, 2019, 2:56 PM
 */

#ifndef I8080_PORT_H
#define I8080_PORT_H

#include <stdint.h>

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

// This must be defined before including i8080_sync.h.
#define PTHREADS_AVAILABLE (1)

// Provides access to synchronization functions.
#include "i8080_sync.h"

#endif /* I8080_PORT_H */