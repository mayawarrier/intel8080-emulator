/*
 * Implement i8080 emulation.
 * 
 */

#include "i8080_internal.h"
#include "i8080_opcodes.h"
#include <math.h>

struct register_pair {
    word_t * left;
    word_t * right;
};

static const buf_t BIT_PAST_WORD = 1 << (HALF_WORD_SIZE * 2);
static const buf_t BIT_PAST_HALF_WORD = 1 << (HALF_WORD_SIZE);
static const buf_t WORD_LO_F = pow(2, HALF_WORD_SIZE) - 1;

// Picks the lower half of the word
#define WORD_LO_BITS(expr) ((buf_t)expr & WORD_LO_F)

// Returns pointers to the left and right registers for a mov operation.
static struct register_pair mov_get_register_pair(i8080 * const cpu, word_t opcode);
// Updates the Z, S, P flags based on accumulator value
static inline void update_ZSP(i8080 * const cpu);

// Performs a mov operation from register to register.
static void i8080_mov_registers(i8080 * const cpu, word_t opcode);
// Performs an add to accumulator and updates flags.
static void i8080_add(i8080 * const cpu, word_t word);

static inline word_t i8080_read_memory(i8080 * const cpu);
static inline void i8080_write_memory(i8080 * const cpu, word_t word);

// Get address represented by {HL}
static inline addr_t i8080_get_hl(i8080 * const cpu);

static word_t i8080_advance_read_word(i8080 * const cpu) {
    // Read a word and advance program counter
    return cpu->read_memory(cpu->pc++);
}

void i8080_reset(i8080 * const cpu) {
    // start executing from beginning again
    cpu->pc = 0;
}

void i8080_next(i8080 * const cpu) {
    i8080_exec(cpu, i8080_advance_read_word(cpu));
}

void i8080_exec(i8080 * const cpu, word_t opcode) {
    
    cpu->cycles_taken += OPCODES_CYCLES[opcode];
    
    switch(opcode) {
        
        // All move operations between registers
        case MOV_B_B: case MOV_B_C: case MOV_B_D: case MOV_B_E: case MOV_B_H: case MOV_B_L: case MOV_B_A:
        case MOV_C_B: case MOV_C_C: case MOV_C_D: case MOV_C_E: case MOV_C_H: case MOV_C_L: case MOV_C_A:
        case MOV_D_B: case MOV_D_C: case MOV_D_D: case MOV_D_E: case MOV_D_H: case MOV_D_L: case MOV_D_A:
        case MOV_E_B: case MOV_E_C: case MOV_E_D: case MOV_E_E: case MOV_E_H: case MOV_E_L: case MOV_E_A:
        case MOV_H_B: case MOV_H_C: case MOV_H_D: case MOV_H_E: case MOV_H_H: case MOV_H_L: case MOV_H_A:
        case MOV_L_B: case MOV_L_C: case MOV_L_D: case MOV_L_E: case MOV_L_H: case MOV_L_L: case MOV_L_A:
        case MOV_A_B: case MOV_A_C: case MOV_A_D: case MOV_A_E: case MOV_A_H: case MOV_A_L: case MOV_A_A:
            i8080_mov_registers(cpu, opcode); break;    
            
        // All move operations from memory to registers
        case MOV_B_M: case MOV_C_M: case MOV_D_M: case MOV_E_M: case MOV_H_M: case MOV_L_M: case MOV_A_M:
        {
            // Read a word at address [HL]
            word_t mem_word = i8080_read_memory(cpu);
            
            switch(opcode) {
                case MOV_B_M: cpu->b = mem_word; break;
                case MOV_C_M: cpu->c = mem_word; break;
                case MOV_D_M: cpu->d = mem_word; break;
                case MOV_E_M: cpu->e = mem_word; break;
                case MOV_H_M: cpu->h = mem_word; break;
                case MOV_L_M: cpu->l = mem_word; break;
                case MOV_A_M: cpu->a = mem_word; break;
            }
            break;
        }
        
        // All move operations from registers to memory
        case MOV_M_B: case MOV_M_C: case MOV_M_D: case MOV_M_E: case MOV_M_H: case MOV_M_L: case MOV_M_A:
        {
            word_t word_to_write = 0x0;
            
            switch(opcode) {
                case MOV_M_B: word_to_write = cpu->b; break;
                case MOV_M_C: word_to_write = cpu->c; break;
                case MOV_M_D: word_to_write = cpu->d; break;
                case MOV_M_E: word_to_write = cpu->e; break;
                case MOV_M_H: word_to_write = cpu->h; break;
                case MOV_M_L: word_to_write = cpu->l; break;
                case MOV_M_A: word_to_write = cpu->a; break;
            }
            
            // Write a word at address [HL]
            i8080_write_memory(cpu, word_to_write);
            break;
        }
        
        // Regular addition
        case ADD_B: i8080_add(cpu, cpu->b); break;
        case ADD_C: i8080_add(cpu, cpu->c); break;
        case ADD_D: i8080_add(cpu, cpu->d); break;
        case ADD_E: i8080_add(cpu, cpu->e); break;
        case ADD_H: i8080_add(cpu, cpu->h); break;
        case ADD_L: i8080_add(cpu, cpu->l); break;
        case ADD_M: i8080_add(cpu, i8080_read_memory(cpu)); break;
        case ADD_A: i8080_add(cpu, cpu->a); break;
    }
}

static inline word_t i8080_read_memory(i8080 * const cpu) {
    return cpu->read_memory(i8080_get_hl(cpu));
}

static inline void i8080_write_memory(i8080 * const cpu, word_t word) {
    cpu->write_memory(i8080_get_hl(cpu), word);
}

static inline addr_t i8080_get_hl(i8080 * const cpu) {
    return (addr_t)cpu->h << HALF_ADDR_SIZE | (addr_t)cpu->l;
}

static struct register_pair mov_get_register_pair(i8080 * const cpu, word_t opcode) {
    
    struct register_pair regs;
    regs.left = NULL;
    regs.right = NULL;
    
    word_t lo_opcode = opcode & 0x0f;
    word_t hi_opcode = opcode & 0xf0;
    
    switch(lo_opcode) {
        case 0x00: case 0x08:
            regs.right = &cpu->b; break;
        case 0x01: case 0x09: 
            regs.right = &cpu->c; break;
        case 0x02: case 0x0a:
            regs.right = &cpu->d; break;
        case 0x03: case 0x0b:
            regs.right = &cpu->e; break;
        case 0x04: case 0x0c:
            regs.right = &cpu->h; break;
        case 0x05: case 0x0d:
            regs.right = &cpu->l; break;
        case 0x07: case 0x0f:
            regs.right = &cpu->a; break;
        // Moving from M = [HL] should be dealt with separately
        default: regs.right = NULL; break;
    }
    
    switch(hi_opcode) {
        case 0x40:
            if (lo_opcode >= 0x00 && lo_opcode <= 0x07) {
                regs.left = &cpu->b;
            } else if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                regs.left = &cpu->c;
            }
            break;
        case 0x50:
            if (lo_opcode >= 0x00 && lo_opcode <= 0x07) {
                regs.left = &cpu->d;
            } else if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                regs.left = &cpu->e;
            }
            break;
        case 0x60:
            if (lo_opcode >= 0x00 && lo_opcode <= 0x07) {
                regs.left = &cpu->h;
            } else if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                regs.left = &cpu->l;
            }
        case 0x70:
            // Moving into M = [HL] should be dealt with separately
            if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                regs.left = &cpu->a;
            }
            break;
        default: regs.left = NULL;
    }
    
    return regs;
}

static void i8080_mov_registers(i8080 * const cpu, word_t opcode) {
    // get the register pair from this opcode
    struct register_pair regs = mov_get_register_pair(cpu, opcode);
    // perform move
    *regs.left = *regs.right;
}

static inline void update_ZSP(i8080 * const cpu) {
    cpu->z = (cpu->a == 0);
    cpu->s = (cpu->a < 0);
    cpu->p = 0; buf_t buf = (buf_t)cpu->a; 
    /* Until all 8 bits have been shifted out,
     * shift each bit to the left and XNOR it with cpu->p.
     * while(buf & (word_t)WORD_T_MAX != 0): check if word bits are zero yet
     * (buff<<=1): shift the buffer to the left
     * & BIT_PAST_WORD: select the bit we just shifted
     * == 0: invert the bit before the XOR */
    while(buf & (word_t)WORD_T_MAX != 0) { 
        cpu->p ^= ((buf<<=1) & BIT_PAST_WORD == 0); 
    }
}

static void i8080_add(i8080 * const cpu, word_t word) {
    cpu->a += word;
    // Update flags
    update_ZSP(cpu);
    cpu->acy = (WORD_LO_BITS(cpu->a) + WORD_LO_BITS(word)) & BIT_PAST_HALF_WORD != 0;
    cpu->cy = ((buf_t)cpu->a + (buf_t)word) & BIT_PAST_WORD != 0;
}