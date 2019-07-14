/*
 * Implement i8080 emulation.
 * 
 */

#include "i8080_internal.h"
#include "i8080_opcodes.h"
#include <math.h>
#include <limits.h>
#include <stdio.h>

enum flag_bit {
    CARRY_BIT = 0,
    PARITY_BIT = 2,
    AUX_CARRY_BIT = 4,
    ZERO_BIT = 6,
    SIGN_BIT = 7
};

static const word_t WORD_LO_F = ((word_t)1 << HALF_WORD_SIZE) - (word_t)1;
static const word_t WORD_HI_F = (((word_t)1 << HALF_WORD_SIZE) - (word_t)1) << HALF_WORD_SIZE;
static const buf_t BUF_HI_WORD_MAX = (buf_t)WORD_T_MAX << (HALF_WORD_SIZE * 2);
// Conditional RETs and CALLs (subroutine ops) take 6 cycles longer if the condition is true.
static const size_t CYCLES_OFFSET_COND_SUB_OPS = (size_t)6;

// Picks the lower half of the word
#define WORD_LO_BITS(expr) ((word_t)(expr) & WORD_LO_F)
// Picks the upper half of the word
#define WORD_HI_BITS(expr) ((word_t)(expr) & WORD_HI_F)
// Picks the lower word bits only from a buf_t, this is safer than just (word_t)buf
#define WORD_BITS(expr) ((buf_t)(expr) & (buf_t)WORD_T_MAX)
// Picks the word bits from the hi part of buf_t.
#define WORD_DBL_BITS(expr) (WORD_BITS(((buf_t)(expr) & BUF_HI_WORD_MAX) >> (2 * HALF_WORD_SIZE)))
// Picks the address bits only from a buf_t, this is safer than just (addr_t)buf
#define ADDR_BITS(expr) ((buf_t)(expr) & (buf_t)ADDR_T_MAX)
// Perform 2's complement on the lower half of the word
#define TWOS_COMP_LO_WORD(expr) (((word_t)(expr) ^ WORD_LO_F) + (word_t)1)
// Perform 2's complement on the entire word, up-scaling to a buf_t
#define TWOS_COMP_WORD(expr) (((buf_t)(expr) ^ (buf_t)WORD_T_MAX) + (buf_t)1)
// Finds the auxiliary carry generated by adding two words.
#define AUX_CARRY(expr1, expr2) (get_word_bit((WORD_LO_BITS(expr1) + WORD_LO_BITS(expr2)), HALF_WORD_SIZE))

static inline bool get_word_bit(word_t word, int bit) {
    return ((word & ((word_t)1 << bit)) != 0);
}

static inline void set_word_bit(word_t * word, int bit, bool val) {
    if (val) {
        *word |= ((word_t)1 << bit);
    } else {
        *word &= ~((word_t)1 << bit);
    }
}

static inline bool get_addr_bit(addr_t addr, int bit) {
    return ((addr & ((addr_t)1 << bit)) != 0);
}

static inline void set_addr_bit(addr_t * addr, int bit, bool val) {
    if (val) {
        *addr |= ((addr_t)1 << bit);
    } else {
        *addr &= ~((addr_t)1 << bit);
    }
}

static inline bool get_buf_bit(buf_t buf, int bit) {
    return ((buf & ((buf_t)1 << bit)) != 0);
}

static inline void set_buf_bit(buf_t * buf, int bit, bool val) {
    if (val) {
        *buf |= ((buf_t)1 << bit);
    } else {
        *buf &= ~((buf_t)1 << bit);
    }
}

// Updates the Z, S, P flags based on the word provided.
// This must be buffered to preserve the bits produced after word's MSB.
// All subtracting instructions must invert sign.
static inline void update_ZSP(i8080 * const cpu, buf_t word, bool invert_sign) {
    // The truncated word only keeps the word bits
    buf_t trun_word_buf = (buf_t)WORD_BITS(word);
    cpu->z = (trun_word_buf == 0);
    /* If the number has gone above WORD_T_MAX, this indicates overflow or
    /* an underflow. The sign is inverted on underflow, so any instructions
     * that subtract from the acc should set invert_sign = true */
    bool s = (word > (buf_t)WORD_T_MAX);
    cpu->s = invert_sign ? !s : s;
    /* Until all 8 bits have been shifted out,
     * shift each bit to the left and XOR it with cpu->p.
     * This indicates odd number of ones if high.
     * while(buf & (word_t)WORD_T_MAX != 0): check if word bits are zero yet
     * (buff<<=1): shift the buffer to the left
     * get_word_bit(,): select the bit we just shifted out */
    cpu->p = false; // reset before starting calculation again
    while((trun_word_buf & (buf_t)WORD_T_MAX) != (buf_t)0x0) {
        trun_word_buf <<= 1;
        cpu->p ^= get_buf_bit(trun_word_buf, HALF_WORD_SIZE * 2);
    }
    // invert since we want even number of ones
    cpu->p = !cpu->p;
}

// Concatenates two words and returns a double word.
static inline buf_t concatenate(word_t word1, word_t word2) {
    return ((buf_t)word1 << HALF_ADDR_SIZE | (buf_t)word2);
}

// Gets the flags register from the internal emulator bool values.
static word_t i8080_get_flags_reg(i8080 * const cpu) {
    word_t flags = (word_t)0;
    set_word_bit(&flags, (int)CARRY_BIT, cpu->cy);
    set_word_bit(&flags, (int)PARITY_BIT, cpu->p);
    set_word_bit(&flags, (int)AUX_CARRY_BIT, cpu->acy);
    set_word_bit(&flags, (int)ZERO_BIT, cpu->z);
    set_word_bit(&flags, (int)SIGN_BIT, cpu->s);
    /* Bits 1, 3, 5 are always set to the following values:
     * http://pastraiser.com/cpu/i8080/i8080_opcodes.html */
    set_word_bit(&flags, 1, true);
    set_word_bit(&flags, 3, false);
    set_word_bit(&flags, 5, false);
    return flags;
}

// Sets the internal emulator bool values from a flags register.
static void i8080_set_flags_reg(i8080 * const cpu, word_t flags_reg) {
    cpu->cy = get_word_bit(flags_reg, (int)CARRY_BIT);
    cpu->p = get_word_bit(flags_reg, (int)PARITY_BIT);
    cpu->acy = get_word_bit(flags_reg, (int)AUX_CARRY_BIT);
    cpu->z = get_word_bit(flags_reg, (int)ZERO_BIT);
    cpu->s = get_word_bit(flags_reg, (int)SIGN_BIT);
} 

// Get address represented by {BC}
static inline addr_t i8080_get_bc(i8080 * const cpu) {
    return (addr_t)ADDR_BITS(concatenate(cpu->b, cpu->c));
}

// Get address represented by {DE}
static inline addr_t i8080_get_de(i8080 * const cpu) {
    return (addr_t)ADDR_BITS(concatenate(cpu->d, cpu->e));
}

// Get address represented by {HL}
static inline addr_t i8080_get_hl(i8080 * const cpu) {
    return (addr_t)ADDR_BITS(concatenate(cpu->h, cpu->l));
}

// Gets the program status word {A, flags}
static inline addr_t i8080_get_psw(i8080 * const cpu) {
    return (addr_t)ADDR_BITS(concatenate(cpu->a, i8080_get_flags_reg(cpu)));
}

// Sets B to hi dbl_word, and C to lo dbl_word
static inline void i8080_set_bc(i8080 * const cpu, buf_t dbl_word) {
    cpu->b = (word_t)WORD_DBL_BITS(dbl_word);
    cpu->c = (word_t)WORD_BITS(dbl_word);
}

// Sets D to hi dbl_word, and E to lo dbl_word
static inline void i8080_set_de(i8080 * const cpu, buf_t dbl_word) {
    cpu->d = (word_t)WORD_DBL_BITS(dbl_word);
    cpu->e = (word_t)WORD_BITS(dbl_word);
}

// Sets H to hi dbl_word, and L to lo dbl_word
static inline void i8080_set_hl(i8080 * const cpu, buf_t dbl_word) {
    cpu->h = (word_t)WORD_DBL_BITS(dbl_word);
    cpu->l = (word_t)WORD_BITS(dbl_word);
}

// Sets the accumulator and flags register from the program status word.
static inline void i8080_set_psw(i8080 * const cpu, buf_t dbl_word) {
    cpu->a = (word_t)WORD_DBL_BITS(dbl_word);
    i8080_set_flags_reg(cpu, (word_t)WORD_BITS(dbl_word));
}

// Reads a word from [HL]
static inline word_t i8080_read_memory(i8080 * const cpu) {
    return cpu->read_memory(i8080_get_hl(cpu));
}

// Writes a word to [HL]
static inline void i8080_write_memory(i8080 * const cpu, word_t word) {
    cpu->write_memory(i8080_get_hl(cpu), word);
}

// Reads a word and advances PC by 1.
static inline word_t i8080_advance_read_word(i8080 * const cpu) {
    return cpu->read_memory(cpu->pc++);
}

// Reads address (a double word) and advances PC by 2.
static inline addr_t i8080_advance_read_addr(i8080 * const cpu) {
    word_t hi_addr = i8080_advance_read_word(cpu);
    word_t lo_addr = i8080_advance_read_word(cpu);
    return (addr_t)concatenate(hi_addr, lo_addr);
}

// Returns pointers to the left and right registers for a mov operation.
static void i8080_mov_get_reg_pair(i8080 * const cpu, word_t opcode, word_t ** left, word_t ** right) {
    
    *left = NULL; *right = NULL;
    
    word_t lo_opcode = opcode & 0x0f;
    word_t hi_opcode = opcode & 0xf0;
    
    switch(lo_opcode) {
        case 0x00: case 0x08:
            *right = &cpu->b; break;
        case 0x01: case 0x09: 
            *right = &cpu->c; break;
        case 0x02: case 0x0a:
            *right = &cpu->d; break;
        case 0x03: case 0x0b:
            *right = &cpu->e; break;
        case 0x04: case 0x0c:
            *right = &cpu->h; break;
        case 0x05: case 0x0d:
            *right = &cpu->l; break;
        case 0x07: case 0x0f:
            *right = &cpu->a; break;
        // Moving from M = [HL] should be dealt with separately
        default: *right = NULL; break;
    }
    
    switch(hi_opcode) {
        case 0x40:
            if (lo_opcode >= 0x00 && lo_opcode <= 0x07) {
                *left = &cpu->b;
            } else if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                *left = &cpu->c;
            }
            break;
        case 0x50:
            if (lo_opcode >= 0x00 && lo_opcode <= 0x07) {
                *left = &cpu->d;
            } else if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                *left = &cpu->e;
            }
            break;
        case 0x60:
            if (lo_opcode >= 0x00 && lo_opcode <= 0x07) {
                *left = &cpu->h;
            } else if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                *left = &cpu->l;
            }
        case 0x70:
            // Moving into M = [HL] should be dealt with separately
            if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                *left = &cpu->a;
            }
            break;
        default: *left = NULL;
    }
}

// Performs a move operation from register to register.
static void i8080_mov_reg(i8080 * const cpu, word_t opcode) {
    // get the register pair from this opcode
    word_t * left_pptr[1], * right_pptr[1];
    i8080_mov_get_reg_pair(cpu, opcode, left_pptr, right_pptr);
    // perform move
    *(*left_pptr) = *(*right_pptr);
}

// Performs an add to accumulator and updates flags.
static void i8080_add(i8080 * const cpu, word_t word) {
    // this cannot be determined from acc_buf, do this before
    cpu->acy = AUX_CARRY(cpu->a, word);
    // We need a larger type so buffer overflow does not occur
    buf_t acc_buf = (buf_t)cpu->a + (buf_t)word;
    // The accumulator only needs to keep the word bits
    cpu->a = (word_t)WORD_BITS(acc_buf);
    // Update remaining flags
    cpu->cy = get_buf_bit(acc_buf, HALF_WORD_SIZE * 2);
    update_ZSP(cpu, acc_buf, false);
}

// Performs an add with carry to accumulator and updates flags.
static inline void i8080_adc(i8080 * const cpu, word_t word) {
    i8080_add(cpu, word + (word_t)cpu->cy);
}

// Performs a subtract from accumulator and updates flags
static void i8080_sub(i8080 * const cpu, word_t word) {
    // Perform 2's complement subtraction on the lo bits of the word
    cpu->acy = AUX_CARRY(cpu->a, TWOS_COMP_LO_WORD(word));
    // Perform SUB
    buf_t acc_buf = (buf_t)cpu->a + TWOS_COMP_WORD(word);
    cpu->a = (word_t)WORD_BITS(acc_buf);
    // Update remaining flags
    // Carry is the borrow flag for SUB, SBB etc, invert carry 
    cpu->cy = !get_buf_bit(acc_buf, HALF_WORD_SIZE * 2);
    // We want to invert the sign here as well to indicate borrow from 0
    update_ZSP(cpu, acc_buf, true);
}

// Perform a subtract with carry borrow from the accumulator, and updates flags. 
static inline void i8080_sbb(i8080 * const cpu, word_t word) {
    i8080_sub(cpu, word + (word_t)cpu->cy);
}

// Perform bitwise AND with accumulator, and update flags.
static void i8080_ana(i8080 * const cpu, word_t word) {
    /* In the 8080, AND logical instructions always set auxiliary carry to the logical OR of bit 3 of the values involved:
     * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 24 */
    cpu->acy = get_word_bit(cpu->a, HALF_WORD_SIZE - 1) | get_word_bit(word, HALF_WORD_SIZE - 1);
    // Perform ANA
    cpu->a &= word;
    // Update remaining flags
    update_ZSP(cpu, (buf_t)cpu->a, false);
    /* In the 8080, AND logical instructions always reset carry:
     * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 63 */
    cpu->cy = false;
}

// Performs bitwise exclusive OR with accumulator, and updates flags.
static void i8080_xra(i8080 * const cpu, word_t word) {
    // Perform XRA
    cpu->a ^= word;
    update_ZSP(cpu, (buf_t)cpu->a, false);
    /* In the 8080, OR logical instructions always reset the carry and auxiliary carry flags to 0:
     * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 122 */
    cpu->acy = false;
    cpu->cy = false;
}

// Performs bitwise inclusive OR with accumulator, and updates flags.
static void i8080_ora(i8080 * const cpu, word_t word) {
    // Perform ORA
    cpu->a |= word;
    update_ZSP(cpu, (buf_t)cpu->a, false);
    /* In the 8080, OR logical instructions always reset the carry and auxiliary carry flags to 0:
     * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 122 */
    cpu->acy = false;
    cpu->cy = false;
}

// Updates flags after subtraction from accumulator, without modifying it.
static void i8080_cmp(i8080 * const cpu, word_t word) {
    // This is almost identical to i8080_sub, with the exception that the accumulator is not affected
    cpu->acy = AUX_CARRY(cpu->a, TWOS_COMP_LO_WORD(word));
    buf_t acc_buf = (buf_t)cpu->a + TWOS_COMP_WORD(word);
    // Carry is inverted for borrow
    cpu->cy = !get_buf_bit(acc_buf, HALF_WORD_SIZE * 2);
    update_ZSP(cpu, acc_buf, true);
}

// Increments and updates flags, and returns the incremented word.
static word_t i8080_inr(i8080 * const cpu, word_t word) {
    cpu->acy = AUX_CARRY(word, 1);
    word++; // no need to buffer since overflow is allowed without sign change
    update_ZSP(cpu, (buf_t)word, false);
    return word;
}

// Decrements and updates flags, and returns the decremented word.
static word_t i8080_dcr(i8080 * const cpu, word_t word) {
    cpu->acy = AUX_CARRY(word, TWOS_COMP_LO_WORD(1));
    word--; // no need to buffer since underflow is allowed without sign change
    update_ZSP(cpu, (buf_t)word, true);
    return word;
}

// Adds the reg_pair to HL and updates flags.
static void i8080_dad(i8080 * const cpu, addr_t reg_pair) {
    // Adds reg_pair, buffering to compute carry
    addr_t hl = i8080_get_hl(cpu);
    buf_t hl_buf = (buf_t)hl + (buf_t)reg_pair;
    i8080_set_hl(cpu, (addr_t)ADDR_BITS(hl_buf));
    cpu->cy = !get_buf_bit(hl_buf, HALF_ADDR_SIZE * 2);
}

// Rotates accumulator left and sets carry to acc MSB.
static void i8080_rlc(i8080 * const cpu) {
    bool msb = get_word_bit(cpu->a, HALF_WORD_SIZE * 2 - 1);
    // shift left
    cpu->a <<= 1;
    // set lsb to msb
    set_word_bit(&cpu->a, 0, msb);
    cpu->cy = msb;
}

// Rotates accumulator right and sets carry to acc LSB.
static void i8080_rrc(i8080 * const cpu) {
    bool lsb = get_word_bit(cpu->a, 0);
    // shift right
    cpu->a >>= 1;
    // set msb to lsb
    set_word_bit(&cpu->a, 2 * HALF_WORD_SIZE - 1, lsb);
    cpu->cy = lsb;
}

// Rotates accumulator left through carry.
static void i8080_ral(i8080 * const cpu) {
    // Save previous cy to become new lsb
    bool prev_cy = cpu->cy;
    // save previous msb to become new carry
    bool msb = get_word_bit(cpu->a, 2 * HALF_WORD_SIZE - 1);
    cpu->a <<= 1;
    cpu->cy = msb;
    set_word_bit(&cpu->a, 0, prev_cy);
}

// Rotates accumulator right through carry.
static void i8080_rar(i8080 * const cpu) {
    bool prev_cy = cpu->cy;
    bool lsb = get_word_bit(cpu->a, 0);
    cpu->a >>= 1;
    // set carry to previous lsb
    cpu->cy = lsb;
    // set MSB to previous carry
    set_word_bit(&cpu->a, 2 * HALF_WORD_SIZE - 1, prev_cy);
}

// Performs a decimal adjust on the accumulator (converts to BCD) and updates flags.
static void i8080_daa(i8080 * const cpu) {
    word_t lo_acc = WORD_LO_BITS(cpu->a);
    // if lo bits are greater than 9 or 15, add 6 to accumulator
    if ((lo_acc > (word_t)9) || cpu->acy) {
        i8080_add(cpu, (word_t)6); // this is lo 6
    }
    // We want to preserve the original auxiliary carry
    // from here because DAA adds 6 only to the hi bits,
    // and the auxiliary carry is not affected.
    bool prev_acy = cpu->acy;
    // if hi bits are greater than 9 or 15, add 6 to hi bits
    word_t hi_acc = WORD_HI_BITS(cpu->a) >> HALF_WORD_SIZE;
    if ((hi_acc > (word_t)9) || cpu->cy) {
        i8080_add(cpu, (word_t)96); // this is hi 6
    }
    // bring back previous acy
    cpu->acy = prev_acy;
}

// Pushes two words to the stack, and decrements the stack pointer by 2.
// Pushes higher word before lower word.
static void i8080_push(i8080 * const cpu, buf_t dbl_word) {
    word_t left_word = (word_t)WORD_DBL_BITS(dbl_word);
    word_t right_word = (word_t)WORD_BITS(dbl_word);
    cpu->write_memory(cpu->sp - (addr_t)1, left_word);
    cpu->write_memory(cpu->sp - (addr_t)2, right_word);
    cpu->sp -= 2;
}

// Pops last two words from the stack and increments the stack pointer by 2.
// Pops lower word before higher word.
static buf_t i8080_pop(i8080 * const cpu) {
    word_t right_word = cpu->read_memory(cpu->sp);
    word_t left_word = cpu->read_memory(cpu->sp + (addr_t)1);
    cpu->sp += 2;
    return concatenate(left_word, right_word);
}

// Jumps to address addr.
static inline void i8080_jmp_addr(i8080 * const cpu, addr_t addr) {
    cpu->pc = addr;
} 

// Pushes the current PC and jumps to addr.
static inline void i8080_call_addr(i8080 * const cpu, addr_t addr) {
    i8080_push(cpu, (buf_t)cpu->pc);
    i8080_jmp_addr(cpu, addr);
}

// Jumps to immediate address.
static inline void i8080_jmp(i8080 * const cpu) {
    i8080_jmp_addr(cpu, i8080_advance_read_addr(cpu));
}

// Pushes the current PC and jumps to immediate address.
static inline void i8080_call(i8080 * const cpu) {
    i8080_call_addr(cpu, i8080_advance_read_addr(cpu));
}

// Pops the last PC and jumps to it.
static inline void i8080_ret(i8080 * const cpu) {
    cpu->pc = (addr_t)ADDR_BITS(i8080_pop(cpu));
}

// Exchanges the top 2 words of the stack with {HL}.
static inline void i8080_xthl(i8080 * const cpu) {
    // Gets the top two words on the stack
    word_t prev_right_word = cpu->read_memory(cpu->sp);
    word_t prev_left_word = cpu->read_memory(cpu->sp + (addr_t)1);
    // Overwrites the top 2 words with H and L
    cpu->write_memory(cpu->sp, cpu->l);
    cpu->write_memory(cpu->sp + (addr_t)1, cpu->h);
    // Sets HL to the previous top 2 words on the stack 
    cpu->h = prev_left_word;
    cpu->l = prev_right_word;
}

// Exchanges the contents of BC and DE.
static inline void i8080_xchg(i8080 * const cpu) {
    addr_t bc = i8080_get_bc(cpu);
    i8080_set_bc(cpu, i8080_get_de(cpu));
    i8080_set_de(cpu, bc);
}

void i8080_reset(i8080 * const cpu) {
    // start executing from beginning again
    cpu->pc = 0;
    cpu->is_halted = false;
}

bool i8080_next(i8080 * const cpu) {
    return i8080_exec(cpu, i8080_advance_read_word(cpu));
}

bool i8080_debug_next(i8080 * const cpu) {
    word_t opcode = i8080_advance_read_word(cpu);
    printf("\n%s", DEBUG_DISASSEMBLY_TABLE[opcode]);
    return i8080_exec(cpu, opcode);
}

bool i8080_exec(i8080 * const cpu, word_t opcode) {
    
    cpu->cycles_taken += OPCODES_CYCLES[opcode];
    
    switch(opcode) {
        
        // No operation + undocumented NOPs. Does nothing.
        case NOP: case ALT_NOP0: case ALT_NOP1: case ALT_NOP2: case ALT_NOP3: case ALT_NOP4: case ALT_NOP5: case ALT_NOP6:
            break;
        
        // Move between registers
        case MOV_B_B: case MOV_B_C: case MOV_B_D: case MOV_B_E: case MOV_B_H: case MOV_B_L: case MOV_B_A:
        case MOV_C_B: case MOV_C_C: case MOV_C_D: case MOV_C_E: case MOV_C_H: case MOV_C_L: case MOV_C_A:
        case MOV_D_B: case MOV_D_C: case MOV_D_D: case MOV_D_E: case MOV_D_H: case MOV_D_L: case MOV_D_A:
        case MOV_E_B: case MOV_E_C: case MOV_E_D: case MOV_E_E: case MOV_E_H: case MOV_E_L: case MOV_E_A:
        case MOV_H_B: case MOV_H_C: case MOV_H_D: case MOV_H_E: case MOV_H_H: case MOV_H_L: case MOV_H_A:
        case MOV_L_B: case MOV_L_C: case MOV_L_D: case MOV_L_E: case MOV_L_H: case MOV_L_L: case MOV_L_A:
        case MOV_A_B: case MOV_A_C: case MOV_A_D: case MOV_A_E: case MOV_A_H: case MOV_A_L: case MOV_A_A:
            i8080_mov_reg(cpu, opcode); break;    
            
        // Move from memory to registers      
        case MOV_B_M: cpu->b = i8080_read_memory(cpu); break;
        case MOV_C_M: cpu->c = i8080_read_memory(cpu); break;
        case MOV_D_M: cpu->d = i8080_read_memory(cpu); break;
        case MOV_E_M: cpu->e = i8080_read_memory(cpu); break;
        case MOV_H_M: cpu->h = i8080_read_memory(cpu); break;
        case MOV_L_M: cpu->l = i8080_read_memory(cpu); break;
        case MOV_A_M: cpu->a = i8080_read_memory(cpu); break;
        
        // Move from registers to memory       
        case MOV_M_B: i8080_write_memory(cpu, cpu->b); break;
        case MOV_M_C: i8080_write_memory(cpu, cpu->c); break;
        case MOV_M_D: i8080_write_memory(cpu, cpu->d); break;
        case MOV_M_E: i8080_write_memory(cpu, cpu->e); break;
        case MOV_M_H: i8080_write_memory(cpu, cpu->h); break;
        case MOV_M_L: i8080_write_memory(cpu, cpu->l); break;
        case MOV_M_A: i8080_write_memory(cpu, cpu->a); break;
        
        // Move immediate
        case MVI_B: cpu->b = i8080_advance_read_word(cpu); break;
        case MVI_C: cpu->c = i8080_advance_read_word(cpu); break;
        case MVI_D: cpu->d = i8080_advance_read_word(cpu); break;
        case MVI_E: cpu->e = i8080_advance_read_word(cpu); break;
        case MVI_H: cpu->h = i8080_advance_read_word(cpu); break;
        case MVI_L: cpu->l = i8080_advance_read_word(cpu); break;
        case MVI_M: i8080_write_memory(cpu, i8080_advance_read_word(cpu)); break;
        case MVI_A: cpu->a = i8080_advance_read_word(cpu); break;
        
        // Regular addition
        case ADD_B: i8080_add(cpu, cpu->b); break;
        case ADD_C: i8080_add(cpu, cpu->c); break;
        case ADD_D: i8080_add(cpu, cpu->d); break;
        case ADD_E: i8080_add(cpu, cpu->e); break;
        case ADD_H: i8080_add(cpu, cpu->h); break;
        case ADD_L: i8080_add(cpu, cpu->l); break;
        case ADD_M: i8080_add(cpu, i8080_read_memory(cpu)); break;
        case ADD_A: i8080_add(cpu, cpu->a); break;
        
        // Addition with carry
        case ADC_B: i8080_adc(cpu, cpu->b); break;
        case ADC_C: i8080_adc(cpu, cpu->c); break;
        case ADC_D: i8080_adc(cpu, cpu->d); break;
        case ADC_E: i8080_adc(cpu, cpu->e); break;
        case ADC_H: i8080_adc(cpu, cpu->h); break;
        case ADC_L: i8080_adc(cpu, cpu->l); break;
        case ADC_M: i8080_adc(cpu, i8080_read_memory(cpu)); break;
        case ADC_A: i8080_adc(cpu, cpu->b); break;
        
        // Regular subtraction
        case SUB_B: i8080_sub(cpu, cpu->b); break;
        case SUB_C: i8080_sub(cpu, cpu->c); break;
        case SUB_D: i8080_sub(cpu, cpu->d); break;
        case SUB_E: i8080_sub(cpu, cpu->e); break;
        case SUB_H: i8080_sub(cpu, cpu->h); break;
        case SUB_L: i8080_sub(cpu, cpu->l); break;
        case SUB_M: i8080_sub(cpu, i8080_read_memory(cpu)); break;
        case SUB_A: i8080_sub(cpu, cpu->a); break;
        
        // Subtraction with borrowed carry
        case SBB_B: i8080_sbb(cpu, cpu->b); break;
        case SBB_C: i8080_sbb(cpu, cpu->c); break;
        case SBB_D: i8080_sbb(cpu, cpu->d); break;
        case SBB_E: i8080_sbb(cpu, cpu->e); break;
        case SBB_H: i8080_sbb(cpu, cpu->h); break;
        case SBB_L: i8080_sbb(cpu, cpu->l); break;
        case SBB_M: i8080_sbb(cpu, i8080_read_memory(cpu)); break;
        case SBB_A: i8080_sbb(cpu, cpu->a); break;
        
        // Logical AND with accumulator
        case ANA_B: i8080_ana(cpu, cpu->b); break;
        case ANA_C: i8080_ana(cpu, cpu->c); break;
        case ANA_D: i8080_ana(cpu, cpu->d); break;
        case ANA_E: i8080_ana(cpu, cpu->e); break;
        case ANA_H: i8080_ana(cpu, cpu->h); break;
        case ANA_L: i8080_ana(cpu, cpu->l); break;
        case ANA_M: i8080_ana(cpu, i8080_read_memory(cpu)); break;
        case ANA_A: i8080_ana(cpu, cpu->a); break;
        
        // Exclusive logical OR with accumulator
        case XRA_B: i8080_xra(cpu, cpu->b); break;
        case XRA_C: i8080_xra(cpu, cpu->c); break;
        case XRA_D: i8080_xra(cpu, cpu->d); break;
        case XRA_E: i8080_xra(cpu, cpu->e); break;
        case XRA_H: i8080_xra(cpu, cpu->h); break;
        case XRA_L: i8080_xra(cpu, cpu->l); break;
        case XRA_M: i8080_xra(cpu, i8080_read_memory(cpu)); break;
        case XRA_A: i8080_xra(cpu, cpu->a); break;
        
        // Inclusive logical OR with accumulator
        case ORA_B: i8080_ora(cpu, cpu->b); break;
        case ORA_C: i8080_ora(cpu, cpu->c); break;
        case ORA_D: i8080_ora(cpu, cpu->d); break;
        case ORA_E: i8080_ora(cpu, cpu->e); break;
        case ORA_H: i8080_ora(cpu, cpu->h); break;
        case ORA_L: i8080_ora(cpu, cpu->l); break;
        case ORA_M: i8080_ora(cpu, i8080_read_memory(cpu)); break;
        case ORA_A: i8080_ora(cpu, cpu->a); break;
        
        // Compare with accumulator
        case CMP_B: i8080_cmp(cpu, cpu->b); break;
        case CMP_C: i8080_cmp(cpu, cpu->c); break;
        case CMP_D: i8080_cmp(cpu, cpu->d); break;
        case CMP_E: i8080_cmp(cpu, cpu->e); break;
        case CMP_H: i8080_cmp(cpu, cpu->h); break;
        case CMP_L: i8080_cmp(cpu, cpu->l); break;
        case CMP_M: i8080_cmp(cpu, i8080_read_memory(cpu)); break;
        case CMP_A: i8080_cmp(cpu, cpu->a); break;
        
        // Increment registers/memory
        case INR_B: cpu->b = i8080_inr(cpu, cpu->b); break;
        case INR_C: cpu->c = i8080_inr(cpu, cpu->c); break;
        case INR_D: cpu->d = i8080_inr(cpu, cpu->d); break;
        case INR_E: cpu->e = i8080_inr(cpu, cpu->e); break;
        case INR_H: cpu->h = i8080_inr(cpu, cpu->h); break;
        case INR_L: cpu->l = i8080_inr(cpu, cpu->l); break;
        case INR_M: i8080_write_memory(cpu, i8080_inr(cpu, i8080_read_memory(cpu))); break;
        case INR_A: i8080_inr(cpu, cpu->a); break;
        
        // Decrement registers/memory
        case DCR_B: cpu->b = i8080_dcr(cpu, cpu->b); break;
        case DCR_C: cpu->c = i8080_dcr(cpu, cpu->c); break;
        case DCR_D: cpu->d = i8080_dcr(cpu, cpu->d); break;
        case DCR_E: cpu->e = i8080_dcr(cpu, cpu->e); break;
        case DCR_H: cpu->h = i8080_dcr(cpu, cpu->h); break;
        case DCR_L: cpu->l = i8080_dcr(cpu, cpu->l); break;
        case DCR_M: i8080_write_memory(cpu, i8080_dcr(cpu, i8080_read_memory(cpu))); break;
        case DCR_A: i8080_dcr(cpu, cpu->a); break;
        
        // Increment/decrement register pairs
        case INX_B: i8080_set_bc(cpu, i8080_get_bc(cpu) + (addr_t)1); break;
        case INX_D: i8080_set_de(cpu, i8080_get_de(cpu) + (addr_t)1); break;
        case INX_H: i8080_set_hl(cpu, i8080_get_hl(cpu) + (addr_t)1); break;
        case INX_SP: cpu->sp += 1; break;
        case DCX_B: i8080_set_bc(cpu, i8080_get_bc(cpu) - (addr_t)1); break;
        case DCX_D: i8080_set_de(cpu, i8080_get_de(cpu) - (addr_t)1); break;
        case DCX_H: i8080_set_hl(cpu, i8080_get_hl(cpu) - (addr_t)1); break;
        case DCX_SP: cpu->sp -= 1; break;
        
        // Double register add (16-bit addition)
        case DAD_B: i8080_dad(cpu, i8080_get_bc(cpu)); break;
        case DAD_D: i8080_dad(cpu, i8080_get_de(cpu)); break;
        case DAD_H: i8080_dad(cpu, i8080_get_hl(cpu)); break;
        case DAD_SP: i8080_dad(cpu, cpu->sp); break;
        
        // Load register pair immediate
        case LXI_B: cpu->c = i8080_advance_read_word(cpu); cpu->b = i8080_advance_read_word(cpu); break;
        case LXI_D: cpu->e = i8080_advance_read_word(cpu); cpu->d = i8080_advance_read_word(cpu); break;
        case LXI_H: cpu->l = i8080_advance_read_word(cpu); cpu->h = i8080_advance_read_word(cpu); break;
        case LXI_SP: cpu->sp = i8080_advance_read_addr(cpu); break;
        
        // Load/store accumulator <-> register pair
        case STAX_B: cpu->write_memory(i8080_get_bc(cpu), cpu->a); break;
        case STAX_D: cpu->write_memory(i8080_get_de(cpu), cpu->a); break;
        case LDAX_B: cpu->a = cpu->read_memory(i8080_get_bc(cpu)); break;
        case LDAX_D: cpu->a = cpu->read_memory(i8080_get_de(cpu)); break;
        
        // Load/store accumulator immediate
        case STA: cpu->write_memory(i8080_advance_read_addr(cpu), cpu->a); break;
        case LDA: cpu->a = cpu->read_memory(i8080_advance_read_addr(cpu)); break;
        
        // Store HL to memory address
        case SHLD:
        {
            addr_t addr = i8080_advance_read_addr(cpu);
            cpu->write_memory(addr, cpu->l);
            cpu->write_memory(addr + (addr_t) 1, cpu->h);
            break;
        }
        
        // Read HL from memory address
        case LHLD:
        {
            addr_t addr = i8080_advance_read_addr(cpu);
            cpu->l = cpu->read_memory(addr);
            cpu->h = cpu->read_memory(addr + (addr_t) 1);
            break;
        }
        
        // Rotate
        case RLC: i8080_rlc(cpu); break;  // Rotate accumulator left
        case RRC: i8080_rrc(cpu); break;  // Rotate accumulator right
        case RAL: i8080_ral(cpu); break;  // Rotate accumulator left through carry
        case RAR: i8080_rar(cpu); break;  // Rotate accumulator right through carry
        
        // Arithmetic / logical / compare immediate
        case ADI: i8080_add(cpu, i8080_advance_read_word(cpu)); break;
        case ACI: i8080_adc(cpu, i8080_advance_read_word(cpu)); break;
        case SUI: i8080_sub(cpu, i8080_advance_read_word(cpu)); break;
        case SBI: i8080_sbb(cpu, i8080_advance_read_word(cpu)); break;
        case ANI: i8080_ana(cpu, i8080_advance_read_word(cpu)); break;
        case XRI: i8080_xra(cpu, i8080_advance_read_word(cpu)); break;
        case ORI: i8080_ora(cpu, i8080_advance_read_word(cpu)); break;
        case CPI: i8080_cmp(cpu, i8080_advance_read_word(cpu)); break;
        
        // Stack push / pop
        case PUSH_B: i8080_push(cpu, i8080_get_bc(cpu)); break;
        case PUSH_D: i8080_push(cpu, i8080_get_de(cpu)); break;
        case PUSH_H: i8080_push(cpu, i8080_get_hl(cpu)); break;
        case PUSH_PSW: i8080_push(cpu, i8080_get_psw(cpu)); break;
        case POP_B: i8080_set_bc(cpu, (addr_t)ADDR_BITS(i8080_pop(cpu))); break;
        case POP_D: i8080_set_de(cpu, (addr_t)ADDR_BITS(i8080_pop(cpu))); break;
        case POP_H: i8080_set_hl(cpu, (addr_t)ADDR_BITS(i8080_pop(cpu))); break;
        case POP_PSW: i8080_set_psw(cpu, (addr_t)ADDR_BITS(i8080_pop(cpu))); break;
        
        // Subroutine calls
        // Conditional CALLs take 6 cycles longer if the condition is met
        case CALL: case ALT_CALL0: case ALT_CALL1: case ALT_CALL2: i8080_call(cpu); break;
        case CNZ: if (!cpu->z) {i8080_call(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;  // CALL on !Z i.e. non-zero acc
        case CZ: if (cpu->z) {i8080_call(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;    // CALL on Z i.e. zero acc
        case CNC: if (!cpu->cy) {i8080_call(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break; // CALL on !CY i.e. carry reset
        case CC: if (cpu->cy) {i8080_call(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;   // CALL on CY i.e. carry set
        case CPO: if (!cpu->p) {i8080_call(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;  // CALL on !P i.e. acc parity odd
        case CPE: if (cpu->p) {i8080_call(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;   // CALL on P i.e. acc parity even
        case CP: if (!cpu->s) {i8080_call(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;   // CALL on !S i.e. acc positive
        case CM: if (cpu->s) {i8080_call(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;    // CALL on S i.e. acc negative
        
        // Subroutine returns
        // Conditional RETs take 6 cycles longer if the condition is met
        case RET: case ALT_RET0: i8080_ret(cpu); break;
        case RNZ: if (!cpu->z) {i8080_ret(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;   // RET on !Z i.e. non-zero acc
        case RZ: if (cpu->z) {i8080_ret(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;     // RET on Z i.e. zero acc
        case RNC: if (!cpu->cy) {i8080_ret(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;  // RET on !CY i.e. carry reset
        case RC: if (cpu->cy) {i8080_ret(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;    // RET on CY i.e. carry set
        case RPO: if (!cpu->p) {i8080_ret(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;   // RET on !P i.e. acc parity odd
        case RPE: if (cpu->p) {i8080_ret(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;    // RET on P i.e. acc parity even
        case RP: if (!cpu->s) {i8080_ret(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;    // RET on !S i.e. acc positive
        case RM: if (cpu->s) {i8080_ret(cpu); cpu->cycles_taken += CYCLES_OFFSET_COND_SUB_OPS; } break;     // RET on S i.e. acc negative
        
        // Jumps
        case JMP: case ALT_JMP0: i8080_jmp(cpu); break;     
        case JNZ: if (!cpu->z) {i8080_jmp(cpu); } break;    // JMP on !Z i.e. non-zero acc
        case JZ: if (cpu->z) {i8080_jmp(cpu); } break;      // JMP on Z i.e. zero acc
        case JNC: if (!cpu->cy) {i8080_jmp(cpu); } break;   // JMP on !CY i.e. carry reset
        case JC: if (cpu->cy) {i8080_jmp(cpu); } break;     // JMP on CY i.e. carry set
        case JPO: if (!cpu->p) {i8080_jmp(cpu); } break;    // JMP on !P i.e. acc parity odd
        case JPE: if (cpu->p) {i8080_jmp(cpu); } break;     // JMP on P i.e. acc parity even
        case JP: if (!cpu->s) {i8080_jmp(cpu); } break;     // JMP on !S i.e. acc positive
        case JM: if (cpu->s) {i8080_jmp(cpu); } break;      // JMP on S i.e. acc negative
        
        // Special instructions
        case DAA: i8080_daa(cpu); break;      // Decimal adjust acc (convert acc to BCD)
        case CMA: cpu->a = ~cpu->a; break;    // Complement accumulator
        case STC: cpu->cy = true; break;      // Set carry
        case CMC: cpu->cy = !cpu->cy; break;  // Complement carry
        case PCHL: cpu->pc = i8080_get_hl(cpu); break; // Move HL into PC
        case SPHL: cpu->sp = i8080_get_hl(cpu); break; // Move HL into SP
        case XTHL: i8080_xthl(cpu); break;             // Exchange top of stack with H&L
        case XCHG: i8080_xchg(cpu); break;             // Exchanges the contents of BC and DE
        
        // I/O, accumulator <-> word
        /* In the 8080, the port address is duplicated across the 16-bit address bus:
         * https://stackoverflow.com/questions/13551973/intel-8080-instruction-out
         * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 97 */
        case IN: 
        {
            word_t port_addr = i8080_advance_read_word(cpu);
            cpu->a = cpu->port_in((addr_t)concatenate(port_addr, port_addr));
        }
        case OUT: 
        {
            word_t port_addr = i8080_advance_read_word(cpu);
            cpu->port_out((addr_t)concatenate(port_addr, port_addr), cpu->a);
        }
        
        // Restart / software interrupts
        case RST_0: i8080_call_addr(cpu, INTERRUPT_TABLE[0]); break;
        case RST_1: i8080_call_addr(cpu, INTERRUPT_TABLE[1]); break;
        case RST_2: i8080_call_addr(cpu, INTERRUPT_TABLE[2]); break;
        case RST_3: i8080_call_addr(cpu, INTERRUPT_TABLE[3]); break;
        case RST_4: i8080_call_addr(cpu, INTERRUPT_TABLE[4]); break;
        case RST_5: i8080_call_addr(cpu, INTERRUPT_TABLE[5]); break;
        case RST_6: i8080_call_addr(cpu, INTERRUPT_TABLE[6]); break;
        case RST_7: i8080_call_addr(cpu, INTERRUPT_TABLE[7]); break;
        
        // Enable / disable interrupts
        case EI: cpu->ie = true; break;
        case DI: cpu->ie = false; break;
        
        // HALT processor
        // Can only be brought out by interrupt or RESET.
        case HLT: cpu->is_halted = true; break;
        
        default: return false;
    }
    
    // successful execution
    return true;
}