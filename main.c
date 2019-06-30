
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "src/i8080.h"
#include "src/emu.h"

int boot_failure() {
    printf("Emulator boot failure.");
    return EXIT_FAILURE;
}

int main(int argc, char ** argv) {
    
    // ---------------- Set up memory and boot emulator ------------------
    
    mem_t memory;
    
    if(memory_init(&memory)) {
        // Setup interrupts vector table and bootloader
       addr_t program_start_loc = memory_setup_rom(&memory);
       // Read file into memory
       size_t words_read = load_file("rom.bin", memory.mem, program_start_loc);
       
       // File read error
       if (words_read == SIZE_MAX) {
           return boot_failure();
       }
       
       memory.num_prog_bytes = words_read;
       
       // Start the emulator. Does not return until complete.
       emu_runtime(&memory);
       
    } else {
        return boot_failure();
    }
    
    return EXIT_SUCCESS;
}