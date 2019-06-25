/* 
 * File:   opcodes.h
 * Author: dhruvwarrier
 * 
 * All the opcodes in the INTEL 8080.
 * Roughly categorized by instruction type.
 *
 * Created on June 23, 2019, 11:02 PM
 */

#ifndef OPCODES_H
#define OPCODES_H
                          /*    size       flags       details                           cycles  */
#define NOP 0x00          /*      1                    no operation                         4    */
#define LXI_B 0x01        /*      3                    B <- byte 3, C <- byte 2             10   */
#define STAX_B 0x02       /*      1                    [BC] <- A                            7    */
#define INX_B 0x03        /*      1                    BC <- BC + 1                         5    */
#define INR_B 0x04        /*      1       Z,S,P,AC     B <- B + 1                           5    */
#define DCR_B 0x05        /*      1       Z,S,P,AC     B <- B - 1                           5    */
#define MVI_B 0x06        /*      2                    B <- byte 2                          7    */
#define RLC 0x07          /*      1       CY           CY <- bit 7, A = A << 1              4    */

#define DAD_B 0x09        /*      1       CY           HL <- HL + BC                        10   */
#define LDAX_B 0x0a       /*      1                    A <- [BC]                            7    */
#define DCX_B 0x0b        /*      1                    BC <- BC - 1                         5    */
#define INR_C 0x0c        /*      1       Z,S,P,AC     C <- C + 1                           5    */
#define DCR_C 0x0d        /*      1       Z,S,P,AC     C <- C - 1                           5    */
#define MVI_C 0x0e        /*      2                    C <- byte 2                          7    */
#define RRC 0x0f          /*      1       CY           CY <- bit 0, A = A >> 1              4    */

#define LXI_D 0x11        /*      3                    D <- byte 3, E <- byte 2             10   */
#define STAX_D 0x12       /*      1                    [DE] <- A                            7    */
#define INX_D 0x13        /*      1                    DE <- DE + 1                         5    */
#define INR_D 0x14        /*      1       Z,S,P,AC     D <- D + 1                           5    */
#define DCR_D 0x15        /*      1       Z,S,P,AC     D <- D - 1                           5    */
#define MVI_D 0x16        /*      2                    D <- byte 2                          7    */
#define RAL 0x17          /*      1       CY           A = A << 1 through carry             4    */

#define DAD_D 0x19        /*      1       CY           HL <- HL + DE                        10   */
#define LDAX_D 0x1a       /*      1                    A <- [DE]                            7    */
#define DCX_D 0x1b        /*      1                    DE <- DE - 1                         5    */
#define INR_E 0x1c        /*      1       Z,S,P,AC     E <- E + 1                           5    */
#define DCR_E 0x1d        /*      1       Z,S,P,AC     E <- E - 1                           5    */
#define MVI_E 0x1e        /*      2                    E <- byte 2                          7    */
#define RAR 0x1f          /*      1       CY           A = A >> 1 through carry             4    */

#define LXI_H 0x21        /*      3                    H <- byte 3, L <- byte 2             10   */
#define SHLD 0x22         /*      3                    [adr] <- L, [adr + 1] <- H           16   */
#define INX_H 0x23        /*      1                    HL <- HL + 1                         5    */
#define INR_H 0x24        /*      1       Z,S,P,AC     H <- H + 1                           5    */
#define DCR_H 0x25        /*      1       Z,S,P,AC     H <- H - 1                           5    */
#define MVI_H 0x26        /*      2                    H <- byte 2                          7    */
#define DAA 0x27          /*      1       Z,S,P,AC     A to BCD, add 6 to lower nibble      
                                                       if AC or > 9, add 6 to higher
                                                       nibble if CY or > 9                  4    */

#define DAD_H 0x29        /*      1       CY           HL <- HL + HL                        10   */
#define LHLD 0x2a         /*      3                    L <- [adr], H <- [adr + 1]           16   */
#define DCX_H 0x2b        /*      1                    HL <- HL - 1                         5    */
#define INR_L 0x2c        /*      1       Z,S,P,AC     L <- L + 1                           5    */
#define DCR_L 0x2d        /*      1       Z,S,P,AC     L <- L - 1                           5    */
#define MVI_L 0x2e        /*      2                    L <- byte 2                          7    */
#define CMA 0x2f          /*      1                    A <- !A                              4    */

#define LXI_SP 0x31       /*      3                    SP <- {byte 3, byte 2}               10   */
#define STA 0x32          /*      3                    [adr] <- A                           13   */
#define INX_SP 0x33       /*      1                    SP <- SP + 1                         5    */
#define INR_M 0x34        /*      1       Z,S,P,AC     [HL] <- [HL] + 1                     10    */
#define DCR_M 0x35        /*      1       Z,S,P,AC     [HL] <- [HL] - 1                     10    */
#define MVI_M 0x36        /*      2                    [HL] <- byte 2                       10    */
#define STC 0x37          /*      1       CY           CY = 1                               4    */

#define DAD_SP 0x39       /*      1       CY           HL <- HL + SP                        10   */
#define LDA 0x3a          /*      3                    A <- [adr]                           13   */
#define DCX_SP 0x3b       /*      1                    SP <- SP - 1                         5    */
#define INR_A 0x3c        /*      1       Z,S,P,AC     A <- A + 1                           5    */
#define DCR_A 0x3d        /*      1       Z,S,P,AC     A <- A - 1                           5    */
#define MVI_A 0x3e        /*      2                    A <- byte 2                          7    */
#define CMC 0x3f          /*      1       CY           CY = !CY                             4    */

// --- 8-bit load/store/move -----
#define MOV_B_B 0x40      /*      1                    B <- B                               5    */
#define MOV_B_C 0x41      /*      1                    B <- C                               5    */
#define MOV_B_D 0x42      /*      1                    B <- D                               5    */
#define MOV_B_E 0x43      /*      1                    B <- E                               5    */
#define MOV_B_H 0x44      /*      1                    B <- H                               5    */
#define MOV_B_L 0x45      /*      1                    B <- L                               5    */
#define MOV_B_M 0x46      /*      1                    B <- [HL]                            7    */
#define MOV_B_A 0x47      /*      1                    B <- A                               5    */
#define MOV_C_B 0x48      /*      1                    C <- B                               5    */
#define MOV_C_C 0x49      /*      1                    C <- C                               5    */
#define MOV_C_D 0x4a      /*      1                    C <- D                               5    */
#define MOV_C_E 0x4b      /*      1                    C <- E                               5    */
#define MOV_C_H 0x4c      /*      1                    C <- H                               5    */
#define MOV_C_L 0x4d      /*      1                    C <- L                               5    */
#define MOV_C_M 0x4e      /*      1                    C <- [HL]                            7    */
#define MOV_C_A 0x4f      /*      1                    C <- A                               5    */
#define MOV_D_B 0x50      /*      1                    D <- B                               5    */
#define MOV_D_C 0x51      /*      1                    D <- C                               5    */
#define MOV_D_D 0x52      /*      1                    D <- D                               5    */
#define MOV_D_E 0x53      /*      1                    D <- E                               5    */
#define MOV_D_H 0x54      /*      1                    D <- H                               5    */
#define MOV_D_L 0x55      /*      1                    D <- L                               5    */
#define MOV_D_M 0x56      /*      1                    D <- [HL]                            7    */
#define MOV_D_A 0x57      /*      1                    D <- A                               5    */
#define MOV_E_B 0x58      /*      1                    E <- B                               5    */
#define MOV_E_C 0x59      /*      1                    E <- C                               5    */
#define MOV_E_D 0x5a      /*      1                    E <- D                               5    */
#define MOV_E_E 0x5b      /*      1                    E <- E                               5    */
#define MOV_E_H 0x5c      /*      1                    E <- H                               5    */
#define MOV_E_L 0x5d      /*      1                    E <- L                               5    */
#define MOV_E_M 0x5e      /*      1                    E <- [HL]                            7    */
#define MOV_E_A 0x5f      /*      1                    E <- A                               5    */
#define MOV_H_B 0x60      /*      1                    H <- B                               5    */
#define MOV_H_C 0x61      /*      1                    H <- C                               5    */
#define MOV_H_D 0x62      /*      1                    H <- D                               5    */
#define MOV_H_E 0x63      /*      1                    H <- E                               5    */
#define MOV_H_H 0x64      /*      1                    H <- H                               5    */
#define MOV_H_L 0x65      /*      1                    H <- L                               5    */
#define MOV_H_M 0x66      /*      1                    H <- [HL]                            7    */
#define MOV_H_A 0x67      /*      1                    H <- A                               5    */
#define MOV_L_B 0x68      /*      1                    L <- B                               5    */
#define MOV_L_C 0x69      /*      1                    L <- C                               5    */
#define MOV_L_D 0x6a      /*      1                    L <- D                               5    */
#define MOV_L_E 0x6b      /*      1                    L <- E                               5    */
#define MOV_L_H 0x6c      /*      1                    L <- H                               5    */
#define MOV_L_L 0x6d      /*      1                    L <- L                               5    */
#define MOV_L_M 0x6e      /*      1                    L <- [HL]                            7    */
#define MOV_L_A 0x6f      /*      1                    L <- A                               5    */
#define MOV_M_B 0x70      /*      1                    [HL] <- B                            7    */
#define MOV_M_C 0x71      /*      1                    [HL] <- B                            7    */
#define MOV_M_D 0x72      /*      1                    [HL] <- B                            7    */
#define MOV_M_E 0x73      /*      1                    [HL] <- B                            7    */
#define MOV_M_H 0x74      /*      1                    [HL] <- B                            7    */
#define MOV_M_L 0x75      /*      1                    [HL] <- B                            7    */
#define HLT 0x76          /*      1                    halt, S0, S1 = 0, interrupt
                                                       brings it out of this state          7    */
#define MOV_M_A 0x77      /*      1                    [HL] <- B                            7    */
#define MOV_A_B 0x78      /*      1                    A <- B                               5    */
#define MOV_A_C 0x79      /*      1                    A <- C                               5    */
#define MOV_A_D 0x7a      /*      1                    A <- D                               5    */
#define MOV_A_E 0x7b      /*      1                    A <- E                               5    */
#define MOV_A_H 0x7c      /*      1                    A <- H                               5    */
#define MOV_A_L 0x7d      /*      1                    A <- L                               5    */
#define MOV_A_M 0x7e      /*      1                    A <- [HL]                            7    */
#define MOV_A_A 0x7f      /*      1                    A <- A                               5    */

// ------- Arithmetic ----------
#define ADD_B 0x80
#define ADD_C 0x81
#define ADD_D 0x82
#define ADD_E 0x83
#define ADD_H 0x84
#define ADD_L 0x85
#define ADD_M 0x86
#define ADD_A 0x87

#define ADC_B 0x88
#define ADC_C 0x89
#define ADC_D 0x8a
#define ADC_E 0x8b
#define ADC_H 0x8c
#define ADC_L 0x8d
#define ADC_M 0x8e
#define ADC_A 0x8f

#define SUB_B 0x90
#define SUB_C 0x91
#define SUB_D 0x92
#define SUB_E 0x93
#define SUB_H 0x94
#define SUB_L 0x95
#define SUB_M 0x96
#define SUB_A 0x97

#define SBB_B 0x98
#define SBB_C 0x99
#define SBB_D 0x9a
#define SBB_E 0x9b
#define SBB_H 0x9c
#define SBB_L 0x9d
#define SBB_M 0x9e
#define SBB_A 0x9f

// ------- Logical ----------
#define ANA_B 0xa0
#define ANA_C 0xa1
#define ANA_D 0xa2
#define ANA_E 0xa3
#define ANA_H 0xa4
#define ANA_L 0xa5
#define ANA_M 0xa6
#define ANA_A 0xa7

#define XRA_B 0xa8
#define XRA_C 0xa9
#define XRA_D 0xaa
#define XRA_E 0xab
#define XRA_H 0xac
#define XRA_L 0xad
#define XRA_M 0xae
#define XRA_A 0xaf

#define ORA_B 0xb0
#define ORA_C 0xb1
#define ORA_D 0xb2
#define ORA_E 0xb3
#define ORA_H 0xb4
#define ORA_L 0xb5
#define ORA_M 0xb6
#define ORA_A 0xb7

#define CMP_B 0xb8
#define CMP_C 0xb9
#define CMP_D 0xba
#define CMP_E 0xbb
#define CMP_H 0xbc
#define CMP_L 0xbd
#define CMP_M 0xbe
#define CMP_A 0xbf

// --- Subroutines, stack, conditionals, I/O -----
#define RNZ 0xc0
#define POP_B 0xc1
#define JNZ 0xc2
#define JMP 0xc3
#define CNZ 0xc4
#define PUSH_B 0xc5
#define ADI 0xc6
#define RST_0 0xc7
#define RZ 0xc8
#define RET 0xc9
#define JZ 0xca

#define CZ 0xcc
#define CALL 0xcd
#define ACI 0xce
#define RST_1 0xcf
#define RNC 0xd0
#define POP_D 0xd1
#define JNC 0xd2
#define OUT 0xd3
#define CNC 0xd4
#define PUSH_D 0xd5
#define SUI 0xd6
#define RST_2 0xd7
#define RC 0xd8

#define JC 0xda
#define IN 0xdb
#define CC 0xdc

#define SBI 0xde
#define RST_3 0xdf
#define RPO 0xe0
#define POP_H 0xe1
#define JPO 0xe2
#define XTHL 0xe3
#define CPO 0xe4
#define PUSH_H 0xe5
#define ANI 0xe6
#define RST_4 0xe7
#define RPE 0xe8
#define PCHL 0xe9
#define JPE 0xea
#define XCHG 0xeb
#define CPE 0xec

#define XRI 0xee
#define RST_5 0xef
#define RP 0xf0
#define POP_PSW 0xf1
#define JP 0xf2
#define DI 0xf3
#define CP 0xf4
#define PUSH_PSW 0xf5
#define ORI 0xf6
#define RST_6 0xf7
#define RM 0xf8
#define SPHL 0xf9
#define JM 0xfa
#define EI 0xfb
#define CM 0xfc

#define CPI 0xfe
#define RST_7 0xff

#endif /* OPCODES_H */