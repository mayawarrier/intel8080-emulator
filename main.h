/* 
 * File:   main.h
 * Author: dhruvwarrier
 *
 * Created on June 24, 2019, 12:48 AM
 */

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include "src/cpu.h"

// initialize all the registers, interrupts etc
void create_cpu(cpu * cpu, peripherals * peripherals);
// The main cpu runtime
void start_cpu(cpu * cpu);
// destroys the cpu
void free_cpu(cpu * cpu);
// create some common peripherals like RAM
peripherals* create_peripherals(uint16_t highest_memory_addr);
// copies a program into the main memory from start_loc. Returns true if succeeded.
bool load_program(uint8_t * program, uint16_t program_size, 
        uint8_t * memory, uint16_t highest_memory_addr, uint16_t start_loc);
// dispatches execution of instructions, and updates flags
void dispatch_instr_exec(cpu * cpu, uint16_t instr_addr);

#endif /* MAIN_H */

