
#include "i8080.h"
#include "i8080_opcodes.h"

#if defined (__STDC__) && !defined(__STDC_VERSION__)
	#define inline
#endif

#ifdef __builtin_expect
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define min2(a, b) (((a) < (b)) ? (a) : (b))

#define F_CARRY_BIT     (0)
#define F_PARITY_BIT    (2)
#define F_AUX_CARRY_BIT (4)
#define F_ZERO_BIT      (6)
#define F_SIGN_BIT      (7)

#define WORD_SIZE (8)
#define WORD_MAX (0xff)
#define DBLWORD_MAX (0xffff)

#define word_lo(word) ((word) & 0x0f)
#define word_hi(word) ((i8080_word_t)((word) & 0xf0) >> (WORD_SIZE / 2))

#define dblword_lo(dblword) ((i8080_word_t)((dblword) & WORD_MAX))
#define dblword_hi(dblword) ((i8080_word_t)((i8080_dbl_word_t)((dblword) & 0xff00) >> WORD_SIZE))

#define limit_to_word(word) ((word) & WORD_MAX)
#define limit_to_dblword(dblword) ((dblword) & DBLWORD_MAX)
#define limit_to_addr(addr) limit_to_dblword(addr)

#define concatenate(word1, word2) (((i8080_dbl_word_t)(word1) << WORD_SIZE) | (word2))

#define get_bit(buf, bit) (((buf) >> (bit)) & 0x1)

static inline void set_bit(i8080_word_t *ptr, unsigned bit, unsigned val) {
	if (val) *ptr = limit_to_word(*ptr | (0x1 << bit));
    else *ptr = limit_to_word(*ptr & ~(0x1 << bit));
}

/* Indexed by opcode. */
/* For conditional RETs and CALLs, add 6 if condition is true. */
static const unsigned int CYCLES[] = {
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

/* Get aux carry (carry out of bit 3) */
static inline unsigned aux_carry(i8080_word_t word1, i8080_word_t word2, i8080_word_t cy) {
	return get_bit(word_lo(word1) + word_lo(word2) + cy, WORD_SIZE / 2);
}

static inline unsigned parity(i8080_word_t word) {
	unsigned p = word;
	p ^= (p >> (WORD_SIZE / 2));
	p ^= (p >> (WORD_SIZE / 4));
	p ^= (p >> (WORD_SIZE / 8));
	p &= 1;
	return !p;
}

/* Update z, s, p flags. */
static inline void update_zsp(struct i8080 *const cpu, i8080_word_t word) {
	cpu->z = (word == 0);
	/* signed numbers are -127 to 127. bit 7 is the sign bit */
	cpu->s = get_bit(word, WORD_SIZE - 1);
	cpu->p = parity(word);
}

static inline i8080_word_t get_flags(struct i8080 *const cpu) {
	/* Bit 1 is always 1:
	 * http://pastraiser.com/cpu/i8080/i8080_opcodes.html */
	i8080_word_t flags = 0x02;
	set_bit(&flags, F_CARRY_BIT, cpu->cy);
	set_bit(&flags, F_PARITY_BIT, cpu->p);
	set_bit(&flags, F_AUX_CARRY_BIT, cpu->ac);
	set_bit(&flags, F_ZERO_BIT, cpu->z);
	set_bit(&flags, F_SIGN_BIT, cpu->s);
	return flags;
}

static inline void set_flags(struct i8080 *const cpu, i8080_word_t flags) {
	cpu->cy = get_bit(flags, F_CARRY_BIT);
	cpu->p = get_bit(flags, F_PARITY_BIT);
	cpu->ac = get_bit(flags, F_AUX_CARRY_BIT);
	cpu->z = get_bit(flags, F_ZERO_BIT);
	cpu->s = get_bit(flags, F_SIGN_BIT);
}

static inline i8080_dbl_word_t get_bc(struct i8080 *const cpu) {
	return concatenate(cpu->b, cpu->c);
}
static inline i8080_dbl_word_t get_de(struct i8080 *const cpu) {
	return concatenate(cpu->d, cpu->e);
}
static inline i8080_dbl_word_t get_hl(struct i8080 *const cpu) {
	return concatenate(cpu->h, cpu->l);
}

/* Get program status (A, flags). */
static inline i8080_dbl_word_t get_psw(struct i8080 *const cpu) {
	return concatenate(cpu->a, get_flags(cpu));
}

static inline void set_bc(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
	cpu->b = dblword_hi(dbl_word);
	cpu->c = dblword_lo(dbl_word);
}
static inline void set_de(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
	cpu->d = dblword_hi(dbl_word);
	cpu->e = dblword_lo(dbl_word);
}
static inline void set_hl(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
	cpu->h = dblword_hi(dbl_word);
	cpu->l = dblword_lo(dbl_word);
}

/* Set program status (A, flags). */
static inline void set_psw(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
	cpu->a = dblword_hi(dbl_word);
	set_flags(cpu, dblword_lo(dbl_word));
}

/* Read word at [HL] */
static inline i8080_word_t read_word_indirect(struct i8080 *const cpu) {
	return limit_to_word(cpu->memory_read(cpu, (i8080_addr_t)get_hl(cpu)));
}

/* Write word to [HL] */
static inline void write_word_indirect(struct i8080 *const cpu, i8080_word_t word) {
	cpu->memory_write(cpu, (i8080_addr_t)get_hl(cpu), word);
}

/* Read word and advance PC. */
static inline i8080_word_t read_word_advance(struct i8080 *const cpu) {
	i8080_word_t word = limit_to_word(cpu->memory_read(cpu, cpu->pc));
    cpu->pc = limit_to_dblword(cpu->pc + 1);
    return word;
}

/* Read address and advance PC by 2. */ 
static inline i8080_addr_t read_addr_advance(struct i8080 *const cpu) {
	i8080_word_t lo = read_word_advance(cpu);
	i8080_word_t hi = read_word_advance(cpu);
	return (i8080_addr_t)concatenate(hi, lo);
}

/* Add to accumulator. */
static void i8080_add(struct i8080 *const cpu, i8080_word_t word, i8080_word_t cy) {
    i8080_dbl_word_t res = (i8080_dbl_word_t)cpu->a + word + cy;
    cpu->ac = aux_carry(cpu->a, word, cy);
    cpu->cy = get_bit(res, WORD_SIZE);
    cpu->a = dblword_lo(res);
    update_zsp(cpu, cpu->a);
}

/* Subtract from accumulator. */
static void i8080_sub(struct i8080 *const cpu, i8080_word_t word, i8080_word_t cy) {
	i8080_dbl_word_t res = (i8080_dbl_word_t)cpu->a + (word ^ WORD_MAX) + !cy;
	cpu->ac = aux_carry(cpu->a, word ^ 0x0f, !cy);
	/* carry is the borrow flag for SUB, SBB etc, invert. */
	cpu->cy = !get_bit(res, WORD_SIZE);
	cpu->a = dblword_lo(res);
	update_zsp(cpu, cpu->a);
}

/* Bitwise AND with accumulator. */
static void i8080_ana(struct i8080 *const cpu, i8080_word_t word) {
	/* AND instructions set aux carry to OR of bit 3:
	 * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 24 */
	cpu->ac = get_bit(cpu->a, (WORD_SIZE / 2) - 1) | get_bit(word, (WORD_SIZE / 2) - 1);
	cpu->a &= word;
	update_zsp(cpu, cpu->a);
	/* AND instructions reset carry:
	 * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 63 */
	cpu->cy = 0;
}

/* Bitwise exclusive OR with accumulator. */
static void i8080_xra(struct i8080 *const cpu, i8080_word_t word) {
	cpu->a ^= word;
	update_zsp(cpu, cpu->a);
	/* OR instructions reset carry and aux carry:
	 * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 122 */
	cpu->ac = 0;
	cpu->cy = 0;
}

/* Bitwise inclusive OR with accumulator. */
static void i8080_ora(struct i8080 *const cpu, i8080_word_t word) {
	cpu->a |= word;
	update_zsp(cpu, cpu->a);
	cpu->ac = 0;
	cpu->cy = 0;
}

/* Compare word with accumulator. */
/* (i8080_sub() but without modifying the accumulator) */
static void i8080_cmp(struct i8080 *const cpu, i8080_word_t word) {
	i8080_dbl_word_t res = (i8080_dbl_word_t)cpu->a + (word ^ WORD_MAX) + 1;
	cpu->ac = aux_carry(cpu->a, word ^ 0x0f, 1);
	cpu->cy = !get_bit(res, WORD_SIZE);
	update_zsp(cpu, dblword_lo(res));
}

/* Increment word. */
static i8080_word_t i8080_inr(struct i8080 *const cpu, i8080_word_t word) {
	cpu->ac = aux_carry(word, 1, 0);
	word = limit_to_word(word + 1);
	update_zsp(cpu, word);
	return word;
}

/* Decrement word. */
static i8080_word_t i8080_dcr(struct i8080 *const cpu, i8080_word_t word) {
	cpu->ac = aux_carry(word, 0xf /* (1 ^ 0x0f) + 1 */, 0);
	word = limit_to_word(word - 1);
	update_zsp(cpu, word);
	return word;
}

/* Add double word to HL. */
static void i8080_dad(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
	i8080_dbl_word_t old_hl = get_hl(cpu);
	i8080_dbl_word_t new_hl = limit_to_dblword(old_hl + dbl_word);
	set_hl(cpu, new_hl);
	/* on overflow, result must be lower than addands */
	cpu->cy = (new_hl < min2(old_hl, dbl_word)) ? 1 : 0;
}

/* Store HL to immediate address. */
static inline void i8080_shld(struct i8080 *const cpu) {
	i8080_addr_t addr = read_addr_advance(cpu);
	cpu->memory_write(cpu, addr, cpu->l);
    addr = limit_to_addr(addr + 1);
	cpu->memory_write(cpu, addr, cpu->h);
}

/* Load HL from immediate address. */
static inline void i8080_lhld(struct i8080 *const cpu) {
	i8080_addr_t addr = read_addr_advance(cpu);
	cpu->l = limit_to_word(cpu->memory_read(cpu, addr));
    addr = limit_to_addr(addr + 1);
	cpu->h = limit_to_word(cpu->memory_read(cpu, addr));
}

/* Circular shift accumulator left, set carry to old MSB. */
static void i8080_rlc(struct i8080 *const cpu) {
	unsigned msb = get_bit(cpu->a, WORD_SIZE - 1);
	cpu->a <<= 1;
	set_bit(&cpu->a, 0, msb);
	cpu->cy = msb;
}

/* Circular shift accumulator right, set carry to old LSB. */
static void i8080_rrc(struct i8080 *const cpu) {
	unsigned lsb = get_bit(cpu->a, 0);
	cpu->a >>= 1;
	set_bit(&cpu->a, WORD_SIZE - 1, lsb);
	cpu->cy = lsb;
}

/* Circular shift accumulator left through carry. */
static void i8080_ral(struct i8080 *const cpu) {
	unsigned old_cy = cpu->cy;
	unsigned msb = get_bit(cpu->a, WORD_SIZE - 1);
	cpu->a <<= 1;
	cpu->cy = msb;
	set_bit(&cpu->a, 0, old_cy);
}

/* Circular shift accumulator right through carry. */
static void i8080_rar(struct i8080 *const cpu) {
	unsigned old_cy = cpu->cy;
	unsigned lsb = get_bit(cpu->a, 0);
	cpu->a >>= 1;
	cpu->cy = lsb;
	set_bit(&cpu->a, WORD_SIZE - 1, old_cy);
}

/* Decimal adjust accumulator (convert to 4-bit BCD). */
static void i8080_daa(struct i8080 *const cpu) {
	/* adjust lo nibble */
	i8080_word_t lo = word_lo(cpu->a);
	if (lo > 9 || cpu->ac) {
		cpu->ac = aux_carry(cpu->a, 0x6, 0);
		cpu->a += 0x6;
	}
	/* adjust hi nibble */
	i8080_word_t hi = word_hi(cpu->a);
	if (hi > 9 || cpu->cy) {
		i8080_dbl_word_t a = (i8080_dbl_word_t)cpu->a + 0x60;
		cpu->cy = get_bit(a, WORD_SIZE);
		cpu->a = dblword_lo(a);
	}
	update_zsp(cpu, cpu->a);
}

/* Push two words to stack. */
static void i8080_push(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
	i8080_word_t hi = dblword_hi(dbl_word);
	i8080_word_t lo = dblword_lo(dbl_word);
    cpu->sp = limit_to_addr(cpu->sp - 1);
	cpu->memory_write(cpu, cpu->sp, hi);
    cpu->sp = limit_to_addr(cpu->sp - 1);
	cpu->memory_write(cpu, cpu->sp, lo);
}

/* Pop two words from stack. */
static i8080_dbl_word_t i8080_pop(struct i8080 *const cpu) {
	i8080_word_t lo = limit_to_word(cpu->memory_read(cpu, cpu->sp));
    cpu->sp = limit_to_addr(cpu->sp + 1);
	i8080_word_t hi = limit_to_word(cpu->memory_read(cpu, cpu->sp));
    cpu->sp = limit_to_addr(cpu->sp + 1);
	return concatenate(hi, lo);
}

#define i8080_jmp_addr(cpu, addr) ((cpu)->pc = (addr))

/* Push PC and jump to addr. */
static inline void i8080_call_addr(struct i8080 *const cpu, i8080_addr_t addr) {
	i8080_push(cpu, (i8080_dbl_word_t)cpu->pc);
	i8080_jmp_addr(cpu, addr);
}

/* Conditional jump to immediate address. */
static void i8080_jmp(struct i8080 *const cpu, int condition) {
	if (condition) i8080_jmp_addr(cpu, read_addr_advance(cpu));
	else cpu->pc += 2; /* skip address */	 
}

/* Conditional call (push PC + jump) to immediate address. */
static void i8080_call(struct i8080 *const cpu, int condition) {
	if (condition) {
		i8080_call_addr(cpu, read_addr_advance(cpu));
		cpu->cycles += 6;
	} else cpu->pc += 2;
}

/* Conditional return to last address on the stack. */
static void i8080_ret(struct i8080 *const cpu, int condition) {
	if (condition) {
		i8080_jmp_addr(cpu, (i8080_addr_t)i8080_pop(cpu));
		cpu->cycles += 6;
	}
}

/* Exchange HL with top two words on the stack. */
static inline void i8080_xthl(struct i8080 *const cpu) {
	i8080_word_t lo = limit_to_word(cpu->memory_read(cpu, cpu->sp));
    cpu->sp = limit_to_addr(cpu->sp + 1);
	i8080_word_t hi = limit_to_word(cpu->memory_read(cpu, cpu->sp));
	cpu->memory_write(cpu, cpu->sp, cpu->h);
    cpu->sp = limit_to_addr(cpu->sp - 1);
	cpu->memory_write(cpu, cpu->sp, cpu->l);
	cpu->h = hi;
	cpu->l = lo;
}

/* Exchange BC and HL. */
static inline void i8080_xchg(struct i8080 *const cpu) {
	i8080_dbl_word_t hl = get_hl(cpu);
	set_hl(cpu, get_de(cpu));
	set_de(cpu, hl);
}

#define RES_SUCCESS (0)
#define RES_FAILURE (-1)

static int i8080_exec(struct i8080 *const cpu, i8080_word_t opcode) {
	switch (opcode)
	{
		/* Documented & undocumented NOPs. Do nothing. */
		case i8080_NOP: case i8080_ALT_NOP0: case i8080_ALT_NOP1: case i8080_ALT_NOP2:
		case i8080_ALT_NOP3: case i8080_ALT_NOP4: case i8080_ALT_NOP5: case i8080_ALT_NOP6:
			break;

		/* Move between registers */
		case i8080_MOV_B_C: cpu->b = cpu->c; break; case i8080_MOV_B_D: cpu->b = cpu->d; break; case i8080_MOV_B_E: cpu->b = cpu->e; break;
		case i8080_MOV_B_H: cpu->b = cpu->h; break; case i8080_MOV_B_L: cpu->b = cpu->l; break; case i8080_MOV_B_A: cpu->b = cpu->a; break;
		case i8080_MOV_C_B: cpu->c = cpu->b; break; case i8080_MOV_C_D: cpu->c = cpu->d; break; case i8080_MOV_C_E: cpu->c = cpu->e; break;
		case i8080_MOV_C_H: cpu->c = cpu->h; break; case i8080_MOV_C_L: cpu->c = cpu->l; break; case i8080_MOV_C_A: cpu->c = cpu->a; break;
		case i8080_MOV_D_C: cpu->d = cpu->c; break; case i8080_MOV_D_B: cpu->d = cpu->b; break; case i8080_MOV_D_E: cpu->d = cpu->e; break;
		case i8080_MOV_D_H: cpu->d = cpu->h; break; case i8080_MOV_D_L: cpu->d = cpu->l; break; case i8080_MOV_D_A: cpu->d = cpu->a; break;
		case i8080_MOV_E_C: cpu->e = cpu->c; break; case i8080_MOV_E_D: cpu->e = cpu->d; break; case i8080_MOV_E_B: cpu->e = cpu->b; break;
		case i8080_MOV_E_H: cpu->e = cpu->h; break; case i8080_MOV_E_L: cpu->e = cpu->l; break; case i8080_MOV_E_A: cpu->e = cpu->a; break;
		case i8080_MOV_H_C: cpu->h = cpu->c; break; case i8080_MOV_H_D: cpu->h = cpu->d; break; case i8080_MOV_H_E: cpu->h = cpu->e; break;
		case i8080_MOV_H_B: cpu->h = cpu->b; break; case i8080_MOV_H_L: cpu->h = cpu->l; break; case i8080_MOV_H_A: cpu->h = cpu->a; break;
		case i8080_MOV_L_C: cpu->l = cpu->c; break; case i8080_MOV_L_D: cpu->l = cpu->d; break; case i8080_MOV_L_E: cpu->l = cpu->e; break;
		case i8080_MOV_L_H: cpu->l = cpu->h; break; case i8080_MOV_L_B: cpu->l = cpu->b; break; case i8080_MOV_L_A: cpu->l = cpu->a; break;
		case i8080_MOV_A_C: cpu->a = cpu->c; break; case i8080_MOV_A_D: cpu->a = cpu->d; break; case i8080_MOV_A_E: cpu->a = cpu->e; break;
		case i8080_MOV_A_H: cpu->a = cpu->h; break; case i8080_MOV_A_L: cpu->a = cpu->l; break; case i8080_MOV_A_B: cpu->a = cpu->b; break;
		case i8080_MOV_A_A: case i8080_MOV_B_B: case i8080_MOV_C_C: case i8080_MOV_D_D:
		case i8080_MOV_E_E: case i8080_MOV_H_H: case i8080_MOV_L_L: break;

		/* Move from memory to registers */
		case i8080_MOV_B_M: cpu->b = read_word_indirect(cpu); break;
		case i8080_MOV_C_M: cpu->c = read_word_indirect(cpu); break;
		case i8080_MOV_D_M: cpu->d = read_word_indirect(cpu); break;
		case i8080_MOV_E_M: cpu->e = read_word_indirect(cpu); break;
		case i8080_MOV_H_M: cpu->h = read_word_indirect(cpu); break;
		case i8080_MOV_L_M: cpu->l = read_word_indirect(cpu); break;
		case i8080_MOV_A_M: cpu->a = read_word_indirect(cpu); break;

		/* Move from registers to memory */
		case i8080_MOV_M_B: write_word_indirect(cpu, cpu->b); break;
		case i8080_MOV_M_C: write_word_indirect(cpu, cpu->c); break;
		case i8080_MOV_M_D: write_word_indirect(cpu, cpu->d); break;
		case i8080_MOV_M_E: write_word_indirect(cpu, cpu->e); break;
		case i8080_MOV_M_H: write_word_indirect(cpu, cpu->h); break;
		case i8080_MOV_M_L: write_word_indirect(cpu, cpu->l); break;
		case i8080_MOV_M_A: write_word_indirect(cpu, cpu->a); break;

		/* Move immediate */
		case i8080_MVI_B: cpu->b = read_word_advance(cpu); break;
		case i8080_MVI_C: cpu->c = read_word_advance(cpu); break;
		case i8080_MVI_D: cpu->d = read_word_advance(cpu); break;
		case i8080_MVI_E: cpu->e = read_word_advance(cpu); break;
		case i8080_MVI_H: cpu->h = read_word_advance(cpu); break;
		case i8080_MVI_L: cpu->l = read_word_advance(cpu); break;
		case i8080_MVI_M: write_word_indirect(cpu, read_word_advance(cpu)); break;
		case i8080_MVI_A: cpu->a = read_word_advance(cpu); break;

		/* Addition */
		case i8080_ADD_B: i8080_add(cpu, cpu->b, 0); break;
		case i8080_ADD_C: i8080_add(cpu, cpu->c, 0); break;
		case i8080_ADD_D: i8080_add(cpu, cpu->d, 0); break;
		case i8080_ADD_E: i8080_add(cpu, cpu->e, 0); break;
		case i8080_ADD_H: i8080_add(cpu, cpu->h, 0); break;
		case i8080_ADD_L: i8080_add(cpu, cpu->l, 0); break;
		case i8080_ADD_M: i8080_add(cpu, read_word_indirect(cpu), 0); break;
		case i8080_ADD_A: i8080_add(cpu, cpu->a, 0); break;

		/* Addition with carry */
		case i8080_ADC_B: i8080_add(cpu, cpu->b, (i8080_word_t)cpu->cy); break;
		case i8080_ADC_C: i8080_add(cpu, cpu->c, (i8080_word_t)cpu->cy); break;
		case i8080_ADC_D: i8080_add(cpu, cpu->d, (i8080_word_t)cpu->cy); break;
		case i8080_ADC_E: i8080_add(cpu, cpu->e, (i8080_word_t)cpu->cy); break;
		case i8080_ADC_H: i8080_add(cpu, cpu->h, (i8080_word_t)cpu->cy); break;
		case i8080_ADC_L: i8080_add(cpu, cpu->l, (i8080_word_t)cpu->cy); break;
		case i8080_ADC_M: i8080_add(cpu, read_word_indirect(cpu), (i8080_word_t)cpu->cy); break;
		case i8080_ADC_A: i8080_add(cpu, cpu->a, (i8080_word_t)cpu->cy); break;

		/* Subtraction */
		case i8080_SUB_B: i8080_sub(cpu, cpu->b, 0); break;
		case i8080_SUB_C: i8080_sub(cpu, cpu->c, 0); break;
		case i8080_SUB_D: i8080_sub(cpu, cpu->d, 0); break;
		case i8080_SUB_E: i8080_sub(cpu, cpu->e, 0); break;
		case i8080_SUB_H: i8080_sub(cpu, cpu->h, 0); break;
		case i8080_SUB_L: i8080_sub(cpu, cpu->l, 0); break;
		case i8080_SUB_M: i8080_sub(cpu, read_word_indirect(cpu), 0); break;
		case i8080_SUB_A: i8080_sub(cpu, cpu->a, 0); break;

		/* Subtraction with borrowed carry */
		case i8080_SBB_B: i8080_sub(cpu, cpu->b, (i8080_word_t)cpu->cy); break;
		case i8080_SBB_C: i8080_sub(cpu, cpu->c, (i8080_word_t)cpu->cy); break;
		case i8080_SBB_D: i8080_sub(cpu, cpu->d, (i8080_word_t)cpu->cy); break;
		case i8080_SBB_E: i8080_sub(cpu, cpu->e, (i8080_word_t)cpu->cy); break;
		case i8080_SBB_H: i8080_sub(cpu, cpu->h, (i8080_word_t)cpu->cy); break;
		case i8080_SBB_L: i8080_sub(cpu, cpu->l, (i8080_word_t)cpu->cy); break;
		case i8080_SBB_M: i8080_sub(cpu, read_word_indirect(cpu), (i8080_word_t)cpu->cy); break;
		case i8080_SBB_A: i8080_sub(cpu, cpu->a, (i8080_word_t)cpu->cy); break;

		/* Logical AND with accumulator */
		case i8080_ANA_B: i8080_ana(cpu, cpu->b); break;
		case i8080_ANA_C: i8080_ana(cpu, cpu->c); break;
		case i8080_ANA_D: i8080_ana(cpu, cpu->d); break;
		case i8080_ANA_E: i8080_ana(cpu, cpu->e); break;
		case i8080_ANA_H: i8080_ana(cpu, cpu->h); break;
		case i8080_ANA_L: i8080_ana(cpu, cpu->l); break;
		case i8080_ANA_M: i8080_ana(cpu, read_word_indirect(cpu)); break;
		case i8080_ANA_A: i8080_ana(cpu, cpu->a); break;

		/* Exclusive logical OR with accumulator */
		case i8080_XRA_B: i8080_xra(cpu, cpu->b); break;
		case i8080_XRA_C: i8080_xra(cpu, cpu->c); break;
		case i8080_XRA_D: i8080_xra(cpu, cpu->d); break;
		case i8080_XRA_E: i8080_xra(cpu, cpu->e); break;
		case i8080_XRA_H: i8080_xra(cpu, cpu->h); break;
		case i8080_XRA_L: i8080_xra(cpu, cpu->l); break;
		case i8080_XRA_M: i8080_xra(cpu, read_word_indirect(cpu)); break;
		case i8080_XRA_A: i8080_xra(cpu, cpu->a); break;

		/* Inclusive logical OR with accumulator */
		case i8080_ORA_B: i8080_ora(cpu, cpu->b); break;
		case i8080_ORA_C: i8080_ora(cpu, cpu->c); break;
		case i8080_ORA_D: i8080_ora(cpu, cpu->d); break;
		case i8080_ORA_E: i8080_ora(cpu, cpu->e); break;
		case i8080_ORA_H: i8080_ora(cpu, cpu->h); break;
		case i8080_ORA_L: i8080_ora(cpu, cpu->l); break;
		case i8080_ORA_M: i8080_ora(cpu, read_word_indirect(cpu)); break;
		case i8080_ORA_A: i8080_ora(cpu, cpu->a); break;

		/* Compare with accumulator */
		case i8080_CMP_B: i8080_cmp(cpu, cpu->b); break;
		case i8080_CMP_C: i8080_cmp(cpu, cpu->c); break;
		case i8080_CMP_D: i8080_cmp(cpu, cpu->d); break;
		case i8080_CMP_E: i8080_cmp(cpu, cpu->e); break;
		case i8080_CMP_H: i8080_cmp(cpu, cpu->h); break;
		case i8080_CMP_L: i8080_cmp(cpu, cpu->l); break;
		case i8080_CMP_M: i8080_cmp(cpu, read_word_indirect(cpu)); break;
		case i8080_CMP_A: i8080_cmp(cpu, cpu->a); break;

		/* Register/memory increment */
		case i8080_INR_B: cpu->b = i8080_inr(cpu, cpu->b); break;
		case i8080_INR_C: cpu->c = i8080_inr(cpu, cpu->c); break;
		case i8080_INR_D: cpu->d = i8080_inr(cpu, cpu->d); break;
		case i8080_INR_E: cpu->e = i8080_inr(cpu, cpu->e); break;
		case i8080_INR_H: cpu->h = i8080_inr(cpu, cpu->h); break;
		case i8080_INR_L: cpu->l = i8080_inr(cpu, cpu->l); break;
		case i8080_INR_M: write_word_indirect(cpu, i8080_inr(cpu, read_word_indirect(cpu))); break;
		case i8080_INR_A: cpu->a = i8080_inr(cpu, cpu->a); break;

		/* Register/memory decrement */
		case i8080_DCR_B: cpu->b = i8080_dcr(cpu, cpu->b); break;
		case i8080_DCR_C: cpu->c = i8080_dcr(cpu, cpu->c); break;
		case i8080_DCR_D: cpu->d = i8080_dcr(cpu, cpu->d); break;
		case i8080_DCR_E: cpu->e = i8080_dcr(cpu, cpu->e); break;
		case i8080_DCR_H: cpu->h = i8080_dcr(cpu, cpu->h); break;
		case i8080_DCR_L: cpu->l = i8080_dcr(cpu, cpu->l); break;
		case i8080_DCR_M: write_word_indirect(cpu, i8080_dcr(cpu, read_word_indirect(cpu))); break;
		case i8080_DCR_A: cpu->a = i8080_dcr(cpu, cpu->a); break;

		/* Double register increment/decrement */
		case i8080_INX_B: set_bc(cpu, get_bc(cpu) + 1); break;
		case i8080_INX_D: set_de(cpu, get_de(cpu) + 1); break;
		case i8080_INX_H: set_hl(cpu, get_hl(cpu) + 1); break;
		case i8080_INX_SP: cpu->sp = limit_to_addr(cpu->sp + 1); break;					
		case i8080_DCX_B: set_bc(cpu, get_bc(cpu) - 1); break;
		case i8080_DCX_D: set_de(cpu, get_de(cpu) - 1); break;
		case i8080_DCX_H: set_hl(cpu, get_hl(cpu) - 1); break;
		case i8080_DCX_SP: cpu->sp = limit_to_addr(cpu->sp - 1); break;

		/* Double register add (16-bit addition) */
		case i8080_DAD_B: i8080_dad(cpu, get_bc(cpu)); break;
		case i8080_DAD_D: i8080_dad(cpu, get_de(cpu)); break;
		case i8080_DAD_H: i8080_dad(cpu, get_hl(cpu)); break;
		case i8080_DAD_SP: i8080_dad(cpu, (i8080_dbl_word_t)cpu->sp); break;

		/* Double register load immediate */
		case i8080_LXI_B: cpu->c = read_word_advance(cpu); cpu->b = read_word_advance(cpu); break;
		case i8080_LXI_D: cpu->e = read_word_advance(cpu); cpu->d = read_word_advance(cpu); break;
		case i8080_LXI_H: cpu->l = read_word_advance(cpu); cpu->h = read_word_advance(cpu); break;
		case i8080_LXI_SP: cpu->sp = read_addr_advance(cpu); break;

		/* Load/store accumulator, immediate indirect */
		case i8080_STA: cpu->memory_write(cpu, read_addr_advance(cpu), cpu->a); break;
		case i8080_LDA: cpu->a = limit_to_word(cpu->memory_read(cpu, read_addr_advance(cpu))); break;

		/* Load/store accumulator, double register indirect */
		case i8080_LDAX_B: cpu->a = limit_to_word(cpu->memory_read(cpu, (i8080_addr_t)get_bc(cpu))); break;
		case i8080_LDAX_D: cpu->a = limit_to_word(cpu->memory_read(cpu, (i8080_addr_t)get_de(cpu))); break;
		case i8080_STAX_B: cpu->memory_write(cpu, (i8080_addr_t)get_bc(cpu), cpu->a); break;
		case i8080_STAX_D: cpu->memory_write(cpu, (i8080_addr_t)get_de(cpu), cpu->a); break;

		/* Load/store double register, immediate indirect */
		case i8080_SHLD: i8080_shld(cpu); break;
		case i8080_LHLD: i8080_lhld(cpu); break;

		/* Rotate */
		case i8080_RLC: i8080_rlc(cpu); break;
		case i8080_RRC: i8080_rrc(cpu); break;
		case i8080_RAL: i8080_ral(cpu); break;
		case i8080_RAR: i8080_rar(cpu); break;

		/* Arithmetic / logical / compare immediate */
		case i8080_ADI: i8080_add(cpu, read_word_advance(cpu), 0); break;
		case i8080_ACI: i8080_add(cpu, read_word_advance(cpu), (i8080_word_t)cpu->cy); break;
		case i8080_SUI: i8080_sub(cpu, read_word_advance(cpu), 0); break;
		case i8080_SBI: i8080_sub(cpu, read_word_advance(cpu), (i8080_word_t)cpu->cy); break;
		case i8080_ANI: i8080_ana(cpu, read_word_advance(cpu)); break;
		case i8080_XRI: i8080_xra(cpu, read_word_advance(cpu)); break;
		case i8080_ORI: i8080_ora(cpu, read_word_advance(cpu)); break;
		case i8080_CPI: i8080_cmp(cpu, read_word_advance(cpu)); break;

		/* Stack push / pop */
		case i8080_PUSH_B: i8080_push(cpu, get_bc(cpu)); break;
		case i8080_PUSH_D: i8080_push(cpu, get_de(cpu)); break;
		case i8080_PUSH_H: i8080_push(cpu, get_hl(cpu)); break;
		case i8080_PUSH_PSW: i8080_push(cpu, get_psw(cpu)); break;
		case i8080_POP_B: set_bc(cpu, i8080_pop(cpu)); break;
		case i8080_POP_D: set_de(cpu, i8080_pop(cpu)); break;
		case i8080_POP_H: set_hl(cpu, i8080_pop(cpu)); break;
		case i8080_POP_PSW: set_psw(cpu, i8080_pop(cpu)); break;

		/* Subroutine calls */
		case i8080_CALL: case i8080_ALT_CALL0:
		case i8080_ALT_CALL1: case i8080_ALT_CALL2:
			i8080_call(cpu, 1); break;
		case i8080_CNZ: i8080_call(cpu, !cpu->z); break;
		case i8080_CZ: i8080_call(cpu, cpu->z); break;
		case i8080_CNC: i8080_call(cpu, !cpu->cy); break;
		case i8080_CC: i8080_call(cpu, cpu->cy); break;
		case i8080_CPO: i8080_call(cpu, !cpu->p); break;
		case i8080_CPE: i8080_call(cpu, cpu->p); break;
		case i8080_CP:  i8080_call(cpu, !cpu->s); break;
		case i8080_CM: i8080_call(cpu, cpu->s); break;

		/* Subroutine returns */
		case i8080_RET: case i8080_ALT_RET0:
			i8080_ret(cpu, 1); break;
		case i8080_RNZ: i8080_ret(cpu, !cpu->z); break;
		case i8080_RZ: i8080_ret(cpu, cpu->z); break;
		case i8080_RNC: i8080_ret(cpu, !cpu->cy); break;
		case i8080_RC: i8080_ret(cpu, cpu->cy); break;
		case i8080_RPO: i8080_ret(cpu, !cpu->p); break;
		case i8080_RPE: i8080_ret(cpu, cpu->p); break;
		case i8080_RP: i8080_ret(cpu, !cpu->s); break;
		case i8080_RM: i8080_ret(cpu, cpu->s); break;

		/* Jumps */
		case i8080_JMP: case i8080_ALT_JMP0:
			i8080_jmp(cpu, 1); break;
		case i8080_JNZ: i8080_jmp(cpu, !cpu->z); break;
		case i8080_JZ: i8080_jmp(cpu, cpu->z); break;
		case i8080_JNC: i8080_jmp(cpu, !cpu->cy); break;
		case i8080_JC: i8080_jmp(cpu, cpu->cy); break;
		case i8080_JPO: i8080_jmp(cpu, !cpu->p); break;
		case i8080_JPE: i8080_jmp(cpu, cpu->p); break;
		case i8080_JP: i8080_jmp(cpu, !cpu->s); break;
		case i8080_JM: i8080_jmp(cpu, cpu->s); break;

		/* Special instructions */
		case i8080_CMA: cpu->a = limit_to_word(~cpu->a); break;                    /* Complement accumulator */
		case i8080_STC: cpu->cy = 1; break;                                        /* Set carry */
		case i8080_CMC: cpu->cy = !cpu->cy; break;                                 /* Complement carry */
		case i8080_PCHL: i8080_jmp_addr(cpu, (i8080_addr_t)get_hl(cpu)); break;    /* Move HL into PC */
		case i8080_SPHL: cpu->sp = (i8080_addr_t)get_hl(cpu); break;               /* Move HL into SP */
		case i8080_DAA: i8080_daa(cpu); break;
		case i8080_XTHL: i8080_xthl(cpu); break;
		case i8080_XCHG: i8080_xchg(cpu); break;

		/* I/O port address is duplicated over 16-bit address bus
		 * https://stackoverflow.com/questions/13551973/intel-8080-instruction-out
		 * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 97 */

        /* Set accumulator to input word. */
		case i8080_IN: {
			if (unlikely(!cpu->io_read)) return RES_FAILURE;
			i8080_word_t port = read_word_advance(cpu);
			cpu->a = limit_to_word(cpu->io_read(cpu, (i8080_addr_t)concatenate(port, port)));
			break;
		}
        /* Set output word to accumulator. */
		case i8080_OUT: {
			if (unlikely(!cpu->io_write)) return RES_FAILURE;				
			i8080_word_t port = read_word_advance(cpu);
			cpu->io_write(cpu, (i8080_addr_t)concatenate(port, port), cpu->a);
			break;
		} 

		/* Restart / soft interrupts */
		case i8080_RST_0: i8080_call_addr(cpu, 0x0000); break;
		case i8080_RST_1: i8080_call_addr(cpu, 0x0008); break;
		case i8080_RST_2: i8080_call_addr(cpu, 0x0010); break;
		case i8080_RST_3: i8080_call_addr(cpu, 0x0018); break;
		case i8080_RST_4: i8080_call_addr(cpu, 0x0020); break;
		case i8080_RST_5: i8080_call_addr(cpu, 0x0028); break;
		case i8080_RST_6: i8080_call_addr(cpu, 0x0030); break;
		case i8080_RST_7: i8080_call_addr(cpu, 0x0038); break;

		/* Enable / disable interrupts */
		case i8080_EI: cpu->inte = 1; break;
		case i8080_DI: cpu->inte = 0; break;

		/* Halt. */
		case i8080_HLT: cpu->halt = 1; break;

		default: return RES_FAILURE; /* unrecognized opcode */
	}
	cpu->cycles += CYCLES[opcode];

	return RES_SUCCESS;
}

void i8080_reset(struct i8080 *const cpu) {
    cpu->pc = 0;
    cpu->inte = 0;
    cpu->intr = 0;
    cpu->halt = 0;
    cpu->cycles = 0;
}

void i8080_interrupt(struct i8080 *const cpu) { cpu->intr = cpu->inte; }

int i8080_next(struct i8080 *const cpu) {
    if (unlikely(!cpu->enable)) return RES_FAILURE;

    if (cpu->intr) {
        if (unlikely(!cpu->interrupt_read)) return RES_FAILURE;

        /* disable interrupts, clear halt */
        cpu->inte = 0;
        cpu->intr = 0;
        cpu->halt = 0;

        /* process interrupt */
        return i8080_exec(cpu, limit_to_word(cpu->interrupt_read(cpu)));
    }
    if (unlikely(cpu->halt)) return RES_SUCCESS;

    /* execute next instruction */
    return i8080_exec(cpu, read_word_advance(cpu));
}
