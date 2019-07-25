
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

static void ww_to_stdout(emu_addr_t addr, emu_word_t word) {
    // don't need the port address to write to stdout
    (void) addr;
    printf(WORD_T_FORMAT, word);
}

static emu_word_t rw_from_stdin(emu_addr_t addr) {
    // don't need the port address to read from stdin
    (void) addr;
    emu_word_t rw;
    scanf(WORD_T_FORMAT, &rw);
    return rw;
}

// Provides some emulation of CP/M BDOS and zero page.
bool cpm_zero_page(void * const udata) {
    
    i8080 * const cpu = (i8080 * const)udata;

    // The return address on the stack
    emu_addr_t ret_addr = (cpu->read_memory(cpu->sp + (emu_addr_t)1) << HALF_ADDR_SIZE) | cpu->read_memory(cpu->sp);
    
    switch(ret_addr) {
        
        case 0x01: 
        {
            // This is a warm boot to CP/M. Quit the emulator for now until supported
            return false;
        }
        
        case 0x06: 
        {
            // This is a BDOS call
            // BDOS arg is stored in C
            int operation = cpu->c;

            switch(operation) {
                case 9:
                {
                    // Writes an output string terminated by '$'
                    // Address of string is {DE}
                    emu_addr_t str_addr = (emu_addr_t)((cpu->d << HALF_ADDR_SIZE) | cpu->e);

                    // Print each character until we hit a '$'
                    while (true) {
                        emu_word_t str_char = rw_from_memory(str_addr);
                        if (str_char == '$') {
                            break;
                        } else {
                            printf(WORD_T_PRT_FORMAT, str_char);
                            str_addr++;
                        }
                    }
                    break;
                }

                case 2: 
                {
                    // Writes the character in register E
                    printf(WORD_T_PRT_FORMAT, cpu->e);
                }
            }
            
            return true;
        }
        
        default: return true;
    }
    
    
}

// Sets up the environment for some basic CP/M 2.2 operating system emulation.
void set_cpm_env(i8080 * const cpu) {
    // 0x38 is a special instruction that calls
    // an external fn outside the emulator.
    // This is used to emulate a call to CP/M zero page fns.
    
    // Entry for CP/M BDOS is 0x05.
    ww_to_memory(0x05, 0x38);
    ww_to_memory(0x06, 0xc9);
    
    // Entry to CP/M WBOOT (warm boot)
    ww_to_memory(0x00, 0x38);
    ww_to_memory(0x01, 0xc9);
    
    cpu->emu_ext_call = cpm_zero_page;
}

int main(int argc, char ** argv) {

    // ---------------- Set up memory and boot emulator ------------------

    // CPU and main memory
    i8080 cpu;
    emu_mem_t memory_handle;

    // Program bytes read
    size_t words_read = 0;

    if (memory_init(&memory_handle)) {

        // Read file into memory after 256 bytes reserved by CP/M
        words_read = memory_load("tests/CPUTEST.COM", memory_handle.mem, 0x100);

        // File read error
        if (words_read == SIZE_MAX) {
            goto boot_failure;
        }

        // Memory load was successful, save reference to memory stream
        MEMORY = memory_handle.mem;

        // Initialize i8080 and setup default memory + IO streams
        emu_init_i8080(&cpu);
        emu_setup_streams(&cpu, rw_from_memory, ww_to_memory, rw_from_stdin, ww_to_stdout);

        set_cpm_env(&cpu);

        // Begin the emulator. Does not return until complete.
        emu_runtime(&cpu, &memory_handle);

    } else {
        goto boot_failure;
    }

    emu_cleanup(&cpu, &memory_handle);    
    return EXIT_SUCCESS;
    
    boot_failure:
    printf("Emulator boot failure.");
    emu_cleanup(&cpu, &memory_handle);
    return EXIT_FAILURE;
    
}