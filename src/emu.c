/*
 * Implement emu.h
 */

#include "emu.h"
#include "emu_consts.h"
#include "emu_debug.h"
#include "i8080/internal/i8080_opcodes.h"
#include "i8080/internal/i8080_consts.h"
#include <string.h>
#include <stdint.h>

typedef enum emu_env {
    CPM,
    DEFAULT
} emu_env_t;

// The current emulator environment set before runtime
static emu_env_t EMU_ENV = (enum emu_env)DEFAULT;

size_t memory_load(const char * file_loc, emu_word_t * memory, const emu_addr_t start_loc) {
    
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
    if (file_size + start_loc > ADDR_T_MAX + (size_t)1) {
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
static void cpm_print_str(const emu_addr_t console_addr, emu_addr_t str_addr, i8080 * const cpu) {
    // Print each character until we hit a '$'
    emu_word_t str_char;
    while (1) {
        str_char = cpu->read_memory(str_addr);
        if (str_char == '$') {
            break;
        } else {
            // cpu->port_out can decide how to format the characters
            cpu->port_out(console_addr, str_char);
            str_addr++;
        }
    }
}

// Provides limited emulation of CP/M BDOS and zero page. At the moment, this only works with BDOS op 2 and 9, and WBOOT.
// WBOOT launches into a simple command processor so programs can be run and the emulator can quit.
static int i8080_cpm_zero_page(void * const udata) {
    
    i8080 * const cpu = (i8080 * const)udata;

	// The console port address duplicated across 16-bit address bus for use with port out
    const emu_addr_t CONSOLE_ADDR_FULL = ((emu_addr_t)(CPM_CONSOLE_ADDR << HALF_ADDR_SIZE) | CPM_CONSOLE_ADDR);
    // The return address on the stack
    const emu_addr_t ret_addr = (emu_addr_t)(cpu->read_memory(cpu->sp + (emu_addr_t)1) << HALF_ADDR_SIZE) | cpu->read_memory(cpu->sp);
    
    switch(ret_addr) {
        
        case 0xe401:
        {
            // This is a basic command processor, entered upon WBOOT. Commands are: RUN addr, HELP, and QUIT.
            // RUN addr starts executing instructions from addr. addr should be in 16-bit hex format eg. 0x0000 or 0x1234
            // HELP shows the list of commands.
            // QUIT quits the emulator.
            
            // Addresses of command proc messages
            const emu_addr_t ERROR_ADDR_PTR = 0xe410;                   // "\nInvalid address."
            const emu_addr_t ERROR_CMD_PTR = ERROR_ADDR_PTR + 0x11;     // "\nInvalid command."
            const emu_addr_t HELP_MSG_PTR = ERROR_CMD_PTR + 0x11;		// Message shown on HELP
            const emu_addr_t CMD_PROMPT_PTR = HELP_MSG_PTR + 0x76;      // "> "
            
            // Max length, excluding trailing null
            #define LEN_INPUT_BUF 127
            emu_word_t input_buf[LEN_INPUT_BUF + 1];
            size_t buf_loc = 0;
            emu_word_t buf_ch;
            
            // Address from RUN command
            emu_addr_t run_addr;
            
            // Keep getting input from the console until a valid command is entered
            while(1) {
                // Prints a prompt "> "
                cpm_print_str(CONSOLE_ADDR_FULL, CMD_PROMPT_PTR, cpu);
                // reset buffer
                buf_loc = 0;
                
                // Get console input till end of line
                while (buf_loc != LEN_INPUT_BUF) {
                    buf_ch = cpu->port_in(CONSOLE_ADDR_FULL);
                    if (buf_ch == '\n') break;
                    input_buf[buf_loc++] = buf_ch;
                }
                input_buf[buf_loc] = '\0';

                // Process commands
                if (strncmp(input_buf, "RUN ", 4) == 0) {
                    if (sscanf(&input_buf[4], ADDR_T_SCN_FORMAT, &run_addr) == 1 && run_addr >= 0 && run_addr <= 0xffff) {
                        // address is in correct format and within bounds
                        #undef LEN_INPUT_BUF
                        goto command_run;
                    } else {
                        // Invalid address error
                        cpm_print_str(CONSOLE_ADDR_FULL, ERROR_ADDR_PTR, cpu);
                    }
                } else if (strncmp(input_buf, "QUIT", 4) == 0) {
                    #undef LEN_INPUT_BUF
                    goto command_quit;
                } else if (strncmp(input_buf, "HELP", 4) == 0) {
                    cpm_print_str(CONSOLE_ADDR_FULL, HELP_MSG_PTR, cpu);
                } else {
                    // Invalid command error
                    cpm_print_str(CONSOLE_ADDR_FULL, ERROR_CMD_PTR, cpu);
                }
                // Print a newline
                cpu->port_out(CONSOLE_ADDR_FULL, '\n');
            }
            
            command_run: {
                // Write JMP addr to the bytes immediately after
                const emu_word_t lo_addr = (emu_word_t)(run_addr & WORD_T_MAX);
                const emu_word_t hi_addr = (emu_word_t)((emu_addr_t)(run_addr >> HALF_ADDR_SIZE) & WORD_T_MAX);
                cpu->write_memory(0xe401, i8080_JMP);
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
            switch(cpu->c) {
                case 9:
                {
                    // Writes an output string terminated by '$'
                    // Address of string is {DE}
                    const emu_addr_t str_addr = ((emu_addr_t)(cpu->d << HALF_ADDR_SIZE) | cpu->e);
                    cpm_print_str(CONSOLE_ADDR_FULL, str_addr, cpu);
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

// Reserved locations for interrupt vector table
static const emu_addr_t INTERRUPT_TABLE[] = {
	0x00, // RESET, RST 0
	0x08, // RST 1
	0x10, // RST 2
	0x18, // RST 3
	0x20, // RST 4
	0x28, // RST 5
	0x30, // RST 6
	0x38  // RST 7
};

void emu_set_cpm_env(i8080 * const cpu) {

    // i8080_EMU_EXT_CALL is used to emulate calls to CP/M BDOS and WBOOT.
    
    if (cpu->write_memory != NULL) {
        // Entry for CP/M BDOS is 0x05.
        cpu->write_memory(0x05, i8080_EMU_EXT_CALL);
        cpu->write_memory(0x06, i8080_RET);

        // Entry to CP/M WBOOT (warm boot). Jumps to command processor,
        // which lies at 0xe400 for 64K machines
        cpu->write_memory(0x00, i8080_JMP);
        cpu->write_memory(0x01, 0x00);
        cpu->write_memory(0x02, 0xe4);
        
        // External call to a simple command processor
        cpu->write_memory(0xe400, i8080_EMU_EXT_CALL);
        
        // Command processor messages, '$'-terminated
        // as is the convention in CP/M.
        const char * const CMD_MSGS[] = {
            "Invalid address.$", 
            "Invalid command.$",
            "RUN addr: Start executing instructions from addr. For eg. RUN 0x0100\nHELP: Bring up this list of commands.\nQUIT: quit$",
            "\n> $"
        };

        // loop indices
        size_t i, j;

		// Store messages from 0xe410
        emu_addr_t msgs_loc = 0xe410;
        const char * msg; size_t msg_len;
        for (i = 0; i < 4; ++i) {
            msg = CMD_MSGS[i];
            msg_len = strlen(msg);
            for (j = 0; j < msg_len; ++j) {
                cpu->write_memory(msgs_loc++, msg[j]);
            }
        }
        
        // HLT for the rest of the interrupts
        for (i = 1; i < 8; ++i) {
            cpu->write_memory(INTERRUPT_TABLE[i], i8080_HLT);
        }

        cpu->emu_ext_call = i8080_cpm_zero_page;
        EMU_ENV = (enum emu_env)CPM;
    } else {
        printf(INIT_MEM_STREAM_ERR_MSG);
    }
}

void emu_set_default_env(i8080 * const cpu) {
    
    if (cpu->write_memory != NULL) {
        // Create the interrupt vector table
        // Do not write to RST 1 sequence, we'll put our bootloader there instead
        size_t i;
        for (i = 1; i < 8; ++i) {
            // HLT for all interrupts
            cpu->write_memory(INTERRUPT_TABLE[i], i8080_HLT);
        }

        // Write a default bootloader that jumps to start of program memory
        cpu->write_memory(0x0000, i8080_JMP);
        cpu->write_memory(0x0001, DEFAULT_START_PA);
        cpu->write_memory(0x0002, 0x00);
        EMU_ENV = (enum emu_env)DEFAULT;
    } else {
        printf(INIT_MEM_STREAM_ERR_MSG);
    }
}

void emu_init_i8080(i8080 * const cpu) {
    i8080_init(cpu);
    cpu->read_memory = NULL;
    cpu->write_memory = NULL;
    cpu->port_in = NULL;
    cpu->port_out = NULL;
    cpu->emu_ext_call = NULL;
    cpu->interrupt_acknowledge = NULL;
}

// Writes test_word to all locations, then reads test_word from all locations.
// Returns 0 if a read failed to return test_word, and stores the failed location in cpu->pc.
static int memory_write_read_pass(i8080 * const cpu, const emu_addr_t start_addr, const emu_addr_t end_addr, const emu_word_t test_word, const int descriptive) {
    size_t i = 0;
    if (descriptive) {
        int success = 1;
        // Write pass
        printf("Write pass: " WORD_T_PRT_FORMAT "\n", test_word);
        for (i = start_addr; i <= end_addr; ++i) {
            // Show progress
            printf("\r(" ADDR_T_PRT_FORMAT "/0xffff)", (emu_addr_t)i);
            cpu->write_memory((emu_addr_t)i, test_word);
        }
        // Read pass
        printf("\nRead pass: " WORD_T_PRT_FORMAT "\n", test_word);
        for (i = start_addr; i <= end_addr; ++i) {
            // Show progress
            printf("\r(" ADDR_T_PRT_FORMAT "/0xffff)", (emu_addr_t)i);
            if (cpu->read_memory((emu_addr_t)i) != test_word) {
                // indicate to user which location failed
                printf("\nLocation " ADDR_T_PRT_FORMAT " failed.", (emu_addr_t)i);
                cpu->pc = (emu_addr_t)i;
                success = 0;
                break;
            }
        }
        printf("\n");
        return success;
    }
    else {
        for (i = start_addr; i <= end_addr; ++i) {
            cpu->write_memory((emu_addr_t)i, test_word);
        }
        for (i = start_addr; i <= end_addr; ++i) {
            if (cpu->read_memory((emu_addr_t)i) != test_word) {
                cpu->pc = (emu_addr_t)i;
                return 0;
            }
        }
        return 1;
    }
}

int memory_check_errors(i8080 * const cpu, const emu_addr_t start_addr, const emu_addr_t end_addr, const int descriptive) {
    if (descriptive) printf("Checking memory...\n");
    // Pass 1
    if (!memory_write_read_pass(cpu, start_addr, end_addr, 0x55, descriptive)) return 0;
    // Pass 2, check alternate bits
    if (!memory_write_read_pass(cpu, start_addr, end_addr, 0xAA, descriptive)) return 0;
    if (descriptive) printf("All memory locations functional.\n");
    return 1;
}

emu_exit_code_t emu_runtime(i8080 * const cpu, emu_debug_args_t * d_args) {
    
    if (cpu->read_memory == NULL || cpu->write_memory == NULL) {
        return EMU_ERR_MEM_STREAMS;
    }
    
    // If in debug mode, this is overridden by i8080_debug_next
    int (* i8080_next_ovrd)(i8080 * const) = i8080_next;
    
    if(d_args != NULL) {
        // In debug mode, override i8080_next()
        set_debug_next_options(d_args);
        i8080_next_ovrd = i8080_debug_next;
    }

    // Print welcome message
    printf("intel8080-emulator, with limited CP/M BIOS support.\nSee github.com/dhruvwarrier/intel8080-emulator/ for more.\n");
    if (EMU_ENV == (enum emu_env)CPM) printf("\nCP/M Warm Boot. Type HELP for a list of commands.");

    // Execute all instructions until failure/quit
    while(1) {
        if (!i8080_next_ovrd(cpu)) {
            break;
        }
    }
    
    if (cpu->last_instr_exec == i8080_IN || cpu->last_instr_exec == i8080_OUT) {
        return EMU_ERR_IO_STREAMS;
    }
    
    return EMU_EXIT_SUCCESS;
}