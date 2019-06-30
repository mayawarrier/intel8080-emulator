/*
 * Implement emu.h
 */

#include <stdio.h>
#include <string.h>
#include "emu.h"
#include "i8080_opcodes.h"

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
}