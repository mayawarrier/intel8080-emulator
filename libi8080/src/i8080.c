
#include "i8080.h"
#include "i8080_opcodes.h"

#if defined (__STDC__) && !defined(__STDC_VERSION__)
	#define inline
#endif

/*
 * Indexed by opcode.
 * For conditional RETs and CALLs, add 6 if condition is true.
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

#define F_CARRY_BIT     (0)
#define F_PARITY_BIT    (2)
#define F_AUX_CARRY_BIT (4)
#define F_ZERO_BIT      (6)
#define F_SIGN_BIT      (7)

#define WORD_SIZE (8)
#define WORD_MAX (255)
#define ADDR_MAX (65535)

static const i8080_word_t HEX0F = ((i8080_word_t)1 << (WORD_SIZE / 2)) - (i8080_word_t)1;
static const i8080_word_t HEXF0 = (((i8080_word_t)1 << (WORD_SIZE / 2)) - (i8080_word_t)1) << (WORD_SIZE / 2);
static const i8080_dbl_word_t HEXFF00 = (i8080_dbl_word_t)WORD_MAX << WORD_SIZE;

#define i8080_min(a,b) (((a) < (b)) ? (a) : (b))

#define concatenate(word1, word2) ((i8080_dbl_word_t)(word1) << WORD_SIZE | (word2))

#define word_lo_nibble(word) ((i8080_word_t)(word) & HEX0F)
#define word_hi_nibble(word) (((i8080_word_t)(word) & HEXF0) >> (WORD_SIZE / 2))

#define trim_to_addr(dbl_word) ((i8080_dbl_word_t)(dbl_word) & ADDR_MAX)
#define trim_to_word(word) ((i8080_word_t)(word) & WORD_MAX)

#define dbl_hi_word(dbl_word) ((i8080_word_t)(((i8080_dbl_word_t)(dbl_word) & HEXFF00) >> WORD_SIZE))
#define dbl_lo_word(dbl_word) ((i8080_word_t)((i8080_dbl_word_t)(dbl_word) & WORD_MAX))

/* Get a bit from a bit buffer. */
#define get_bit(buf, bit) ((int)(((buf) >> (bit)) & (unsigned)1))

/* Set a bit in a word. */
static inline void set_bit(i8080_word_t *ptr, int bit, int val) {
	if (val) *ptr |= ((i8080_word_t)1 << bit);
	else *ptr &= ~((i8080_word_t)1 << bit);
}

/* Get 2's complement of lower nibble of a word. */
static inline i8080_word_t twos_comp_lo_word(i8080_word_t word) {
	return (word ^ HEX0F) + (i8080_word_t)1;
}

/*
 * Get 2's complement of a word.
 * i8080_dbl_word_t preserves carry on CPI 0:
 * https://retrocomputing.stackexchange.com/questions/6407/intel-8080-behaviour-of-the-carry-bit-when-comparing-a-value-with-0
 */
static inline i8080_dbl_word_t twos_comp_word(i8080_word_t word) {
	return (word ^ (i8080_word_t)WORD_MAX) + (i8080_word_t)1;
}

/* Get auxiliary carry resulting from adding two words. */
static inline int aux_carry(i8080_word_t word1, i8080_word_t word2) {
	return get_bit(word_lo_nibble(word1) + word_lo_nibble(word2), WORD_SIZE / 2);
}

/* Calculate parity of a word. */
static inline int parity(i8080_word_t word) {
	unsigned p = word;
	p ^= (p >> (WORD_SIZE / 2));
	p ^= (p >> (WORD_SIZE / 4));
	p ^= (p >> (WORD_SIZE / 8));
	p &= 1;
	return !(int)p;
}

/* Update Z, S, P flags from a word. */
static inline void update_ZSP(struct i8080 *const cpu, i8080_word_t word) {
	cpu->z = (word == 0);
	/* signed numbers are -127 to 127. bit 7 is the sign bit */
	cpu->s = get_bit(word, WORD_SIZE - 1);
	cpu->p = parity(word);
}

/* Get flags register from emulator bools. */
static inline i8080_word_t i8080_get_flags(struct i8080 *const cpu) {
	/* Bit 1 is always 1:
	 * http://pastraiser.com/cpu/i8080/i8080_opcodes.html */
	i8080_word_t flags = 0x02;
	set_bit(&flags, F_CARRY_BIT, cpu->cy);
	set_bit(&flags, F_PARITY_BIT, cpu->p);
	set_bit(&flags, F_AUX_CARRY_BIT, cpu->acy);
	set_bit(&flags, F_ZERO_BIT, cpu->z);
	set_bit(&flags, F_SIGN_BIT, cpu->s);
	return flags;
}

/* Set emulator bools from flags register. */
static inline void i8080_set_flags(struct i8080 *const cpu, i8080_word_t flags) {
	cpu->cy = get_bit(flags, F_CARRY_BIT);
	cpu->p = get_bit(flags, F_PARITY_BIT);
	cpu->acy = get_bit(flags, F_AUX_CARRY_BIT);
	cpu->z = get_bit(flags, F_ZERO_BIT);
	cpu->s = get_bit(flags, F_SIGN_BIT);
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
	return concatenate(cpu->a, i8080_get_flags(cpu));
}

/* Set {BC} */
static inline void i8080_set_bc(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
	cpu->b = dbl_hi_word(dbl_word);
	cpu->c = dbl_lo_word(dbl_word);
}
/* Set {DE} */
static inline void i8080_set_de(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
	cpu->d = dbl_hi_word(dbl_word);
	cpu->e = dbl_lo_word(dbl_word);
}
/* Set {HL} */
static inline void i8080_set_hl(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
	cpu->h = dbl_hi_word(dbl_word);
	cpu->l = dbl_lo_word(dbl_word);
}
/* Set A, flags from program status word. */
static inline void i8080_set_psw(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
	cpu->a = dbl_hi_word(dbl_word);
	i8080_set_flags(cpu, dbl_lo_word(dbl_word));
}

/* Read word at [HL] */
static inline i8080_word_t i8080_memory_read_indirect(struct i8080 *const cpu) {
	return trim_to_word(cpu->memory_read(cpu, (i8080_addr_t)i8080_get_hl(cpu)));
}

/* Write word to [HL] */
static inline void i8080_memory_write_indirect(struct i8080 *const cpu, i8080_word_t word) {
	cpu->memory_write(cpu, (i8080_addr_t)i8080_get_hl(cpu), word);
}

/* Read a word and advance PC by 1. */
static inline i8080_word_t i8080_advance_read_word(struct i8080 *const cpu) {
	return trim_to_word(cpu->memory_read(cpu, cpu->pc++));
}

/* Read address and advance PC by 2.
 * Assembler inverts the words on assembly:
 * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 84 */
static inline i8080_addr_t i8080_advance_read_addr(struct i8080 *const cpu) {
	i8080_word_t lo_addr = i8080_advance_read_word(cpu);
	i8080_word_t hi_addr = i8080_advance_read_word(cpu);
	return (i8080_addr_t)concatenate(hi_addr, lo_addr);
}

/* Add to accumulator and update flags. */
static void i8080_add(struct i8080 *const cpu, i8080_word_t word) {
	i8080_dbl_word_t acc_buf = (i8080_dbl_word_t)cpu->a + word;
	cpu->acy = aux_carry(cpu->a, word);
	cpu->cy = get_bit(acc_buf, WORD_SIZE);
	cpu->a = dbl_lo_word(acc_buf);
	update_ZSP(cpu, cpu->a);
}

/* Add with carry to accumulator and update flags. */
static inline void i8080_adc(struct i8080 *const cpu, i8080_word_t word) {
	i8080_add(cpu, word + (i8080_word_t)cpu->cy);
}

/* Subtract from accumulator and update flags. */
static void i8080_sub(struct i8080 *const cpu, i8080_word_t word) {
	i8080_dbl_word_t acc_buf = (i8080_dbl_word_t)cpu->a + twos_comp_word(word);
	cpu->acy = aux_carry(cpu->a, twos_comp_lo_word(word));
	/* carry is the borrow flag for SUB, SBB etc, invert. */
	cpu->cy = !get_bit(acc_buf, WORD_SIZE);
	cpu->a = dbl_lo_word(acc_buf);
	update_ZSP(cpu, cpu->a);
}

/* Subtract with borrow from accumulator, and update flags. */
static inline void i8080_sbb(struct i8080 *const cpu, i8080_word_t word) {
	i8080_sub(cpu, word + (i8080_word_t)cpu->cy);
}

/* Do bitwise AND with accumulator and update flags. */
static void i8080_ana(struct i8080 *const cpu, i8080_word_t word) {
	/* AND instructions set aux carry to logical OR of bit 3 of each value:
	 * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 24 */
	cpu->acy = get_bit(cpu->a, (WORD_SIZE / 2) - 1) | get_bit(word, (WORD_SIZE / 2) - 1);
	cpu->a &= word;
	update_ZSP(cpu, cpu->a);
	/* AND instructions always reset carry:
	 * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 63 */
	cpu->cy = 0;
}

/* Do bitwise exclusive OR with accumulator and update flags. */
static void i8080_xra(struct i8080 *const cpu, i8080_word_t word) {
	cpu->a ^= word;
	update_ZSP(cpu, cpu->a);
	/* OR instructions always reset carry and auxiliary carry:
	 * https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel, pg 122 */
	cpu->acy = 0;
	cpu->cy = 0;
}

/* Do bitwise inclusive OR with accumulator and update flags. */
static void i8080_ora(struct i8080 *const cpu, i8080_word_t word) {
	cpu->a |= word;
	update_ZSP(cpu, cpu->a);
	cpu->acy = 0;
	cpu->cy = 0;
}

/* Do i8080_sub() but without modifying the accumulator. */
static void i8080_cmp(struct i8080 *const cpu, i8080_word_t word) {
	i8080_dbl_word_t acc_buf = (i8080_dbl_word_t)cpu->a + twos_comp_word(word);
	cpu->acy = aux_carry(cpu->a, twos_comp_lo_word(word));
	cpu->cy = !get_bit(acc_buf, WORD_SIZE);
	update_ZSP(cpu, dbl_lo_word(acc_buf));
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

/* Add double word to HL and update flags. */
static void i8080_dad(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
	i8080_dbl_word_t hl = i8080_get_hl(cpu);
	i8080_dbl_word_t hl_new = trim_to_addr(hl + dbl_word);
	i8080_set_hl(cpu, hl_new);
	/* If addition overflowed, result must be lower than addands */
	cpu->cy = (hl_new < i8080_min(hl, dbl_word)) ? 1 : 0;
}

/* Store HL to immediate address. */
static inline void i8080_shld(struct i8080 *const cpu) {
	i8080_addr_t addr = i8080_advance_read_addr(cpu);
	cpu->memory_write(cpu, addr++, cpu->l);
	cpu->memory_write(cpu, addr, cpu->h);
}

/* Load HL from immediate address. */
static inline void i8080_lhld(struct i8080 *const cpu) {
	i8080_addr_t addr = i8080_advance_read_addr(cpu);
	cpu->l = trim_to_word(cpu->memory_read(cpu, addr++));
	cpu->h = trim_to_word(cpu->memory_read(cpu, addr));
}

/* Circular shift accumulator left, set carry to old MSB. */
static void i8080_rlc(struct i8080 *const cpu) {
	int msb = get_bit(cpu->a, WORD_SIZE - 1);
	cpu->a <<= 1;
	set_bit(&cpu->a, 0, msb);
	cpu->cy = msb;
}

/* Circular shift accumulator right, set carry to old LSB. */
static void i8080_rrc(struct i8080 *const cpu) {
	int lsb = get_bit(cpu->a, 0);
	cpu->a >>= 1;
	set_bit(&cpu->a, WORD_SIZE - 1, lsb);
	cpu->cy = lsb;
}

/* Circular shift accumulator left through carry. */
static void i8080_ral(struct i8080 *const cpu) {
	int prev_cy = cpu->cy;
	int msb = get_bit(cpu->a, WORD_SIZE - 1);
	cpu->a <<= 1;
	cpu->cy = msb;
	set_bit(&cpu->a, 0, prev_cy);
}

/* Circular shift accumulator right through carry. */
static void i8080_rar(struct i8080 *const cpu) {
	int prev_cy = cpu->cy;
	int lsb = get_bit(cpu->a, 0);
	cpu->a >>= 1;
	cpu->cy = lsb;
	set_bit(&cpu->a, WORD_SIZE - 1, prev_cy);
}

/* Decimal adjust the accumulator (convert to 4-bit BCD) and update flags. */
static void i8080_daa(struct i8080 *const cpu) {
	/* adjust lo nibble */
	i8080_word_t lo = word_lo_nibble(cpu->a);
	if (lo > 9 || cpu->acy) {
		cpu->acy = aux_carry(cpu->a, 0x6);
		cpu->a += 0x6;
	}
	/* adjust hi nibble */
	i8080_word_t hi = word_hi_nibble(cpu->a);
	if (hi > 9 || cpu->cy) {
		i8080_dbl_word_t a = (i8080_dbl_word_t)cpu->a + 0x60;
		cpu->cy = get_bit(a, WORD_SIZE);
		cpu->a = dbl_lo_word(a);
	}
	update_ZSP(cpu, cpu->a);
}

/* Push two words to stack and decrement stack pointer by 2.
 * (upper then lower word) */
static void i8080_push(struct i8080 *const cpu, i8080_dbl_word_t dbl_word) {
	i8080_word_t upper_word = dbl_hi_word(dbl_word);
	i8080_word_t lower_word = dbl_lo_word(dbl_word);
	cpu->memory_write(cpu, --cpu->sp, upper_word);
	cpu->memory_write(cpu, --cpu->sp, lower_word);
}

/* Pop two words from stack and increment stack pointer by 2.
 * (lower then upper word) */
static i8080_dbl_word_t i8080_pop(struct i8080 *const cpu) {
	i8080_word_t lower_word = trim_to_word(cpu->memory_read(cpu, cpu->sp++));
	i8080_word_t upper_word = trim_to_word(cpu->memory_read(cpu, cpu->sp++));
	return concatenate(upper_word, lower_word);
}

#define i8080_jmp_addr(cpu, addr) ((cpu)->pc = (addr))

/* Push PC and jump to addr. */
static inline void i8080_call_addr(struct i8080 *const cpu, i8080_addr_t addr) {
	i8080_push(cpu, (i8080_dbl_word_t)cpu->pc);
	i8080_jmp_addr(cpu, addr);
}

/* Jump to immediate address if condition is satisfied. */
static void i8080_jmp(struct i8080 *const cpu, int condition) {
	if (condition) i8080_jmp_addr(cpu, i8080_advance_read_addr(cpu));
	else cpu->pc += 2; /* advance to word after address */	 
}

/* Push PC and jump to immediate address if condition is satisfied.
 * (add 6 cycles if satisfied) */
static void i8080_call(struct i8080 *const cpu, int condition) {
	if (condition) {
		i8080_call_addr(cpu, i8080_advance_read_addr(cpu));
		cpu->cycles += 6;
	} else cpu->pc += 2;
}

/* Pop PC and jump to it if condition is satisfied.
 * (add 6 cycles if satisfied) */
static void i8080_ret(struct i8080 *const cpu, int condition) {
	if (condition) {
		i8080_jmp_addr(cpu, (i8080_addr_t)i8080_pop(cpu));
		cpu->cycles += 6;
	}
}

/* Exchange HL with top two words on the stack. */
static inline void i8080_xthl(struct i8080 *const cpu) {
	i8080_word_t prev_lower_word = trim_to_word(cpu->memory_read(cpu, cpu->sp++));
	i8080_word_t prev_upper_word = trim_to_word(cpu->memory_read(cpu, cpu->sp));
	cpu->memory_write(cpu, cpu->sp--, cpu->h);
	cpu->memory_write(cpu, cpu->sp, cpu->l);
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

 /* Read input port into accumulator. */
static inline int i8080_in(struct i8080 *const cpu) {
	if (!cpu->io_read) return -1;
	i8080_word_t port_addr = i8080_advance_read_word(cpu);
	cpu->a = trim_to_word(cpu->io_read(cpu, (i8080_addr_t)concatenate(port_addr, port_addr)));
	if (cpu->mexitcode) return -1;
	return 0;
}

/* Write accumulator to output port. */
static inline int i8080_out(struct i8080 *const cpu) {
	if (!cpu->io_write) return -1;
	i8080_word_t port_addr = i8080_advance_read_word(cpu);
	cpu->io_write(cpu, (i8080_addr_t)concatenate(port_addr, port_addr), cpu->a);
	if (cpu->mexitcode) return -1;
	return 0;
}

#define monitor_attached(cpu, fun) ((cpu)->monitor && (cpu)->monitor->fun)

/* Set halt, signal monitor if state changed */
static inline void set_notify_halt(struct i8080 *const cpu, int val) {
	if (monitor_attached(cpu, on_halt_changed) &&
		cpu->halt != val) cpu->monitor->on_halt_changed(cpu, val);
	cpu->halt = val;
}

void i8080_init(struct i8080 *const cpu) {
	cpu->cycles = 0;
	cpu->intr = 0;
	cpu->halt = 0;
	cpu->mexitcode = 0;
	cpu->io_read = 0;
	cpu->io_write = 0;
	cpu->interrupt_read = 0;
	cpu->monitor = 0;
}

void i8080_reset(struct i8080 *const cpu) {
	cpu->pc = 0;
	cpu->inte = 0;
	/* won't notify if called right after i8080_init */
	set_notify_halt(cpu, 0);
}

void i8080_interrupt(struct i8080 *const cpu) {
	if (cpu->inte) cpu->intr = 1;
}

int i8080_next(struct i8080 *const cpu) {
	/* Service interrupt if pending */
	if (cpu->inte && cpu->intr) {
		if (!cpu->interrupt_read) return -1;
		i8080_word_t opcode = trim_to_word(cpu->interrupt_read(cpu));
		if (cpu->mexitcode) return -1;
		/* disable interrupts and bring out of halt */
		cpu->inte = 0;
		cpu->intr = 0;
		set_notify_halt(cpu, 0);
		return i8080_exec(cpu, opcode);
	}

	if (cpu->halt) return 0; /* do nothing until cleared */

	/* execute next opcode in memory */
	return i8080_exec(cpu, i8080_advance_read_word(cpu));
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
		case i8080_INX_B: i8080_set_bc(cpu, i8080_get_bc(cpu) + 1); break;
		case i8080_INX_D: i8080_set_de(cpu, i8080_get_de(cpu) + 1); break;
		case i8080_INX_H: i8080_set_hl(cpu, i8080_get_hl(cpu) + 1); break;
		case i8080_INX_SP: cpu->sp += 1; break;					
		case i8080_DCX_B: i8080_set_bc(cpu, i8080_get_bc(cpu) - 1); break;
		case i8080_DCX_D: i8080_set_de(cpu, i8080_get_de(cpu) - 1); break;
		case i8080_DCX_H: i8080_set_hl(cpu, i8080_get_hl(cpu) - 1); break;
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
		case i8080_STA: cpu->memory_write(cpu, i8080_advance_read_addr(cpu), cpu->a); break;
		case i8080_LDA: cpu->a = trim_to_word(cpu->memory_read(cpu, i8080_advance_read_addr(cpu))); break;

		/* Load/store accumulator, double register indirect */
		case i8080_LDAX_B: cpu->a = trim_to_word(cpu->memory_read(cpu, (i8080_addr_t)i8080_get_bc(cpu))); break;
		case i8080_LDAX_D: cpu->a = trim_to_word(cpu->memory_read(cpu, (i8080_addr_t)i8080_get_de(cpu))); break;
		case i8080_STAX_B: cpu->memory_write(cpu, (i8080_addr_t)i8080_get_bc(cpu), cpu->a); break;
		case i8080_STAX_D: cpu->memory_write(cpu, (i8080_addr_t)i8080_get_de(cpu), cpu->a); break;

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
		case i8080_CMA: cpu->a = ~cpu->a; break;                                         /* Complement accumulator */
		case i8080_STC: cpu->cy = 1; break;                                              /* Set carry */
		case i8080_CMC: cpu->cy = !cpu->cy; break;                                       /* Complement carry */
		case i8080_PCHL: i8080_jmp_addr(cpu, (i8080_addr_t)i8080_get_hl(cpu)); break;    /* Move HL into PC */
		case i8080_SPHL: cpu->sp = (i8080_addr_t)i8080_get_hl(cpu); break;               /* Move HL into SP */
		case i8080_DAA: i8080_daa(cpu); break;
		case i8080_XTHL: i8080_xthl(cpu); break;
		case i8080_XCHG: i8080_xchg(cpu); break;

		/* I/O, accumulator <-> word */
		case i8080_IN: err = i8080_in(cpu); break;
		case i8080_OUT: err = i8080_out(cpu); break;

		/* Restart / soft interrupts */
		case i8080_RST_0: i8080_call_addr(cpu, 0x0000); break;
		case i8080_RST_1: i8080_call_addr(cpu, 0x0008); break;
		case i8080_RST_2: i8080_call_addr(cpu, 0x0010); break;
		case i8080_RST_3: i8080_call_addr(cpu, 0x0018); break;
		case i8080_RST_4: i8080_call_addr(cpu, 0x0020); break;
		case i8080_RST_5: i8080_call_addr(cpu, 0x0028); break;
		case i8080_RST_6: i8080_call_addr(cpu, 0x0030); break;
		case i8080_RST_7:
			if (monitor_attached(cpu, on_rst7)) {
				cpu->mexitcode = cpu->monitor->on_rst7(cpu);
				err = cpu->mexitcode ? -1 : 0;
			} else i8080_call_addr(cpu, 0x0038); 
			break;

		/* Enable / disable interrupts */
		case i8080_EI: cpu->inte = 1; break;
		case i8080_DI: cpu->inte = 0; break;

		/* Halt. Brought out by interrupt or RESET. */
		case i8080_HLT: set_notify_halt(cpu, 1); break;

		default: err = -1; /* unrecognized opcode */
	}

	if (!err) cpu->cycles += OPCODES_CYCLES[opcode];
	return err;
}
