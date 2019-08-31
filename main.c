
#include "src/emu.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

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

struct emu_runtime_args {
    i8080 * const cpu;
    _Bool perform_startup_check;
    emu_debug_args * const debug_args;
    EMU_EXIT_CODE exit_code;
};

static void * emu_runtime_thread(void * args) { 
    struct emu_runtime_args * emu_args = (struct emu_runtime_args *)args;
    // Begin the emulator.
    emu_args->exit_code = emu_runtime(emu_args->cpu, emu_args->perform_startup_check, emu_args->debug_args);
    // Destroy i8080 when emulator quits.
    i8080_destroy(emu_args->cpu);
    return NULL;
}

static void * interrupt_gen_thread(void * args) {
    /* Mark this as cancel-able so when the emulator quits
     * it can cancel this thread. */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    i8080 * const cpu = (i8080 * const)args;
    
    // Wait for 10 seconds and send an interrupt
    unsigned int return_time = time(0) + 10;
    while(time(0) < return_time);
    i8080_interrupt(cpu);
    
    // Freeze, and wait to be canceled
    while(1);
}

static emu_word_t i8080_interrupt_handler(void) {
    return 0xc7; // RST 0, this exits to WBOOT in the CP/M environment
}

int main(int argc, char ** argv) {
    
    const char * test_file_location = (argc > 1) ? argv[1] : NULL;
    
    if (test_file_location == NULL) {
        printf("Error: No file provided.\n");
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

    // Start the emulator and an interrupt generator on different threads
    pthread_t emu_thread, intgen_thread;    
    struct emu_runtime_args emu_args = {.cpu = &cpu, .perform_startup_check = 0, .debug_args = NULL};
    
    pthread_create(&emu_thread, NULL, &emu_runtime_thread, (void *)&emu_args);
    pthread_create(&intgen_thread, NULL, &interrupt_gen_thread, (void *)&cpu);
    
    pthread_join(emu_thread, NULL);
    pthread_cancel(intgen_thread);
    // Emu runtime is over
    
    if (emu_args.exit_code == EMU_EXIT_SUCCESS) return EXIT_SUCCESS;
    else return EXIT_FAILURE;
    
    // Failed to boot emulator
    boot_failure: return EXIT_FAILURE;  
}