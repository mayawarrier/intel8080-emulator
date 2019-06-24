
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "src/cpu.h"
#include "utilities.h"

#define MAX_INPUT_LENGTH 1024

// The contents of the ROM, loaded to memory
uint8_t * ROM;
// The global ptr to the CPU emulator
cpu * CPU;

// initialize all the registers, interrupts etc
void create_cpu(cpu * cpu, peripherals * peripherals);
// The main cpu runtime
void start_cpu(cpu * cpu);
// destroys the cpu
void free_cpu(cpu * cpu);
// create some common peripherals like RAM
peripherals* create_peripherals(uint16_t num_bytes_memory);
// copies a program into the main memory from start_loc. Returns true if succeeded.
bool load_program(uint8_t * program, uint16_t program_size, 
        uint8_t * memory, uint16_t memory_size, uint16_t start_loc);
// dispatches execution of instructions, and updates flags
void dispatch_instr_exec(cpu * cpu, uint16_t instr_addr);

int main(void) {
    
    FILE * rom_fptr;
    char * rom_loc = malloc(sizeof(char) * MAX_INPUT_LENGTH);
    
    printf("Enter ROM location: ");
    get_string_safely(rom_loc, MAX_INPUT_LENGTH);
    
    rom_fptr = fopen(rom_loc, "rb");
    
    if (rom_fptr == NULL) {
        printf("ROM file read error, quitting.");
        return -1;
    } else {
        printf("ROM file found. Loading to memory...\n");
        free(rom_loc);
    }
    
    // Get the file length
    fseek(rom_fptr, 0, SEEK_END);
    uint16_t ROM_SIZE = ftell(rom_fptr);
    rewind(rom_fptr);
    
    // Read the entire file into memory
    ROM = (uint8_t *)malloc(sizeof(uint8_t) * (ROM_SIZE + 1));
    fread(ROM, 1, ROM_SIZE, rom_fptr);
    // End the string and close the file
    ROM[ROM_SIZE] = '\0';
    fclose(rom_fptr);
    
    // --------------------- Create a new cpu --------------------
    
    CPU = (struct cpu *) malloc(sizeof (cpu));
    
    // Initialize common peripherals
    peripherals * peripherals = create_peripherals((uint16_t)HIGHEST_ADDR);
    
    if (load_program(ROM, ROM_SIZE, peripherals->memory, peripherals->memory_size, 0x0)) {
        // program loading succeeded
        create_cpu(CPU, peripherals);
        start_cpu(CPU);
    } else {
        // ran past end of memory
        free_cpu(CPU);
    }
    
    return 0;
}

void dispatch_instr_exec(cpu * cpu, uint16_t instr_addr) {
    
}

void create_cpu(cpu * cpu, peripherals * peripherals) {
    // blank the registers
    cpu->A = cpu->B = cpu->C = cpu->D = cpu->E = cpu->H = cpu->L = 0x0;
    
    // initialize stack, peripherals
    cpu->peripherals = peripherals;
    if (peripherals != NULL) {
        cpu->SP = peripherals->memory_size - 1;
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

peripherals* create_peripherals(uint16_t num_bytes_memory) {
    peripherals * peripherals = (struct peripherals *)malloc(sizeof(peripherals));
    // Allocate num_bytes_mem bytes of working CPU memory
    peripherals->memory = (uint8_t *)malloc(sizeof(uint8_t) * num_bytes_memory);
    peripherals->memory_size = num_bytes_memory;
    return peripherals;
}

bool load_program(uint8_t * program, uint16_t program_size, 
        uint8_t * memory, uint16_t memory_size, uint16_t loc) {
    // copy the program into memory, making sure not to go over bounds
    for (uint16_t i = 0; i < program_size; ++i) {
        if (loc < memory_size) {
            memory[loc++] = program[i];
        } else {
            return false;
        }
    }
    return true;
}