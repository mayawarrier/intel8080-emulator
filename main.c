
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "src/i8080/i8080.h"
#include "src/emu.h"

// Emulator main memory
static emu_word_t * MEMORY = NULL;

/* Called when emulator fails to boot. Prints error message
 * and cleans up before exit. */
int boot_failure(i8080 * cpu, mem_t * memory_handle);

// Default memory and I/O streams
static emu_word_t rw_from_memory(emu_addr_t addr);
static void ww_to_memory(emu_addr_t addr, emu_word_t word);
static void ww_to_stdout(emu_addr_t addr, emu_word_t word);
static emu_word_t rw_from_stdin(emu_addr_t addr);

int main(int argc, char ** argv) {
    
    // ---------------- Set up memory and boot emulator ------------------
    
    // CPU and main memory
    i8080 cpu;
    mem_t memory_handle;
    
    // Program bytes read
    size_t words_read = 0;
    
    if(memory_init(&memory_handle)) {
        // Setup interrupt vector table and default bootloader
       emu_addr_t program_start_loc = memory_setup_IVT(&memory_handle);
       memory_write_bootloader_default(&memory_handle);
       
       // Read file into memory
       words_read = memory_load("rom.bin", memory_handle.mem, program_start_loc);
       
       // File read error
       if (words_read == SIZE_MAX) {
           return boot_failure(&cpu, &memory_handle);
       }
       
       // Memory load was successful, save reference to memory stream
       MEMORY = memory_handle.mem;
       
       // Initialize i8080 and setup default memory + IO streams
       emu_init_i8080(&cpu);
       emu_setup_streams(&cpu, rw_from_memory, ww_to_memory, rw_from_stdin, ww_to_stdout);
       
       // Begin the emulator. Does not return until complete.
       emu_runtime(&cpu, &memory_handle);
       
    } else {
        return boot_failure(&cpu, &memory_handle);
    }
    
    return EXIT_SUCCESS;
}

int boot_failure(i8080 * cpu, mem_t * memory_handle) {
    printf("Emulator boot failure.");
    emu_cleanup(cpu, memory_handle);
    return EXIT_FAILURE;
}

static emu_word_t rw_from_memory(emu_addr_t addr) {
    return MEMORY[addr];
}

static void ww_to_memory(emu_addr_t addr, emu_word_t word) {
    MEMORY[addr] = word;
}

static void ww_to_stdout(emu_addr_t addr, emu_word_t word) {
    // don't need the port address to write to stdout
    (void)addr;
    printf(WORD_T_FORMAT, word);
}

static emu_word_t rw_from_stdin(emu_addr_t addr) {
    // don't need the port address to read from stdin
    (void)addr;
    emu_word_t rw;
    scanf(WORD_T_FORMAT, &rw);
    return rw;
}