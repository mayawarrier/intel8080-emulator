
#include "src/emu.h"
#include "src/emu_consts.h"
#include "src/i8080/internal/i8080_consts.h"
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <thread>
#include <chrono>

// Emulator main memory, 64KB
static emu_word_t MEMORY[ADDR_T_MAX + 1];

// Default memory read/write streams
static emu_word_t rw_from_memory(emu_addr_t addr) { 
    return MEMORY[addr]; 
}
static void ww_to_memory(emu_addr_t addr, emu_word_t word) { 
    MEMORY[addr] = word; 
}

// CPU port out for the CP/M environment.
static void cpm_env_port_out(emu_addr_t addr, emu_word_t word) {
    // Address is duplicated, pick lower 8 bits
    emu_word_t port_addr = (emu_word_t)(addr & WORD_T_MAX);
    if (port_addr == CPM_CONSOLE_ADDR) {
        printf(WORD_T_ASCII_FORMAT, word);
    }
}

// CPU port in for the CP/M environment.
static emu_word_t cpm_env_port_in(emu_addr_t addr) {
    emu_word_t rw = 0x00;
    // Address is duplicated, pick lower 8 bits
    emu_word_t port_addr = (emu_word_t)(addr & WORD_T_MAX);
    if (port_addr == CPM_CONSOLE_ADDR) {
        scanf(WORD_T_ASCII_FORMAT, &rw);
    }
    return rw;
}

static emu_word_t i8080_interrupt_handler(void) {
    return 0xc7; // RST 0, this exits to WBOOT in the CP/M environment
}

int main(int argc, char ** argv) {

    const char * test_file_location = "../tests/test_interrupts.COM";

    if (test_file_location == NULL) {
        printf("Error: No file provided.\n");
        return EXIT_FAILURE;
    }

    // Make an emulated i8080
    i8080 cpu;
    std::atomic<emu_exit_code_t> ON_EMU_EXIT;

    // Initialize i8080 and setup default memory + IO streams, interrupt handler
    emu_init_i8080(&cpu);
    cpu.read_memory = rw_from_memory;
    cpu.write_memory = ww_to_memory;
    cpu.port_in = cpm_env_port_in;
    cpu.port_out = cpm_env_port_out;
    cpu.interrupt_acknowledge = i8080_interrupt_handler;

    // Read file into memory after CP/M reserved space
    if (!memory_check_errors(&cpu, 0, ADDR_T_MAX, 0) || memory_load(test_file_location, MEMORY, CPM_START_OF_TPA) == 0)
        return EXIT_FAILURE;

    // Set up BDOS and WBOOT emulation for CP/M
    emu_set_cpm_env(&cpu);

    // Begin the emulator and an interrupt generator
    std::thread emu_runtime_thread([&] {
        ON_EMU_EXIT = emu_runtime(&cpu, NULL);
    });
    std::thread intgen_thread([&] {
        // sleep for 10s then send an interrupt
        std::this_thread::sleep_for(std::chrono::seconds(10));
        i8080_interrupt(&cpu);
    });

    emu_runtime_thread.join();
    intgen_thread.join();

    if (ON_EMU_EXIT == EMU_EXIT_CODE::EMU_EXIT_SUCCESS) return EXIT_SUCCESS;
    else return EXIT_FAILURE;
}