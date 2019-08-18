
#include <stdio.h>
#include <stdlib.h>
#include "src/emu.h"

// Emulator main memory, 64KB
static emu_word_t MEMORY[ADDR_T_MAX + 1];

// Default memory read/write streams
static emu_word_t rw_from_memory(emu_addr_t addr) { return MEMORY[addr]; }
static void ww_to_memory(emu_addr_t addr, emu_word_t word) { MEMORY[addr] = word; }

// CPU port out for the CP/M environment.
static void cpm_env_port_out(emu_addr_t addr, emu_word_t word) {
    // Address is duplicated, pick lower 8 bits
    emu_word_t port_addr = (emu_word_t)(addr & WORD_T_MAX);
    if (port_addr == CPM_CONSOLE_ADDR) {
        printf(WORD_T_PRT_FORMAT, word);
        // Some OSs buffer stdout, flush this buffer so that future scanfs
        // don't read from this buffer but from stdin instead
        fflush(stdout);
    } 
}

// CPU port in for the CP/M environment.
static emu_word_t cpm_env_port_in(emu_addr_t addr) {
    emu_word_t rw = 0x00;
     // Address is duplicated, pick lower 8 bits
    emu_word_t port_addr = (emu_word_t)(addr & WORD_T_MAX);
    if (port_addr == CPM_CONSOLE_ADDR) {
        scanf(WORD_T_PRT_FORMAT, &rw);
    }
    return rw;
}

static emu_word_t i8080_interrupt_handler(void) {
    return 0xc7; // RST 0
}

int main(int argc, char ** argv) {
    
    const char * test_file_location = (argc > 1) ? argv[1] : NULL;
    
    if (test_file_location == NULL) {
        printf("No file provided.");
        goto boot_failure;
    }
    
    // Make an emulated i8080
    i8080 cpu;
    
    // Read file into memory after CP/M reserved space
    if (memory_load(test_file_location, MEMORY, CPM_START_OF_TPA) == 0) goto boot_failure;
    
    // Initialize i8080 and setup default memory + IO streams, interrupt handler
    emu_init_i8080(&cpu);
    cpu.read_memory = rw_from_memory;
    cpu.write_memory = ww_to_memory;
    cpu.port_in = cpm_env_port_in;
    cpu.port_out = cpm_env_port_out;
    cpu.interrupt_acknowledge = i8080_interrupt_handler;

    // Set up BDOS and WBOOT emulation for CP/M
    emu_set_cpm_env(&cpu);

    // Begin the emulator.
    EMU_EXIT_CODE emu_exit_code = emu_main_runtime(&cpu, 0);
    // Destroy i8080 when emulator quits.
    i8080_destroy(&cpu);
    
    if (emu_exit_code == EMU_EXIT_SUCCESS) return EXIT_SUCCESS;
    else return EXIT_FAILURE;
    
    // Failed to boot emulator
    boot_failure: return EXIT_FAILURE;  
}