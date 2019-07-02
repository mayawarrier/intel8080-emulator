/*
 * Implement emu.h
 */

#include <stdio.h>
#include <string.h>
#include "emu.h"
#include "emu_debug.h"
#include "i8080/i8080_internal.h"
#include "i8080/i8080_opcodes.h"

#define UNUSED(expr) (void)(expr)

// Largest amount of memory addressable
static word_t * MEMORY;

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
    
    // Keep reference to memory stream
    MEMORY = memory_handle->mem;
    
    return true;
}

addr_t memory_setup_IVT(mem_t * const memory_handle) {
    
    // Create the interrupt vector table
    for (int i = 0; i < NUM_VECTOR_TABLE_RESERVED_LOCATIONS; ++i) {
        // HLT for all interrupts
        memory_handle->mem[VECTOR_TABLE_RESERVED_LOCATIONS[i]] = HLT;
    }
    
    return DEFAULT_START_OF_PROGRAM_MEMORY;
}

bool memory_write_bootloader(mem_t * const memory_handle, const word_t * bootloader, size_t bootloader_size) {
    
    if (bootloader_size > 8) {
        printf("Error: bootloader too large.");
        return false;
    }
    
    // Copy the bootloader into memory, this is executed on RESET
    memcpy((void *)memory_handle->mem, (const void *)bootloader, bootloader_size);
    
    return true;
}

void memory_write_bootloader_default(mem_t * const memory_handle) {
    memory_write_bootloader(memory_handle, DEFAULT_BOOTLOADER, DEFAULT_BOOTLOADER_SIZE);
}

size_t memory_load(const char * file_loc, word_t * memory, addr_t start_loc) {
    
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

static word_t rw_from_memory(addr_t addr) {
    return MEMORY[addr];
}

static void ww_to_memory(addr_t addr, word_t word) {
    MEMORY[addr] = word;
}

static void ww_to_stdout(addr_t addr, word_t word) {
    // don't need the port address to write to stdout
    UNUSED(addr);
    printf(WORD_T_FORMAT, word);
}

static word_t rw_from_stdin(addr_t addr) {
    // don't need the port address to read from stdin
    UNUSED(addr);
    word_t rw;
    scanf(WORD_T_FORMAT, &rw);
    return rw;
}

void emu_init_i8080(i8080 * const cpu) {
    i8080_reset(cpu);
    cpu->read_memory = NULL;
    cpu->write_memory = NULL;
    cpu->port_in = NULL;
    cpu->port_out = NULL;
}

bool emu_setup_streams(i8080 * const cpu, read_word_fp read_memory, write_word_fp write_memory, 
        read_word_fp port_in, write_word_fp port_out) {
    
    if (read_memory == NULL || write_memory == NULL) {
        printf("Error. Memory stream functions NULL.");
        return false;
    }
    
    if (port_in == NULL || port_out == NULL) {
        printf("Warning: I/O stream functions NULL.");
    }
    
    cpu->read_memory = read_memory;
    cpu->write_memory = write_memory;
    cpu->port_in = port_in;
    cpu->port_out = port_out;
    
    return true;
}

void emu_setup_streams_default(i8080 * const cpu) {
    emu_setup_streams(cpu, rw_from_memory, ww_to_memory, rw_from_stdin, ww_to_stdout);
}

bool emu_runtime(i8080 * const cpu, mem_t * const memory) {
    
    // debug
    dump_memory(memory->mem, memory->highest_addr);
}