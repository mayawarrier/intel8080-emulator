
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "src/i8080/i8080.h"
#include "src/emu.h"

int boot_failure() {
    printf("Emulator boot failure.");
    return EXIT_FAILURE;
}

int main(int argc, char ** argv) {
    
    // ---------------- Set up memory and boot emulator ------------------
    
    // CPU and main memory references
    i8080 cpu;
    mem_t memory;
    
    // Program bytes read
    size_t words_read = 0;
    
    if(memory_init(&memory)) {
        // Setup interrupt vector table and default bootloader
       addr_t program_start_loc = memory_setup_IVT(&memory);
       memory_write_bootloader_default(&memory);
       
       // Read file into memory
       words_read = memory_load("rom.bin", memory.mem, program_start_loc);
       
       // File read error
       if (words_read == SIZE_MAX) {
           return boot_failure();
       }
       
       // Initialize i8080 and setup default memory + IO streams
       emu_init_i8080(&cpu);
       emu_setup_streams_default(&cpu);
       
       // Begin the emulator. Does not return until complete.
       emu_runtime(&cpu, &memory);
       
    } else {
        return boot_failure();
    }
    
    return EXIT_SUCCESS;
}