/* 
 * File:   emu.h
 * Author: dhruvwarrier
 *
 * Starts up the emulator and handles boot.
 * 
 * Created on June 30, 2019, 5:28 PM
 */

#ifndef EMU_H
#define EMU_H

#include <stdbool.h>
#include <stdlib.h>
#include "types.h"
#include "i8080_internal.h"

// Allocates the largest amount of memory addressable (in this case 64KB)
bool memory_init(mem_t * const memory_handle);
// Sets up the ROM: creates an interrupt vector table and sets up a default bootloader.
// Returns the address from which it is safe to load the program without overwriting ROM.
addr_t memory_setup_rom(mem_t * const memory_handle);
// Loads a file into memory. Call only after init and setting up ROM.
size_t load_file(const char * file_loc, word_t * memory, addr_t start_loc);

// Begin the emulator. Must have properly set up memory first!
bool emu_runtime(mem_t * const memory);

#endif /* EMU_H */

