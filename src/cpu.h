#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef enum flag {
    FLAG_CARRY = 0,
    FLAG_PARITY = 2,
    FLAG_AUX_CARRY = 4,
    FLAG_ZERO = 6,
    FLAG_SIGN = 7
} flag;

typedef struct cpu {
    // registers
    uint8_t A;
    uint8_t B;
    uint8_t C;
    uint8_t D;
    uint8_t E;
    uint8_t H;
    uint8_t L;
    uint8_t flags;
    uint16_t SP;
    uint16_t PC;
    
    // buffers
    uint8_t instr_buf;
    uint16_t addr_buf;
    uint8_t data_buf;
} cpu;

void set_flag(cpu * cpu, flag flag) {
    cpu->flags |= (1 << (int) flag);
}

void clear_flag(cpu * cpu, flag flag) {
    cpu->flags &= ~(1 << (int) flag);
}

int get_flag(cpu * cpu, flag flag) {
    return (cpu->flags >> (int) flag) & 1;
}

#endif