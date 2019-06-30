
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "src/i8080.h"
#include "utilities.h"
#include "src/i8080_opcodes.h"

int main(int argc, char ** argv) {
    
    // --------------------- Create a new cpu --------------------
    
    cpu * CPU = (struct cpu *) malloc(sizeof (cpu));
    
    // Initialize common peripherals
    peripherals * peripherals = create_peripherals((uint16_t)HIGHEST_ADDR);
    
    if (load_program(ROM, ROM_SIZE, peripherals->memory, peripherals->highest_mem_addr, 0x0)) {
        // program loading succeeded
        create_cpu(CPU, peripherals);
        start_cpu(CPU);
    } else {
        // ran past end of memory
        free_cpu(CPU);
    }
    
    return EXIT_SUCCESS;
}

void dispatch_instr_exec(cpu * cpu, uint16_t instr_addr) {
    
}

void create_cpu(cpu * cpu, peripherals * peripherals) {
    // blank the registers    
    cpu->A = cpu->B = cpu->C = cpu->D = cpu->E = cpu->H = cpu->L = 0x0;
    
    // initialize stack, peripherals
    cpu->peripherals = peripherals;
    if (peripherals != NULL) {
        cpu->SP = peripherals->highest_mem_addr;
    }
    
    cpu->PC = 0x0;
    cpu->addr_buf = cpu->data_buf = cpu->instr_buf = 0x0;
    
    // initialize flags
    cpu->flags = 0x0;
    set_flag(cpu, FLAG_PARITY);
    set_flag(cpu, FLAG_ZERO);
}

void free_cpu(cpu * cpu) {
    free(cpu->peripherals->memory);
    cpu->peripherals->memory = NULL;
    free(cpu->peripherals);
    cpu->peripherals = NULL;
    free(cpu);
}

// The main cpu runtime
void start_cpu(cpu * cpu) {
    
}

bool init_memory(i8080_mem * const memory_handle) {
    // Allocate 64KB
    memory_handle->mem = (u8 *)malloc(sizeof(u8) * (HIGHEST_ADDR + 1));
    
    if (memory_handle->mem == NULL) {
        printf("Error: Could not allocate enough memory to emulate.");
        return false;
    }
    
    // Zero out the entire 64 KB
    memset((void *)memory_handle->mem, 0, HIGHEST_ADDR + 1);
    // set size
    memory_handle->highest_addr = HIGHEST_ADDR;
    
    return true;
}

u16 load_file(const char * file_loc, u8 * memory, u16 start_loc) {
    
    size_t file_size = 0;
    
    FILE * f_ptr = fopen(file_loc, "rb");
    
    if (f_ptr == NULL) {
        printf("Error: file cannot be opened.");
        return -1;
    }
    
    // get the file size
    fseek(f_ptr, 0, SEEK_END);
    file_size = ftell(f_ptr);
    rewind(f_ptr);
    
    // check if it can fit into memory
    if (file_size + start_loc > HIGHEST_ADDR + 1) {
        printf("Error: file too large.");
        return -1;
    }
    
    // Attempt to read the entire file
    size_t bytes_read = fread(&memory[start_loc], sizeof(u8), file_size, f_ptr);
    
    if (bytes_read != file_size) {
        printf("Error: file read failure.");
        return -1;
    }
    
    printf("Success: %zd bytes read.", bytes_read);
    
    fclose(f_ptr);
    return (int)bytes_read;
}