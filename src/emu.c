/*
 * Implement emu.h
 */

#include <stdio.h>
#include <string.h>
#include "emu.h"
#include "emu_debug.h"
#include "i8080/i8080_internal.h"
#include "i8080/i8080_opcodes.h"

#define DEFAULT_START_OF_PROGRAM_MEMORY 0x40

// Jumps to start of program memory
static const word_t DEFAULT_BOOTLOADER[] = {
    JMP,
    0x00,
    DEFAULT_START_OF_PROGRAM_MEMORY
};

static const int DEFAULT_BOOTLOADER_SIZE = 3;

// Reserved locations for interrupt vector table
static const addr_t VECTOR_TABLE_RESERVED_LOCATIONS[] = {
    0x00, // RESET, RST 0
    0x08, // RST 1
    0x10, // RST 2
    0x18, // RST 3
    0x20, // RST 4
    0x28, // RST 5
    0x30, // RST 6
    0x38 // RST 7
};

static const int NUM_VECTOR_TABLE_RESERVED_LOCATIONS = 8;

bool memory_init(mem_t * const memory_handle) {
    // Allocate memory
    memory_handle->mem = (word_t *)malloc(sizeof(word_t) * (ADDR_T_MAX + 1));
    
    if (memory_handle->mem == NULL) {
        printf("Error: Could not allocate enough memory to emulate.");
        return false;
    }
    
    // Zero out the entire memory
    memset((void *)memory_handle->mem, 0, ADDR_T_MAX + 1);
    // set size
    memory_handle->highest_addr = ADDR_T_MAX;
    
    return true;
}

addr_t memory_setup_rom(mem_t * const memory_handle) {
    return memory_setup_rom_custom(memory_handle, DEFAULT_BOOTLOADER, DEFAULT_BOOTLOADER_SIZE);
}

addr_t memory_setup_rom_custom(mem_t * const memory_handle, const word_t * bootloader, int bootloader_size) {
    
    if (bootloader_size > 8) {
        printf("Error: bootloader too large.");
        return ADDR_T_MAX;
    }
    
    // Copy the bootloader into memory, this is executed on RESET
    memcpy((void *)memory_handle->mem, (const void *)bootloader, bootloader_size);
    
    // Create the rest of the interrupt vector table after the RESET vector
    for (int i = 1; i < NUM_VECTOR_TABLE_RESERVED_LOCATIONS; ++i) {
        // HLT for all interrupts
        memory_handle->mem[VECTOR_TABLE_RESERVED_LOCATIONS[i]] = HLT;
    }
    
    return DEFAULT_START_OF_PROGRAM_MEMORY;
}

size_t load_file(const char * file_loc, word_t * memory, addr_t start_loc) {
    
    size_t file_size = 0;
    
    FILE * f_ptr = fopen(file_loc, "rb");
    
    if (f_ptr == NULL) {
        printf("Error: file cannot be opened.");
        return SIZE_MAX;
    }
    
    // get the file size
    fseek(f_ptr, 0, SEEK_END);
    file_size = ftell(f_ptr);
    rewind(f_ptr);
    
    // check if it can fit into memory
    if (file_size + start_loc > ADDR_T_MAX + 1) {
        printf("Error: file too large.");
        return SIZE_MAX;
    }
    
    // Attempt to read the entire file
    size_t words_read = fread(&memory[start_loc], sizeof(word_t), file_size, f_ptr);
    
    if (words_read != file_size) {
        printf("Error: file read failure.");
        return SIZE_MAX;
    }
    
    printf("Success: %zd words read.", words_read);
    
    fclose(f_ptr);
    return words_read;
}

bool emu_runtime(mem_t * const memory) {
    
    i8080 cpu;
    i8080_reset(&cpu);
    
    dump_memory(memory->mem, memory->highest_addr);
}