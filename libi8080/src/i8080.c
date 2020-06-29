
#include "i8080.h"
#include "i8080_opcodes.h"

#ifdef __STDC__
    #define inline
    #define volatile
#endif

/* 
 * Sourced from https://github.com/superzazu/8080/blob/master/i8080.c, with corrections.
 * Indexed by opcode. For conditional RETs and CALLs, add 6 if true.
 */
static const unsigned int OPCODES_CYCLES[] = {
/*  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
    4,  10, 7,  5,  5,  5,  7,  4,  4,  10, 7,  5,  5,  5,  7,  4,  /* 0 */
    4,  10, 7,  5,  5,  5,  7,  4,  4,  10, 7,  5,  5,  5,  7,  4,  /* 1 */
    4,  10, 16, 5,  5,  5,  7,  4,  4,  10, 16, 5,  5,  5,  7,  4,  /* 2 */
    4,  10, 13, 5,  10, 10, 10, 4,  4,  10, 13, 5,  5,  5,  7,  4,  /* 3 */
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  /* 4 */
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  /* 5 */
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  /* 6 */
    7,  7,  7,  7,  7,  7,  7,  7,  5,  5,  5,  5,  5,  5,  7,  5,  /* 7 */
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  /* 8 */
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  /* 9 */
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  /* A */
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  /* B */
    5,  10, 10, 10, 11, 11, 7,  11, 5,  10, 10, 10, 11, 17, 7,  11, /* C */
    5,  10, 10, 10, 11, 11, 7,  11, 5,  10, 10, 10, 11, 17, 7,  11, /* D */
    5,  10, 10, 18, 11, 11, 7,  11, 5,  5,  10, 5,  11, 17, 7,  11, /* E */
    5,  10, 10, 4,  11, 11, 7,  11, 5,  5,  10, 4,  11, 17, 7,  11  /* F */
};
#define SUBROUTINE_CYCLES_OFFSET (6)

enum i8080_flags 
{
    CARRY_BIT = 0,
    PARITY_BIT = 2,
    AUX_CARRY_BIT = 4,
    ZERO_BIT = 6,
    SIGN_BIT = 7
};

#define I8080_WORD_SIZE (8)
#define I8080_WORD_MAX (255)
#define I8080_ADDR_MAX (65535)

static const i8080_word_t WORD_LO_F = ((i8080_word_t)1 << (I8080_WORD_SIZE / 2)) - (i8080_word_t)1;
static const i8080_word_t WORD_HI_F = (((i8080_word_t)1 << (I8080_WORD_SIZE / 2)) - (i8080_word_t)1) << (I8080_WORD_SIZE / 2);
static const i8080_dbl_word_t DBL_WORD_UPPER_MAX = (i8080_dbl_word_t)I8080_WORD_MAX << I8080_WORD_SIZE;

#define i8080_min(a,b) (((a) < (b)) ? (a) : (b))

/* Concatenate two words and return a double word. */
#define concatenate(word1, word2) ((i8080_dbl_word_t)(word1) << I8080_WORD_SIZE | (word2))

#define word_lo_nibble(word) ((i8080_word_t)(word) & WORD_LO_F)
#define word_hi_nibble(word) ((i8080_word_t)(word) & WORD_HI_F)

#define trim_to_addr(dbl_word) ((i8080_dbl_word_t)(dbl_word) & I8080_ADDR_MAX)
#define trim_to_word(word) ((i8080_word_t)(word) & I8080_WORD_MAX)

#define dbl_word_upper(dbl_word) ((i8080_word_t)(((i8080_dbl_word_t)(dbl_word) & DBL_WORD_UPPER_MAX) >> I8080_WORD_SIZE))
#define dbl_word_lower(dbl_word) ((i8080_word_t)((i8080_dbl_word_t)(dbl_word) & I8080_WORD_MAX))

/* Get a bit from a bit buffer. */
#define get_bit(buf, bit) ((int)(((buf) >> (bit)) & (unsigned)1))

/* Set a bit in a word. */
static inline void set_word_bit(i8080_word_t *word_ptr, int bit, int val) {
	if (val) {
		*word_ptr |= ((i8080_word_t)1 << bit);
	}
	else {
		*word_ptr &= ~((i8080_word_t)1 << bit);
	}
}

/* Perform 2's complement on lower nibble of a word. */
static inline i8080_word_t twos_comp_lo_word(i8080_word_t word) {
	return (word ^ WORD_LO_F) + (i8080_word_t)1;
}

/* 
 * Perform 2's complement on a word.
 * i8080_dbl_word_t preserves carry on CPI 0:
 * https://retrocomputing.stackexchange.com/questions/6407/intel-8080-behaviour-of-the-carry-bit-when-comparing-a-value-with-0 
 */
static inline i8080_dbl_word_t twos_comp_word(i8080_word_t word) {
    return (word ^ (i8080_word_t)I8080_WORD_MAX) + (i8080_word_t)1;
}

/* Get auxiliary carry resulting from adding two words. */
static inline int aux_carry(i8080_word_t word1, i8080_word_t word2) {
    return get_bit(word_lo_nibble(word1) + word_lo_nibble(word2), I8080_WORD_SIZE / 2);
}

/* Calculate parity of a word. */
static inline int parity(i8080_word_t word) {
    int p = 0;
    int i;
    for (i = 0; i < I8080_WORD_SIZE; ++i) {
        p ^= get_bit(word, i);
    }
    return !p;
}

/* Update Z, S, P flags from a word. */
static inline void update_ZSP(struct i8080 *const cpu, i8080_word_t word) {
    cpu->z = (word == 0);
    /* signed numbers are -127 to 127. bit 7 of the accumulator is used for signing */
    cpu->s = get_bit(word, I8080_WORD_SIZE - 1);
    cpu->p = parity(word);
}

/* Get flags register from emulator bool values. */
static i8080_word_t i8080_get_flags_reg(struct i8080 *const cpu) {
    i8080_word_t flags = 0;
    set_word_bit(&flags, (int)CARRY_BIT, cpu->cy);
    set_word_bit(&flags, (int)PARITY_BIT, cpu->p);
    set_word_bit(&flags, (int)AUX_CARRY_BIT, cpu->acy);
    set_word_bit(&flags, (int)ZERO_BIT, cpu->z);
    set_word_bit(&flags, (int)SIGN_BIT, cpu->s);
    /* Bit 1 is always 1:
     * http://pastraiser.com/cpu/i8080/i8080_opcodes.html */
    set_word_bit(&flags, 1, 1);
    return flags;
}

/* Set emulator bool values from a flags register. */
static void i8080_set_flags_reg(struct i8080 *const cpu, i8080_word_t flags_reg) {
    cpu->cy = get_bit(flags_reg, (int)CARRY_BIT);
    cpu->p = get_bit(flags_reg, (int)PARITY_BIT);
    cpu->acy = get_bit(flags_reg, (int)AUX_CARRY_BIT);
    cpu->z = get_bit(flags_reg, (int)ZERO_BIT);
    cpu->s = get_bit(flags_reg, (int)SIGN_BIT);
}

/* Get {BC} */
static inline i8080_dbl_word_t i8080_get_bc(struct i8080 *const cpu) {
    return concatenate(cpu->b, cpu->c);
}
/* Get {DE} */
static inline i8080_dbl_word_t i8080_get_de(struct i8080 *const cpu) {
    return concatenate(cpu->d, cpu->e);
}
/* Get {HL} */
static inline i8080_dbl_word_t i8080_get_hl(struct i8080 *const cpu) {
    return concatenate(cpu->h, cpu->l);
}
/* Get program status word {A, flags} */
static inline i8080_dbl_word_t i8080_get_psw(struct i8080 *const cpu) {
    return concatenate(cpu->a, i8080_get_flags_reg(cpu));
}

/* Set {BC} */
static inline void i8080_set_bc(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
    cpu->b = dbl_word_upper(dbl_word);
    cpu->c = dbl_word_lower(dbl_word);
}
/* Set {DE} */
static inline void i8080_set_de(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
    cpu->d = dbl_word_upper(dbl_word);
    cpu->e = dbl_word_lower(dbl_word);
}
/* Set {HL} */
static inline void i8080_set_hl(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
    cpu->h = dbl_word_upper(dbl_word);
    cpu->l = dbl_word_lower(dbl_word);
}
/* Set accumulator and flags from program status word. */
static inline void i8080_set_psw(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
    cpu->a = dbl_word_upper(dbl_word);
    i8080_set_flags_reg(cpu, dbl_word_lower(dbl_word));
}

/* Read word at [HL] */
static inline i8080_word_t i8080_memory_read_indirect(struct i8080 *const cpu) {
	return trim_to_word(cpu->memory_read((i8080_addr_t)i8080_get_hl(cpu)));
}

/* Write word to [HL] */
static inline void i8080_memory_write_indirect(struct i8080 *const cpu, i8080_word_t word) {
    cpu->memory_write((i8080_addr_t)i8080_get_hl(cpu), word);
}

/* Read a word and advance PC by 1. */
static inline i8080_word_t i8080_advance_read_word(struct i8080 *const cpu) {
    return trim_to_word(cpu->memory_read(cpu->pc++));
}

/* Read address and advance PC by 2.
 * Address is read backwards as assembler inverts the words upon assembly:
 * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 84 */
static inline i8080_addr_t i8080_advance_read_addr(struct i8080 *const cpu) {
    i8080_word_t lo_addr = i8080_advance_read_word(cpu);
    i8080_word_t hi_addr = i8080_advance_read_word(cpu);
    return (i8080_addr_t)concatenate(hi_addr, lo_addr);
}

/* Return pointers to the left and right registers for a mov operation. */
static inline void i8080_mov_get_reg_pair(struct i8080 *const cpu, i8080_word_t opcode, i8080_word_t *left[1], i8080_word_t *right[1])
{
    i8080_word_t lo_opcode = opcode & 0x0f;
    i8080_word_t hi_opcode = opcode & 0xf0;

    switch (lo_opcode) {
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
        default: break;
    }

    switch (hi_opcode) {
        case 0x40:
            if (lo_opcode <= 0x07) {
                *left = &cpu->b;
            } else if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                *left = &cpu->c;
            }
            break;
        case 0x50:
            if (lo_opcode <= 0x07) {
                *left = &cpu->d;
            } else if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                *left = &cpu->e;
            }
            break;
        case 0x60:
            if (lo_opcode <= 0x07) {
                *left = &cpu->h;
            } else if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                *left = &cpu->l;
            }
            break;
        case 0x70:
            if (lo_opcode >= 0x08 && lo_opcode <= 0x0f) {
                *left = &cpu->a;
            }
            break;
		default: break;
    }
}

/* Perform an add to accumulator and update flags. */
static void i8080_add(struct i8080 *const cpu, i8080_word_t word) {
    /* Do the addition in a larger type to preserve carry */
    i8080_dbl_word_t acc_buf = (i8080_dbl_word_t)cpu->a + word;
    cpu->acy = aux_carry(cpu->a, word);
    cpu->cy = get_bit(acc_buf, I8080_WORD_SIZE);
    cpu->a = dbl_word_lower(acc_buf);
    /* Update remaining flags */
    update_ZSP(cpu, cpu->a);
}

/* Perform an add with carry to accumulator and update flags. */
static inline void i8080_adc(struct i8080 *const cpu, i8080_word_t word) {
    i8080_add(cpu, word + (i8080_word_t)cpu->cy);
}

/* Perform a subtraction from accumulator and update flags */
static void i8080_sub(struct i8080 *const cpu, i8080_word_t word) {
    i8080_dbl_word_t acc_buf = (i8080_dbl_word_t)cpu->a + twos_comp_word(word);
    cpu->acy = aux_carry(cpu->a, twos_comp_lo_word(word));
    /* carry is the borrow flag for SUB, SBB etc, invert. */
    cpu->cy = !get_bit(acc_buf, I8080_WORD_SIZE);
    cpu->a = dbl_word_lower(acc_buf);
    update_ZSP(cpu, cpu->a);
}

/* Perform a subtraction with borrow from the accumulator, and update flags. */
static inline void i8080_sbb(struct i8080 *const cpu, i8080_word_t word) {
    i8080_sub(cpu, word + (i8080_word_t)cpu->cy);
}

/* Perform bitwise AND with accumulator and update flags. */
static void i8080_ana(struct i8080 *const cpu, i8080_word_t word) {
    /* AND logical instructions always set auxiliary carry to logical OR of bit 3 of the values involved:
     * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 24 */
    cpu->acy = get_bit(cpu->a, (I8080_WORD_SIZE / 2) - 1) | get_bit(word, (I8080_WORD_SIZE / 2) - 1);
    cpu->a &= word;
    update_ZSP(cpu, cpu->a);
    /* AND logical instructions always reset carry:
     * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 63 */
    cpu->cy = 0;
}

/* Perform bitwise exclusive OR with accumulator and update flags. */
static void i8080_xra(struct i8080 *const cpu, i8080_word_t word) {
    cpu->a ^= word;
    update_ZSP(cpu, cpu->a);
    /* OR logical instructions always reset carry and auxiliary carry:
     * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 122 */
    cpu->acy = 0;
    cpu->cy = 0;
}

/* Perform bitwise inclusive OR with accumulator and update flags. */
static void i8080_ora(struct i8080 *const cpu, i8080_word_t word) {
    cpu->a |= word;
    update_ZSP(cpu, cpu->a);
    cpu->acy = 0;
    cpu->cy = 0;
}

/* Perform i8080_sub() but without modifying the accumulator. */
static void i8080_cmp(struct i8080 *const cpu, i8080_word_t word) {
    i8080_dbl_word_t acc_buf = (i8080_dbl_word_t)cpu->a + twos_comp_word(word);
    cpu->acy = aux_carry(cpu->a, twos_comp_lo_word(word));
    cpu->cy = !get_bit(acc_buf, I8080_WORD_SIZE);
    update_ZSP(cpu, dbl_word_lower(acc_buf));
}

/* Increment word and update flags. */
static i8080_word_t i8080_inr(struct i8080 *const cpu, i8080_word_t word) {
    cpu->acy = aux_carry(word, 1);
    word++;
    update_ZSP(cpu, word);
    return word;
}

/* Decrement word and update flags. */
static i8080_word_t i8080_dcr(struct i8080 *const cpu, i8080_word_t word) {
    cpu->acy = aux_carry(word, twos_comp_lo_word(1));
    word--;
    update_ZSP(cpu, word);
    return word;
}

/* Add double register to HL and update flags. */
static void i8080_dad(struct i8080 *const cpu, i8080_dbl_word_t reg_pair) {
    i8080_dbl_word_t hl = i8080_get_hl(cpu);
    i8080_dbl_word_t hl_new = trim_to_addr(hl + reg_pair);
    i8080_set_hl(cpu, hl_new);
    /* If addition overflowed, result must be lower than the addands */
    cpu->cy = (hl_new < i8080_min(hl, reg_pair)) ? 1 : 0;
}

/* Store HL to immediate address. */
static inline void i8080_shld(struct i8080 *const cpu) {
	i8080_addr_t addr = i8080_advance_read_addr(cpu);
	cpu->memory_write(addr++, cpu->l);
	cpu->memory_write(addr, cpu->h);
}

/* Load HL from immediate address. */
static inline void i8080_lhld(struct i8080 *const cpu) {
	i8080_addr_t addr = i8080_advance_read_addr(cpu);
	cpu->l = trim_to_word(cpu->memory_read(addr++));
	cpu->h = trim_to_word(cpu->memory_read(addr));
}

/* Circular shift accumulator left, set carry to old MSB. */
static void i8080_rlc(struct i8080 *const cpu) {
    int msb = get_bit(cpu->a, I8080_WORD_SIZE - 1);
    cpu->a <<= 1;
    set_word_bit(&cpu->a, 0, msb);
    cpu->cy = msb;
}

/* Circular shift accumulator right, set carry to old LSB. */
static void i8080_rrc(struct i8080 *const cpu) {
    int lsb = get_bit(cpu->a, 0);
    cpu->a >>= 1;
    set_word_bit(&cpu->a, I8080_WORD_SIZE - 1, lsb);
    cpu->cy = lsb;
}

/* Circular shift accumulator left through carry. */
static void i8080_ral(struct i8080 *const cpu) {
    int prev_cy = cpu->cy;
    int msb = get_bit(cpu->a, I8080_WORD_SIZE - 1);
    cpu->a <<= 1;
    cpu->cy = msb;
    set_word_bit(&cpu->a, 0, prev_cy);
}

/* Circular shift accumulator right through carry. */
static void i8080_rar(struct i8080 *const cpu) {
    int prev_cy = cpu->cy;
    int lsb = get_bit(cpu->a, 0);
    cpu->a >>= 1;
    cpu->cy = lsb;
    set_word_bit(&cpu->a, I8080_WORD_SIZE - 1, prev_cy);
}

/* Perform decimal adjust on the accumulator and update flags. */
static void i8080_daa(struct i8080 *const cpu) {
    i8080_word_t lo_acc = word_lo_nibble(cpu->a);
    /* overflow lo nibble if required - add 0x6 */
    if ((lo_acc > (i8080_word_t)9) || cpu->acy) {
        cpu->acy = aux_carry(cpu->a, 0x6);
        cpu->a += (i8080_word_t)0x6;
    }
    /* overflow hi nibble if required - add 0x60 */
    i8080_word_t hi_acc = word_hi_nibble(cpu->a) >> (I8080_WORD_SIZE / 2);
    if ((hi_acc > (i8080_word_t)9) || cpu->cy) {      
        i8080_dbl_word_t acc_buf = (i8080_dbl_word_t)cpu->a + (i8080_word_t)0x60;
        cpu->cy = get_bit(acc_buf, I8080_WORD_SIZE);
        cpu->a = dbl_word_lower(acc_buf);
    }
    update_ZSP(cpu, cpu->a);
}

/* Push two words to stack and decrement stack pointer by 2.
 * (upper word before lower word) */
static void i8080_push(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
    i8080_word_t upper_word = dbl_word_upper(dbl_word);
    i8080_word_t lower_word = dbl_word_lower(dbl_word);
    cpu->memory_write(--cpu->sp, upper_word);
    cpu->memory_write(--cpu->sp, lower_word);
}

/* Pop two words from stack and increment stack pointer by 2.
 * (lower word before upper word) */
static i8080_dbl_word_t i8080_pop(struct i8080 *const cpu) {
    i8080_word_t lower_word = trim_to_word(cpu->memory_read(cpu->sp++));
    i8080_word_t upper_word = trim_to_word(cpu->memory_read(cpu->sp++));
    return concatenate(upper_word, lower_word);
}

#define i8080_jmp_addr(cpu, addr) ((cpu)->pc = (addr))

/* Push PC and jump to addr. */
static inline void i8080_call_addr(struct i8080 *const cpu, i8080_addr_t addr) {
    i8080_push(cpu, (i8080_dbl_word_t)cpu->pc);
    i8080_jmp_addr(cpu, addr);
}

/* Jump to immediate address if condition is satisfied. */
static inline void i8080_jmp(struct i8080 *const cpu, int condition) {
    if (condition) {
        i8080_jmp_addr(cpu, i8080_advance_read_addr(cpu));
    } else {
        cpu->pc += 2; /* advance to word after address */
    }
}

/* Push PC and jump to immediate address if condition is satisfied.
 * (add 6 cycles if satisfied) */
static inline void i8080_call(struct i8080 *const cpu, int condition) {
    if (condition) {
        i8080_call_addr(cpu, i8080_advance_read_addr(cpu));
        cpu->cycles += SUBROUTINE_CYCLES_OFFSET;
    } else {
        cpu->pc += 2;
    }
}

/* Pop PC and jump to it if condition is satisfied.
 * (add 6 cycles if satisfied) */
static inline void i8080_ret(struct i8080 *const cpu, int condition) {
    if (condition) {
        i8080_jmp_addr(cpu, (i8080_addr_t)i8080_pop(cpu));
        cpu->cycles += SUBROUTINE_CYCLES_OFFSET;
    }
}

/* Exchange HL with top two words on the stack. */
static inline void i8080_xthl(struct i8080 *const cpu) {
    i8080_word_t prev_lower_word = trim_to_word(cpu->memory_read(cpu->sp++));
    i8080_word_t prev_upper_word = trim_to_word(cpu->memory_read(cpu->sp));
    cpu->memory_write(cpu->sp--, cpu->h);
    cpu->memory_write(cpu->sp, cpu->l);
    cpu->h = prev_upper_word;
    cpu->l = prev_lower_word;
}

/* Exchange BC and HL. */
static inline void i8080_xchg(struct i8080 *const cpu) {
    i8080_dbl_word_t hl = i8080_get_hl(cpu);
    i8080_set_hl(cpu, i8080_get_de(cpu));
    i8080_set_de(cpu, hl);
}

/* I/O port address is duplicated across the 16-bit address bus:
 * https://stackoverflow.com/questions/13551973/intel-8080-instruction-out
 * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 97 */

/* I/O: Read input port into accumulator. */
static inline void i8080_in(struct i8080 *const cpu) {
	i8080_word_t port_addr = i8080_advance_read_word(cpu);
	cpu->a = trim_to_word(cpu->io_read((i8080_addr_t)concatenate(port_addr, port_addr)));
}

/* I/O: Write accumulator to output port. */
static inline void i8080_out(struct i8080 *const cpu) {
	i8080_word_t port_addr = i8080_advance_read_word(cpu);
	cpu->io_write((i8080_addr_t)concatenate(port_addr, port_addr), cpu->a);
}


void i8080_interrupt(struct i8080 *const cpu) {
	if (cpu->inte && !cpu->intr) {
		cpu->intr = 1;
	}
}

void i8080_reset(struct i8080 *const cpu) {
	cpu->pc = 0;
	cpu->halt = 0;
	cpu->inte = 0;
	cpu->intr = 0;
	cpu->cycles = 0;
}

int i8080_next(struct i8080 *const cpu) {
	int intr_read = 0;
	i8080_word_t opcode;

    /* Service interrupt if pending request exists */
    if (cpu->inte && cpu->intr) {
        opcode = trim_to_word(cpu->interrupt_read());
        /* disable interrupts and bring out of halt */
        cpu->inte = 0;
        cpu->intr = 0;
        cpu->halt = 0;
		intr_read = 1;
    }

    /* Execute opcode */
    int err;
    if (!cpu->halt) {
		if (!intr_read) {
			opcode = i8080_advance_read_word(cpu);
		}
        err = i8080_exec(cpu, opcode);
    } else {
        /* success but remain halted */
        err = 0;
    }
    return err;
}

int i8080_exec(struct i8080 *const cpu, i8080_word_t opcode)
{
    int err = 0;

    switch (opcode) 
	{
        /* Documented & undocumented NOPs. Does nothing. */
        case i8080_NOP: case i8080_ALT_NOP0: case i8080_ALT_NOP1: case i8080_ALT_NOP2:
        case i8080_ALT_NOP3: case i8080_ALT_NOP4: case i8080_ALT_NOP5: case i8080_ALT_NOP6:
            break;

        /* Move between registers */
        case i8080_MOV_B_B: case i8080_MOV_B_C: case i8080_MOV_B_D: case i8080_MOV_B_E: case i8080_MOV_B_H: case i8080_MOV_B_L: case i8080_MOV_B_A:
        case i8080_MOV_C_B: case i8080_MOV_C_C: case i8080_MOV_C_D: case i8080_MOV_C_E: case i8080_MOV_C_H: case i8080_MOV_C_L: case i8080_MOV_C_A:
        case i8080_MOV_D_B: case i8080_MOV_D_C: case i8080_MOV_D_D: case i8080_MOV_D_E: case i8080_MOV_D_H: case i8080_MOV_D_L: case i8080_MOV_D_A:
        case i8080_MOV_E_B: case i8080_MOV_E_C: case i8080_MOV_E_D: case i8080_MOV_E_E: case i8080_MOV_E_H: case i8080_MOV_E_L: case i8080_MOV_E_A:
        case i8080_MOV_H_B: case i8080_MOV_H_C: case i8080_MOV_H_D: case i8080_MOV_H_E: case i8080_MOV_H_H: case i8080_MOV_H_L: case i8080_MOV_H_A:
        case i8080_MOV_L_B: case i8080_MOV_L_C: case i8080_MOV_L_D: case i8080_MOV_L_E: case i8080_MOV_L_H: case i8080_MOV_L_L: case i8080_MOV_L_A:
        case i8080_MOV_A_B: case i8080_MOV_A_C: case i8080_MOV_A_D: case i8080_MOV_A_E: case i8080_MOV_A_H: case i8080_MOV_A_L: case i8080_MOV_A_A:
		{
			/* get register pair and perform move */
			i8080_word_t *left_pptr[1], *right_pptr[1];
			i8080_mov_get_reg_pair(cpu, opcode, left_pptr, right_pptr);
			*(*left_pptr) = *(*right_pptr);
			break;
		}

        /* Move from memory to registers */
        case i8080_MOV_B_M: cpu->b = i8080_memory_read_indirect(cpu); break;
        case i8080_MOV_C_M: cpu->c = i8080_memory_read_indirect(cpu); break;
        case i8080_MOV_D_M: cpu->d = i8080_memory_read_indirect(cpu); break;
        case i8080_MOV_E_M: cpu->e = i8080_memory_read_indirect(cpu); break;
        case i8080_MOV_H_M: cpu->h = i8080_memory_read_indirect(cpu); break;
        case i8080_MOV_L_M: cpu->l = i8080_memory_read_indirect(cpu); break;
        case i8080_MOV_A_M: cpu->a = i8080_memory_read_indirect(cpu); break;

        /* Move from registers to memory */
        case i8080_MOV_M_B: i8080_memory_write_indirect(cpu, cpu->b); break;
        case i8080_MOV_M_C: i8080_memory_write_indirect(cpu, cpu->c); break;
        case i8080_MOV_M_D: i8080_memory_write_indirect(cpu, cpu->d); break;
        case i8080_MOV_M_E: i8080_memory_write_indirect(cpu, cpu->e); break;
        case i8080_MOV_M_H: i8080_memory_write_indirect(cpu, cpu->h); break;
        case i8080_MOV_M_L: i8080_memory_write_indirect(cpu, cpu->l); break;
        case i8080_MOV_M_A: i8080_memory_write_indirect(cpu, cpu->a); break;

        /* Move immediate */
        case i8080_MVI_B: cpu->b = i8080_advance_read_word(cpu); break;
        case i8080_MVI_C: cpu->c = i8080_advance_read_word(cpu); break;
        case i8080_MVI_D: cpu->d = i8080_advance_read_word(cpu); break;
        case i8080_MVI_E: cpu->e = i8080_advance_read_word(cpu); break;
        case i8080_MVI_H: cpu->h = i8080_advance_read_word(cpu); break;
        case i8080_MVI_L: cpu->l = i8080_advance_read_word(cpu); break;
        case i8080_MVI_M: i8080_memory_write_indirect(cpu, i8080_advance_read_word(cpu)); break;
        case i8080_MVI_A: cpu->a = i8080_advance_read_word(cpu); break;

        /* Addition */
        case i8080_ADD_B: i8080_add(cpu, cpu->b); break;
        case i8080_ADD_C: i8080_add(cpu, cpu->c); break;
        case i8080_ADD_D: i8080_add(cpu, cpu->d); break;
        case i8080_ADD_E: i8080_add(cpu, cpu->e); break;
        case i8080_ADD_H: i8080_add(cpu, cpu->h); break;
        case i8080_ADD_L: i8080_add(cpu, cpu->l); break;
        case i8080_ADD_M: i8080_add(cpu, i8080_memory_read_indirect(cpu)); break;
        case i8080_ADD_A: i8080_add(cpu, cpu->a); break;

        /* Addition with carry */
        case i8080_ADC_B: i8080_adc(cpu, cpu->b); break;
        case i8080_ADC_C: i8080_adc(cpu, cpu->c); break;
        case i8080_ADC_D: i8080_adc(cpu, cpu->d); break;
        case i8080_ADC_E: i8080_adc(cpu, cpu->e); break;
        case i8080_ADC_H: i8080_adc(cpu, cpu->h); break;
        case i8080_ADC_L: i8080_adc(cpu, cpu->l); break;
        case i8080_ADC_M: i8080_adc(cpu, i8080_memory_read_indirect(cpu)); break;
        case i8080_ADC_A: i8080_adc(cpu, cpu->a); break;

        /* Subtraction */
        case i8080_SUB_B: i8080_sub(cpu, cpu->b); break;
        case i8080_SUB_C: i8080_sub(cpu, cpu->c); break;
        case i8080_SUB_D: i8080_sub(cpu, cpu->d); break;
        case i8080_SUB_E: i8080_sub(cpu, cpu->e); break;
        case i8080_SUB_H: i8080_sub(cpu, cpu->h); break;
        case i8080_SUB_L: i8080_sub(cpu, cpu->l); break;
        case i8080_SUB_M: i8080_sub(cpu, i8080_memory_read_indirect(cpu)); break;
        case i8080_SUB_A: i8080_sub(cpu, cpu->a); break;

        /* Subtraction with borrowed carry */
        case i8080_SBB_B: i8080_sbb(cpu, cpu->b); break;
        case i8080_SBB_C: i8080_sbb(cpu, cpu->c); break;
        case i8080_SBB_D: i8080_sbb(cpu, cpu->d); break;
        case i8080_SBB_E: i8080_sbb(cpu, cpu->e); break;
        case i8080_SBB_H: i8080_sbb(cpu, cpu->h); break;
        case i8080_SBB_L: i8080_sbb(cpu, cpu->l); break;
        case i8080_SBB_M: i8080_sbb(cpu, i8080_memory_read_indirect(cpu)); break;
        case i8080_SBB_A: i8080_sbb(cpu, cpu->a); break;

        /* Logical AND with accumulator */
        case i8080_ANA_B: i8080_ana(cpu, cpu->b); break;
        case i8080_ANA_C: i8080_ana(cpu, cpu->c); break;
        case i8080_ANA_D: i8080_ana(cpu, cpu->d); break;
        case i8080_ANA_E: i8080_ana(cpu, cpu->e); break;
        case i8080_ANA_H: i8080_ana(cpu, cpu->h); break;
        case i8080_ANA_L: i8080_ana(cpu, cpu->l); break;
        case i8080_ANA_M: i8080_ana(cpu, i8080_memory_read_indirect(cpu)); break;
        case i8080_ANA_A: i8080_ana(cpu, cpu->a); break;

        /* Exclusive logical OR with accumulator */
        case i8080_XRA_B: i8080_xra(cpu, cpu->b); break;
        case i8080_XRA_C: i8080_xra(cpu, cpu->c); break;
        case i8080_XRA_D: i8080_xra(cpu, cpu->d); break;
        case i8080_XRA_E: i8080_xra(cpu, cpu->e); break;
        case i8080_XRA_H: i8080_xra(cpu, cpu->h); break;
        case i8080_XRA_L: i8080_xra(cpu, cpu->l); break;
        case i8080_XRA_M: i8080_xra(cpu, i8080_memory_read_indirect(cpu)); break;
        case i8080_XRA_A: i8080_xra(cpu, cpu->a); break;

        /* Inclusive logical OR with accumulator */
        case i8080_ORA_B: i8080_ora(cpu, cpu->b); break;
        case i8080_ORA_C: i8080_ora(cpu, cpu->c); break;
        case i8080_ORA_D: i8080_ora(cpu, cpu->d); break;
        case i8080_ORA_E: i8080_ora(cpu, cpu->e); break;
        case i8080_ORA_H: i8080_ora(cpu, cpu->h); break;
        case i8080_ORA_L: i8080_ora(cpu, cpu->l); break;
        case i8080_ORA_M: i8080_ora(cpu, i8080_memory_read_indirect(cpu)); break;
        case i8080_ORA_A: i8080_ora(cpu, cpu->a); break;

        /* Compare with accumulator */
        case i8080_CMP_B: i8080_cmp(cpu, cpu->b); break;
        case i8080_CMP_C: i8080_cmp(cpu, cpu->c); break;
        case i8080_CMP_D: i8080_cmp(cpu, cpu->d); break;
        case i8080_CMP_E: i8080_cmp(cpu, cpu->e); break;
        case i8080_CMP_H: i8080_cmp(cpu, cpu->h); break;
        case i8080_CMP_L: i8080_cmp(cpu, cpu->l); break;
        case i8080_CMP_M: i8080_cmp(cpu, i8080_memory_read_indirect(cpu)); break;
        case i8080_CMP_A: i8080_cmp(cpu, cpu->a); break;

        /* Register/memory increment */
        case i8080_INR_B: cpu->b = i8080_inr(cpu, cpu->b); break;
        case i8080_INR_C: cpu->c = i8080_inr(cpu, cpu->c); break;
        case i8080_INR_D: cpu->d = i8080_inr(cpu, cpu->d); break;
        case i8080_INR_E: cpu->e = i8080_inr(cpu, cpu->e); break;
        case i8080_INR_H: cpu->h = i8080_inr(cpu, cpu->h); break;
        case i8080_INR_L: cpu->l = i8080_inr(cpu, cpu->l); break;
        case i8080_INR_M: i8080_memory_write_indirect(cpu, i8080_inr(cpu, i8080_memory_read_indirect(cpu))); break;
        case i8080_INR_A: cpu->a = i8080_inr(cpu, cpu->a); break;

        /* Register/memory decrement */
        case i8080_DCR_B: cpu->b = i8080_dcr(cpu, cpu->b); break;
        case i8080_DCR_C: cpu->c = i8080_dcr(cpu, cpu->c); break;
        case i8080_DCR_D: cpu->d = i8080_dcr(cpu, cpu->d); break;
        case i8080_DCR_E: cpu->e = i8080_dcr(cpu, cpu->e); break;
        case i8080_DCR_H: cpu->h = i8080_dcr(cpu, cpu->h); break;
        case i8080_DCR_L: cpu->l = i8080_dcr(cpu, cpu->l); break;
        case i8080_DCR_M: i8080_memory_write_indirect(cpu, i8080_dcr(cpu, i8080_memory_read_indirect(cpu))); break;
        case i8080_DCR_A: cpu->a = i8080_dcr(cpu, cpu->a); break;

        /* Double register increment/decrement */
        case i8080_INX_B: i8080_set_bc(cpu, i8080_get_bc(cpu) + (i8080_dbl_word_t)1); break;
        case i8080_INX_D: i8080_set_de(cpu, i8080_get_de(cpu) + (i8080_dbl_word_t)1); break;
        case i8080_INX_H: i8080_set_hl(cpu, i8080_get_hl(cpu) + (i8080_dbl_word_t)1); break;
        case i8080_INX_SP: cpu->sp += 1; break;
        case i8080_DCX_B: i8080_set_bc(cpu, i8080_get_bc(cpu) - (i8080_dbl_word_t)1); break;
        case i8080_DCX_D: i8080_set_de(cpu, i8080_get_de(cpu) - (i8080_dbl_word_t)1); break;
        case i8080_DCX_H: i8080_set_hl(cpu, i8080_get_hl(cpu) - (i8080_dbl_word_t)1); break;
        case i8080_DCX_SP: cpu->sp -= 1; break;

        /* Double register add (16-bit addition) */
        case i8080_DAD_B: i8080_dad(cpu, i8080_get_bc(cpu)); break;
        case i8080_DAD_D: i8080_dad(cpu, i8080_get_de(cpu)); break;
        case i8080_DAD_H: i8080_dad(cpu, i8080_get_hl(cpu)); break;
        case i8080_DAD_SP: i8080_dad(cpu, (i8080_dbl_word_t)cpu->sp); break;

        /* Double register load immediate */
        case i8080_LXI_B: cpu->c = i8080_advance_read_word(cpu); cpu->b = i8080_advance_read_word(cpu); break;
        case i8080_LXI_D: cpu->e = i8080_advance_read_word(cpu); cpu->d = i8080_advance_read_word(cpu); break;
        case i8080_LXI_H: cpu->l = i8080_advance_read_word(cpu); cpu->h = i8080_advance_read_word(cpu); break;
        case i8080_LXI_SP: cpu->sp = i8080_advance_read_addr(cpu); break;

		/* Load/store accumulator, immediate indirect */
		case i8080_STA: cpu->memory_write(i8080_advance_read_addr(cpu), cpu->a); break;
		case i8080_LDA: cpu->a = trim_to_word(cpu->memory_read(i8080_advance_read_addr(cpu))); break;

        /* Load/store accumulator, double register indirect */
        case i8080_LDAX_B: cpu->a = trim_to_word(cpu->memory_read((i8080_addr_t)i8080_get_bc(cpu))); break;
        case i8080_LDAX_D: cpu->a = trim_to_word(cpu->memory_read((i8080_addr_t)i8080_get_de(cpu))); break;
		case i8080_STAX_B: cpu->memory_write((i8080_addr_t)i8080_get_bc(cpu), cpu->a); break;
		case i8080_STAX_D: cpu->memory_write((i8080_addr_t)i8080_get_de(cpu), cpu->a); break;

		/* Load/store double register, immediate indirect */
		case i8080_SHLD: i8080_shld(cpu); break;
		case i8080_LHLD: i8080_lhld(cpu); break;

        /* Rotate */
        case i8080_RLC: i8080_rlc(cpu); break;
        case i8080_RRC: i8080_rrc(cpu); break;
        case i8080_RAL: i8080_ral(cpu); break;
        case i8080_RAR: i8080_rar(cpu); break;

        /* Arithmetic / logical / compare immediate */
        case i8080_ADI: i8080_add(cpu, i8080_advance_read_word(cpu)); break;
        case i8080_ACI: i8080_adc(cpu, i8080_advance_read_word(cpu)); break;
        case i8080_SUI: i8080_sub(cpu, i8080_advance_read_word(cpu)); break;
        case i8080_SBI: i8080_sbb(cpu, i8080_advance_read_word(cpu)); break;
        case i8080_ANI: i8080_ana(cpu, i8080_advance_read_word(cpu)); break;
        case i8080_XRI: i8080_xra(cpu, i8080_advance_read_word(cpu)); break;
        case i8080_ORI: i8080_ora(cpu, i8080_advance_read_word(cpu)); break;
        case i8080_CPI: i8080_cmp(cpu, i8080_advance_read_word(cpu)); break;

        /* Stack push / pop */
        case i8080_PUSH_B: i8080_push(cpu, i8080_get_bc(cpu)); break;
        case i8080_PUSH_D: i8080_push(cpu, i8080_get_de(cpu)); break;
        case i8080_PUSH_H: i8080_push(cpu, i8080_get_hl(cpu)); break;
        case i8080_PUSH_PSW: i8080_push(cpu, i8080_get_psw(cpu)); break;
        case i8080_POP_B: i8080_set_bc(cpu, i8080_pop(cpu)); break;
        case i8080_POP_D: i8080_set_de(cpu, i8080_pop(cpu)); break;
        case i8080_POP_H: i8080_set_hl(cpu, i8080_pop(cpu)); break;
        case i8080_POP_PSW: i8080_set_psw(cpu, i8080_pop(cpu)); break;

        /* Subroutine calls */
        case i8080_CALL: case i8080_ALT_CALL0: case i8080_ALT_CALL1: case i8080_ALT_CALL2:
            i8080_call(cpu, 1); break;
        case i8080_CNZ: i8080_call(cpu, !cpu->z); break;  /* CALL on !Z i.e. non-zero acc */
        case i8080_CZ: i8080_call(cpu, cpu->z); break;    /* CALL on Z i.e. zero acc */
        case i8080_CNC: i8080_call(cpu, !cpu->cy); break; /* CALL on !CY i.e. carry reset */
        case i8080_CC: i8080_call(cpu, cpu->cy); break;   /* CALL on CY i.e. carry set */
        case i8080_CPO: i8080_call(cpu, !cpu->p); break;  /* CALL on !P i.e. acc parity odd */
        case i8080_CPE: i8080_call(cpu, cpu->p); break;   /* CALL on P i.e. acc parity even */
        case i8080_CP:  i8080_call(cpu, !cpu->s); break;  /* CALL on !S i.e. acc positive */
        case i8080_CM: i8080_call(cpu, cpu->s); break;    /* CALL on S i.e. acc negative */

        /* Subroutine returns */
        case i8080_RET: case i8080_ALT_RET0:
            i8080_ret(cpu, 1); break;
        case i8080_RNZ: i8080_ret(cpu, !cpu->z); break;   /* RET on !Z i.e. non-zero acc */
        case i8080_RZ: i8080_ret(cpu, cpu->z); break;     /* RET on Z i.e. zero acc */
        case i8080_RNC: i8080_ret(cpu, !cpu->cy); break;  /* RET on !CY i.e. carry reset */
        case i8080_RC: i8080_ret(cpu, cpu->cy); break;    /* RET on CY i.e. carry set */
        case i8080_RPO: i8080_ret(cpu, !cpu->p); break;   /* RET on !P i.e. acc parity odd */
        case i8080_RPE: i8080_ret(cpu, cpu->p); break;    /* RET on P i.e. acc parity even */
        case i8080_RP: i8080_ret(cpu, !cpu->s); break;    /* RET on !S i.e. acc positive */
        case i8080_RM: i8080_ret(cpu, cpu->s); break;     /* RET on S i.e. acc negative */

        /* Jumps */
        case i8080_JMP: case i8080_ALT_JMP0:
            i8080_jmp(cpu, 1); break;
        case i8080_JNZ: i8080_jmp(cpu, !cpu->z); break;    /* JMP on !Z i.e. non-zero acc */
        case i8080_JZ: i8080_jmp(cpu, cpu->z); break;      /* JMP on Z i.e. zero acc */
        case i8080_JNC: i8080_jmp(cpu, !cpu->cy); break;   /* JMP on !CY i.e. carry reset */
        case i8080_JC: i8080_jmp(cpu, cpu->cy); break;     /* JMP on CY i.e. carry set */
        case i8080_JPO: i8080_jmp(cpu, !cpu->p); break;    /* JMP on !P i.e. acc parity odd */
        case i8080_JPE: i8080_jmp(cpu, cpu->p); break;     /* JMP on P i.e. acc parity even */
        case i8080_JP: i8080_jmp(cpu, !cpu->s); break;     /* JMP on !S i.e. acc positive */
        case i8080_JM: i8080_jmp(cpu, cpu->s); break;      /* JMP on S i.e. acc negative */

        /* Special instructions */
        case i8080_DAA: i8080_daa(cpu); break;                                           /* Decimal adjust acc (convert acc to BCD) */
        case i8080_CMA: cpu->a = ~cpu->a; break;                                         /* Complement accumulator */
        case i8080_STC: cpu->cy = 1; break;                                              /* Set carry */
        case i8080_CMC: cpu->cy = !cpu->cy; break;                                       /* Complement carry */
        case i8080_PCHL: i8080_jmp_addr(cpu, (i8080_addr_t)i8080_get_hl(cpu)); break;    /* Move HL into PC */
        case i8080_SPHL: cpu->sp = (i8080_addr_t)i8080_get_hl(cpu); break;               /* Move HL into SP */
        case i8080_XTHL: i8080_xthl(cpu); break;                                         /* Exchange top of stack with H&L */
        case i8080_XCHG: i8080_xchg(cpu); break;                                         /* Exchanges the contents of BC and DE */

        /* I/O, accumulator <-> word */
		case i8080_IN: i8080_in(cpu); break;
		case i8080_OUT: i8080_out(cpu); break;

        /* Restart / soft interrupts */
        case i8080_RST_0: i8080_call_addr(cpu, (i8080_addr_t)0x0000); break;
        case i8080_RST_1: i8080_call_addr(cpu, (i8080_addr_t)0x0008); break;
        case i8080_RST_2: i8080_call_addr(cpu, (i8080_addr_t)0x0010); break;
        case i8080_RST_3: i8080_call_addr(cpu, (i8080_addr_t)0x0018); break;
        case i8080_RST_4: i8080_call_addr(cpu, (i8080_addr_t)0x0020); break;
        case i8080_RST_5: i8080_call_addr(cpu, (i8080_addr_t)0x0028); break;
        case i8080_RST_6: i8080_call_addr(cpu, (i8080_addr_t)0x0030); break;
		case i8080_RST_7: {
			if (cpu->debugger.is_attached) {
				err = cpu->debugger.on_breakpoint(cpu);
			} else {
				i8080_call_addr(cpu, (i8080_addr_t)0x0038);
			}
			break;
		}

        /* Enable / disable interrupts */
        case i8080_EI: cpu->inte = 1; break;
        case i8080_DI: cpu->inte = 0; break;

        /* HALT cpu. Can only be brought out by interrupt or RESET. */
        case i8080_HLT: cpu->halt = 1; break;

        default: err = -1; /* unrecognized opcode */
    }

	if (!err) cpu->cycles += OPCODES_CYCLES[opcode];
    return err;
}
