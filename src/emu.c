/*
 * Implement emu.h
 */

#include <string.h>
#include "emu.h"
#include "emu_debug.h"
#include "i8080/i8080_opcodes.h"

// The console port address duplicated across 16-bit address bus for use with port out
static const emu_addr_t CONSOLE_ADDR_FULL = (emu_addr_t)((CPM_CONSOLE_ADDR << HALF_ADDR_SIZE) | CPM_CONSOLE_ADDR);

size_t memory_load(const char * file_loc, emu_word_t * memory, emu_addr_t start_loc) {
    
    size_t file_size = 0;
    
    FILE * f_ptr = fopen(file_loc, "rb");
    
    if (f_ptr == NULL) {
        printf("Error: file cannot be opened.\n");
        goto failure;
    }
    
    // get the file size
    fseek(f_ptr, 0, SEEK_END);
    file_size = ftell(f_ptr);
    rewind(f_ptr);
    
    // check if it can fit into memory
    if (file_size + start_loc > ADDR_T_MAX + 1) {
        printf("Error: file too large.\n");
        goto failure;
    }
    
    // Attempt to read the entire file
    size_t words_read = fread(&memory[start_loc], sizeof(emu_word_t), file_size, f_ptr);
    
    if (words_read != file_size) {
        printf("Error: file read failure.\n");
        goto failure;
    }
    
    fclose(f_ptr);
    return words_read;
    
    failure: {
        fclose(f_ptr);
        return 0;
    }
}

// Prints the string at str_addr to CPM console, terminated by '$'
static void cpm_print_str(i8080 * const cpu, emu_addr_t str_addr) {
    // Print each character until we hit a '$'
    while (1) {
        emu_word_t str_char = cpu->read_memory(str_addr);
        if (str_char == '$') {
            break;
        } else {
            // cpu->port_out can decide how to format the characters
            cpu->port_out(CONSOLE_ADDR_FULL, str_char);
            str_addr++;
        }
    }
}

// Provides limited emulation of CP/M BDOS and zero page. At the moment, this only works with BDOS op 2 and 9, and WBOOT.
// WBOOT launches into a simple command processor so programs can be run and the emulator can quit.
static _Bool i8080_cpm_zero_page(void * const udata) {
    
    i8080 * const cpu = (i8080 * const)udata;

    // The return address on the stack
    emu_addr_t ret_addr = (cpu->read_memory(cpu->sp + (emu_addr_t)1) << HALF_ADDR_SIZE) | cpu->read_memory(cpu->sp);
    
    switch(ret_addr) {
        
        case 0xe401:
        {
            // This is a basic command processor, entered upon WBOOT. Commands are: RUN addr, and QUIT.
            // RUN addr starts executing instructions from addr. addr should be in 16-bit hex format eg. 0x0000 or 0x1234
            // QUIT quits the emulator.
            
            // Addresses of command proc messages
            const emu_addr_t ERROR_ADDR_PTR = 0x40;                     // "\nInvalid address."
            const emu_addr_t ERROR_CMD_PTR = ERROR_ADDR_PTR + 0x18;     // "\nInvalid command."
            const emu_addr_t CMD_PROMPT_PTR = ERROR_CMD_PTR + 0x18;     // "> "
            
            // Max length, excluding trailing null
            int LEN_INPUT_BUF = 127;
            emu_word_t input_buf[LEN_INPUT_BUF + 1];
            
            // Address from RUN command
            emu_addr_t run_addr;
            
            // Keep getting input from the console until a valid command is entered
            while(1) {
                // Prints a prompt "> "
                cpm_print_str(cpu, CMD_PROMPT_PTR);
                // reset buffer
                int buf_loc = 0;
                
                // Get console input till end of line
                while (buf_loc != LEN_INPUT_BUF) {
                    emu_word_t ch = cpu->port_in(CONSOLE_ADDR_FULL);
                    if (ch == '\n') break;
                    input_buf[buf_loc++] = ch;
                }
                input_buf[buf_loc] = '\0';

                // Process commands
                if (strncmp(input_buf, "RUN ", 4) == 0) {
                    // ADDR_T_FORMAT is %#06x, # formats for 0x in every case except 0x0000
                    if (strncmp(&input_buf[4], "0x0000", 6) == 0) {
                        run_addr = 0x0000;
                        goto command_run;
                    } else if (sscanf(&input_buf[4], ADDR_T_FORMAT, &run_addr) == 1
                            && run_addr >= 0 && run_addr <= 0xffff) {
                        // address is in correct format and within bounds
                        goto command_run;
                    } else {
                        // Invalid address error
                        cpm_print_str(cpu, ERROR_ADDR_PTR);
                    }
                } else if (strncmp(input_buf, "QUIT", 4) == 0) {
                    goto command_quit;
                } else {
                    // Invalid command error
                    cpm_print_str(cpu, ERROR_CMD_PTR);
                }
                // Print a newline
                cpu->port_out(CONSOLE_ADDR_FULL, '\n');
            }
            
            command_run: {
                // Write JMP addr to the bytes immediately after
                emu_word_t lo_addr = (emu_word_t)(run_addr & WORD_T_MAX);
                emu_word_t hi_addr = (emu_word_t)((run_addr >> HALF_WORD_SIZE) & WORD_T_MAX);
                cpu->write_memory(0xe401, JMP);
                cpu->write_memory(0xe402, lo_addr);
                cpu->write_memory(0xe403, hi_addr);
                return 1;
            }
            
            command_quit: return 0;
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
                    cpm_print_str(cpu, str_addr);
                    break;
                }

                case 2: 
                {
                    // Writes the character in register E
                    cpu->port_out(CONSOLE_ADDR_FULL, cpu->e);
                    break;
                }
            }
            
            return 1;
        }
        
        default: return 1;
    }
}

static const char INIT_MEM_STREAM_ERR_MSG[] = "Error: Cannot write to memory. Initialize emulator memory streams first.\n";

void emu_set_cpm_env(i8080 * const cpu) {
    // 0x38 is a special instruction that calls
    // an external fn outside the i8080 runtime.
    // This is used to emulate calls to CP/M BDOS and WBOOT, and
    // provide a basic command processor.
    
    if (cpu->write_memory != NULL) {
        // Entry for CP/M BDOS is 0x05.
        cpu->write_memory(0x05, EMU_EXT_CALL);
        cpu->write_memory(0x06, RET);

        // Entry to CP/M WBOOT (warm boot). Jumps to command processor,
        // which lies at 0xe400 for 64K machines
        cpu->write_memory(0x00, JMP);
        cpu->write_memory(0x01, 0x00);
        cpu->write_memory(0x02, 0xe4);
        
        // External call to a simple command processor
        cpu->write_memory(0xe400, EMU_EXT_CALL);
        
        // Command processor messages, '$'-terminated
        // as is the convention in CP/M.
        char * const CMD_MSGS[] = {"\nInvalid address.$", "\nInvalid command.$", "> $"};
        
        // Store all messages after the standard IVT
        emu_addr_t msgs_loc = 0x40;
        for (int i = 0; i < 3; ++i) {
            char * msg = CMD_MSGS[i];
            for (int j = 0; j < strlen(msg); ++j) {
                cpu->write_memory(msgs_loc++, msg[j]);
            }
        }
        
        // HLT for the rest of the interrupts
        for (int i = 1; i < NUM_IVT_VECTORS; ++i) {
            cpu->write_memory(INTERRUPT_TABLE[i], HLT);
        }

        cpu->emu_ext_call = i8080_cpm_zero_page;
    } else {
        printf(INIT_MEM_STREAM_ERR_MSG);
    }
}

// Jumps to start of program memory
static const emu_word_t DEFAULT_BOOTLOADER[] = {
    JMP,
    DEFAULT_START_PA,
    0x00
};

static const int DEFAULT_BOOTLOADER_SIZE = 3;

void emu_set_default_env(i8080 * const cpu) {
    
    if (cpu->write_memory != NULL) {
        // Create the interrupt vector table
        // Do not write to RST 1 sequence, we'll put our bootloader there instead
        for (int i = 1; i < NUM_IVT_VECTORS; ++i) {
            // HLT for all interrupts
            cpu->write_memory(INTERRUPT_TABLE[i], HLT);
        }

        // Write a default bootloader that jumps to start of program memory
        emu_addr_t wr_addr = INTERRUPT_TABLE[0]; // start at 0x0

        for (int i = 0; i < DEFAULT_BOOTLOADER_SIZE; ++i) {
            cpu->write_memory(wr_addr, DEFAULT_BOOTLOADER[i]);
            wr_addr++;
        }
    } else {
        printf(INIT_MEM_STREAM_ERR_MSG);
    }
}

void emu_init_i8080(i8080 * const cpu) {
    i8080_reset(cpu);
    cpu->read_memory = NULL;
    cpu->write_memory = NULL;
    cpu->port_in = NULL;
    cpu->port_out = NULL;
    cpu->emu_ext_call = NULL;
    // interrupts disabled by default
    cpu->ie = 0;
}

// Writes test_word to all locations, then reads test_word from all locations.
// Returns 0 if a read failed to return test_word, and stores the failed location in cpu->pc.
static _Bool memory_write_read_pass(i8080 * const cpu, emu_word_t test_word) {
    // Write pass
    for (emu_addr_t i = 0; i <= ADDR_T_MAX; ++i) {
        cpu->write_memory(i, test_word);
    }
    // Read pass
    for (emu_addr_t i = 0; i <= ADDR_T_MAX; ++i) {
        if (cpu->read_memory(i) != test_word) {
            // indicate to user which location failed
            cpu->pc = i;
            return 0;
        }
    }
    return 1;
}

static EMU_EXIT_CODE emu_runtime(i8080 * const cpu, _Bool perform_startup_check, 
        _Bool debug_mode, FILE * debug_out, const char mem_dump_format[], int mem_dump_newline_after) {
    
    if (cpu->read_memory == NULL || cpu->write_memory == NULL) {
        return EMU_ERR_MEM_STREAMS;
    }
    
    // Check if all memory locations are read/write-able
    if (perform_startup_check) {
        // Pass 1
        if (!memory_write_read_pass(cpu, 0x55)) return EMU_ERR_MEMORY;
        // Pass 2, check alternate bits
        if (!memory_write_read_pass(cpu, 0xAA)) return EMU_ERR_MEMORY;
    }
    
    // If in debug mode, this is overridden by i8080_debug_next
    _Bool (* i8080_next_ovrd)(i8080 * const) = i8080_next;
    
    if(debug_mode) {
        set_debug_next_options(debug_out, mem_dump_format, mem_dump_newline_after);
        i8080_next_ovrd = i8080_debug_next;
    }
    
    // Execute all instructions until failure/quit
    while(1) {
        if (!i8080_next_ovrd(cpu)) {
            break;
        }
    }
    
    if (cpu->last_instr_exec == IN || cpu->last_instr_exec == OUT) {
        return EMU_ERR_IO_STREAMS;
    }
    
    return EMU_EXIT_SUCCESS;
}

EMU_EXIT_CODE emu_main_runtime(i8080 * const cpu, _Bool perform_startup_check) {
    return emu_runtime(cpu, perform_startup_check, 0, NULL, "", 0);
}

EMU_EXIT_CODE emu_debug_runtime(i8080 * const cpu, _Bool perform_startup_check, 
        FILE * debug_out, const char mem_dump_format[], int mem_dump_newline_after) {
    return emu_runtime(cpu, perform_startup_check, 1, debug_out, mem_dump_format, mem_dump_newline_after);
}