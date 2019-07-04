/*
 * Implement i8080 emulation.
 * 
 */

#include "i8080_internal.h"
#include "i8080_opcodes.h"

struct register_pair {
    word_t * left;
    word_t * right;
};

// Returns pointers to the left and right registers for a mov operation.
static struct register_pair i8080_mov_get_register_pair(i8080 * const cpu, word_t opcode) {
    
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
            if (lo_opcode >= 0x0 && lo_opcode <= 0x07) {
                regs.left = &cpu->b;
            } else if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                regs.left = &cpu->c;
            }
            break;
        case 0x50:
            if (lo_opcode >= 0x0 && lo_opcode <= 0x07) {
                regs.left = &cpu->d;
            } else if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                regs.left = &cpu->e;
            }
            break;
        case 0x60:
            if (lo_opcode >= 0x0 && lo_opcode <= 0x07) {
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
    struct register_pair regs = i8080_mov_get_register_pair(cpu, opcode);
    // perform move
    *regs.left = *regs.right;
}

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
            word_t mem_word = cpu->read_memory((addr_t)cpu->h << HALF_ADDR_SIZE | (addr_t)cpu->l);
            
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
            cpu->write_memory((addr_t)cpu->h << HALF_ADDR_SIZE | (addr_t)cpu->l, word_to_write);
            break;
        }
    }
}
