
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "src/cpu.h"
#include "utilities.h"

#define MAX_INPUT_LENGTH 1024

// The contents of the ROM, loaded to memory
uint8_t * ROM;
// The global ptr to the CPU emulator
cpu * CPU;

void dispatch_instr_exec(uint8_t instr);

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
    long long int ROM_SIZE = ftell(rom_fptr);
    rewind(rom_fptr);
    
    // Read the entire file into memory
    ROM = (uint8_t *)malloc(sizeof(uint8_t) * (ROM_SIZE + 1));
    fread(ROM, 1, ROM_SIZE, rom_fptr);
    // End the string and close the file
    ROM[ROM_SIZE] = '\0';
    fclose(rom_fptr);
    
    // --------------------- Create a new cpu --------------------
    CPU = (struct cpu *) malloc(sizeof (cpu));
    
    for (long long int i = 0; i < ROM_SIZE; ++i) {
        // Load the instruction into a buffer
        CPU->instr_buf = ROM[i];
    }
    
    return 0;
}

void dispatch_instr_exec(uint8_t instr) {
    
}