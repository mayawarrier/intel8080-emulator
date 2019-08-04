
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "src/i8080/i8080.h"
#include "src/emu.h"

// Emulator main memory
static emu_word_t * MEMORY = NULL;

// Default memory and I/O streams
static emu_word_t rw_from_memory(emu_addr_t addr) {
    return MEMORY[addr];
}

static void ww_to_memory(emu_addr_t addr, emu_word_t word) {
    MEMORY[addr] = word;
}

static void cpm_ww_to_stdout(emu_addr_t addr, emu_word_t word) {
    // Address is duplicated, pick lower 8 bits
    emu_word_t port_addr = (emu_word_t)(addr & WORD_T_MAX);
    if (port_addr == CPM_CONSOLE_ADDR) {
        printf(WORD_T_PRT_FORMAT, word);
        // Some OSs buffer stdout, flush this buffer so that future scanfs
        // don't read from this buffer but from stdin instead
        fflush(stdout);
    } 
}

static emu_word_t cpm_rw_from_stdin(emu_addr_t addr) {
    emu_word_t rw = 0x00;
     // Address is duplicated, pick lower 8 bits
    emu_word_t port_addr = (emu_word_t)(addr & WORD_T_MAX);
    if (port_addr == CPM_CONSOLE_ADDR) {
        scanf(WORD_T_PRT_FORMAT, &rw);
    }
    return rw;
}

int main(int argc, char ** argv) {
    
    const char * test_file_location = (argc > 1) ? argv[1] : NULL;
    
    if (test_file_location == NULL) {
        printf("No file provided.");
        goto boot_failure;
    }
    
    // CPU and main memory
    i8080 cpu;
    emu_mem_t memory_handle;

    // Program bytes read
    size_t words_read = 0;

    /* CPUTEST.COM and TST8080.COM assume they run on the CP/M OS for the i8080.
     * So this emulator has support for some basic CP/M BDOS emulation, and WBOOT. */
    
    if (memory_init(&memory_handle)) {

        // Read file into memory after CP/M reserved space
        words_read = memory_load(test_file_location, memory_handle.mem, CPM_START_OF_TPA);

        // File read error
        if (words_read == 0) {
            goto boot_failure;
        }

        // Memory load was successful, save reference to memory stream
        MEMORY = memory_handle.mem;

        // Initialize i8080 and setup default memory + IO streams
        emu_init_i8080(&cpu);
        emu_setup_streams(&cpu, rw_from_memory, ww_to_memory, cpm_rw_from_stdin, cpm_ww_to_stdout);

        // Set up BDOS and WBOOT emulation for CP/M
        emu_set_cpm_env(&cpu);

        // Begin the emulator.
        emu_runtime(&cpu, &memory_handle);

    } else {
        goto boot_failure;
    }

    emu_cleanup(&cpu, &memory_handle);    
    return EXIT_SUCCESS;
    
    // Failed to boot. Cleanup and exit.
    boot_failure:
    printf("Emulator boot failure.");
    emu_cleanup(&cpu, &memory_handle);
    return EXIT_FAILURE;
}