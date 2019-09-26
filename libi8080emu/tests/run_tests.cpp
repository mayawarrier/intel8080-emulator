/*
 * Run all the tests for libi8080emu and pretty-print them to console.
 * This shows many examples on how to use libi8080emu, and some documentation on setup.
 */

#include "../include/emu.h"
#include "../include/emu_consts.h"
#include "../libi8080/include/i8080_consts.h"

#include <cstdio> // for easy formatted printing
#include <cstring> // memset
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>

// 64KB of emulated memory
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

// Minimum required calls to set up i8080 emulator.
i8080 init_i8080_emu_default() {
    i8080 cpu;
    /*
       Clears interrupt bit, pending interrupts, HLT,
       memory+I/O streams, cycles_taken, and resets PC.
       Registers, stack pointer and flags are not initialized/cleared,
       the program is expected to initialize them by itself.
    */
    emu_init_i8080(&cpu);
    // Assign memory stream fns
    cpu.read_memory = rw_from_memory;
    cpu.write_memory = ww_to_memory;
    /*
       I/O stream fns can also be assigned:
       cpu.port_in = <i8080.h/emu_read_word_fp>;
       cpu.port_out = <i8080.h/emu_write_word_fp>;
       Assigning these are not necessary but I/O
       requests will cause the emulator to quit.

       Hardware interrupts can also be enabled,
       if the build environment supports mutexes (see i8080_sync.h):
       cpu.interrupt_acknowledge = <emu_word_t(*)(void)>;
       interrupt_acknowledge() should return the word to be executed on interrupt.
       If not assigned, calls to i8080_interrupt() will be ignored.
    */
    /*
       Set default environment, with no CP/M BIOS support.
       This places an IVT at the first 64 bytes, and a boot sequence
       at RST 0 which jumps to 0x40.

       CP/M 2.2 environment can be set instead with emu_set_cpm_env(&cpu, <enable_cmd_proc>).
       This provides a simple command processor at CP/M WBOOT, and adds
       support for BDOS operations 2 and 9. To disable the command
       processor, set enable_cmd_proc = 0.
       - The first 0x100 bytes are reserved for the BIOS, and locations
         from 0xe400 onwards are reserved for CP/M (assuming a 64K machine).
    */
    emu_set_default_env(&cpu);
    return cpu;
}

// See init_i8080_emu_default for documentation
i8080 init_i8080_emu_cpm() {
    i8080 cpu;
    emu_init_i8080(&cpu);
    cpu.read_memory = rw_from_memory;
    cpu.write_memory = ww_to_memory;
    cpu.port_in = cpm_env_port_in;
    cpu.port_out = cpm_env_port_out;
    // Initialize in CPM mode, disable cmd processor since this is used for tests
    emu_set_cpm_env(&cpu, 0);
    return cpu;
}

int run_generic_test(i8080 * const cpu, const std::string & file_location, emu_addr_t program_load_loc) {
    // Load emulated memory with the file contents, then start the emulator
    if (memory_load(file_location.c_str(), MEMORY, program_load_loc) == 0) return 0;
    if (emu_runtime(cpu, NULL) == EMU_EXIT_CODE::EMU_EXIT_SUCCESS) return 1;
    else return 0;
}

int run_interrupted_test(i8080 * const cpu, const std::string & file_location, emu_addr_t program_load_loc, int ms_to_interrupt) {
    // Enable hardware interrupts
    cpu->interrupt_acknowledge = i8080_interrupt_handler;
    if (memory_load(file_location.c_str(), MEMORY, program_load_loc) == 0) return 0;

    std::atomic<emu_exit_code_t> on_emu_exit;

    // Begin the emulator and send an interrupt after a period of time
    std::thread emu_runtime_thread([&] {
        on_emu_exit = emu_runtime(cpu, NULL);
    });
    std::thread intgen_thread([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms_to_interrupt));
        i8080_interrupt(cpu);
    });

    emu_runtime_thread.join();
    intgen_thread.join();

    if (on_emu_exit == EMU_EXIT_CODE::EMU_EXIT_SUCCESS) return 1;
    else return 0;
}

int run_all_tests() {
    // test locations
    const std::string interrupts_test_fileloc = "tests/test_interrupts.COM";
    const std::string tst8080_test_fileloc = "tests/TST8080.COM";
    const std::string cputest_test_fileloc = "tests/CPUTEST.COM";

    // All tests are run in CP/M env
    i8080 cpu = init_i8080_emu_cpm();

    // ---------------- running tests ------------------------
    const int num_tests = 3;
    int num_tests_passed = 0;
    // Clears the entire TPA (till 0xe400). EEPROMS were commonly blanked with 0xff.
    const auto clear_cpm_tpa = []() {std::memset(&MEMORY[CPM_START_OF_TPA], 0xff, 0xe300);};

    clear_cpm_tpa();
    std::cout << "\n-------------------------- Test 1: TST8080.COM --------------------------\n";
    std::cout << "\t\t8080/8085 CPU Diagnostic, Kelly Smith, 1980";
    std::cout << "\n-------------------------------------------------------------------------\n";
    num_tests_passed += run_generic_test(&cpu, tst8080_test_fileloc, CPM_START_OF_TPA);
    std::cout << "\n-------------------------------------------------------------------------\n";

    clear_cpm_tpa();
    i8080_reset(&cpu);
    std::cout << "\n-------------------------- Test 2: CPUTEST.COM --------------------------\n";
    std::cout << "\tSupersoft Associates CPU test, Diagnostics II suite, 1981";
    std::cout << "\n-------------------------------------------------------------------------\n";
    num_tests_passed += run_generic_test(&cpu, cputest_test_fileloc, CPM_START_OF_TPA);
    std::cout << "\n-------------------------------------------------------------------------\n";

    clear_cpm_tpa();
    i8080_reset(&cpu);
    std::cout << "\n---------------------- Test 3: test_interrupts.COM ----------------------\n";
    std::cout << "\t\tPrints '+' in a loop for 0.2s, then returns to \n\t\tWBOOT upon receiving interrupt.";
    std::cout << "\n-------------------------------------------------------------------------\n";
    num_tests_passed += run_interrupted_test(&cpu, interrupts_test_fileloc, CPM_START_OF_TPA, 200);
    std::cout << "\n-------------------------------------------------------------------------\n";

    std::cout << "\n" << num_tests_passed << "/" << num_tests << " tests passed." << std::endl;
    i8080_destroy(&cpu);

    return (num_tests_passed == 3) ? 1 : 0;
}