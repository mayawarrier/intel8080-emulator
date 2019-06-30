/* 
 * File:   opcodes.h
 * Author: dhruvwarrier
 * 
 * All the opcodes in the INTEL 8080.
 * Roughly categorized by instruction type.
 * 
 * Cycle counts with _/_ indicate:
 * - left is the cycle count if the action is taken
 * - right is the cycle count if the action is not taken
 * - conditional RETs and CALLs take 6 extra cycles if
 * the action is taken
 *
 * ALT_s are undocumented opcodes that are alternatives
 * for existing ones.
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
#define ALT_NOP0 0x08

#define DAD_B 0x09        /*      1       CY           HL <- HL + BC                        10   */
#define LDAX_B 0x0a       /*      1                    A <- [BC]                            7    */
#define DCX_B 0x0b        /*      1                    BC <- BC - 1                         5    */
#define INR_C 0x0c        /*      1       Z,S,P,AC     C <- C + 1                           5    */
#define DCR_C 0x0d        /*      1       Z,S,P,AC     C <- C - 1                           5    */
#define MVI_C 0x0e        /*      2                    C <- byte 2                          7    */
#define RRC 0x0f          /*      1       CY           CY <- bit 0, A = A >> 1              4    */
#define ALT_NOP1 0x10

#define LXI_D 0x11        /*      3                    D <- byte 3, E <- byte 2             10   */
#define STAX_D 0x12       /*      1                    [DE] <- A                            7    */
#define INX_D 0x13        /*      1                    DE <- DE + 1                         5    */
#define INR_D 0x14        /*      1       Z,S,P,AC     D <- D + 1                           5    */
#define DCR_D 0x15        /*      1       Z,S,P,AC     D <- D - 1                           5    */
#define MVI_D 0x16        /*      2                    D <- byte 2                          7    */
#define RAL 0x17          /*      1       CY           A = A << 1 through carry             4    */
#define ALT_NOP2 0x18

#define DAD_D 0x19        /*      1       CY           HL <- HL + DE                        10   */
#define LDAX_D 0x1a       /*      1                    A <- [DE]                            7    */
#define DCX_D 0x1b        /*      1                    DE <- DE - 1                         5    */
#define INR_E 0x1c        /*      1       Z,S,P,AC     E <- E + 1                           5    */
#define DCR_E 0x1d        /*      1       Z,S,P,AC     E <- E - 1                           5    */
#define MVI_E 0x1e        /*      2                    E <- byte 2                          7    */
#define RAR 0x1f          /*      1       CY           A = A >> 1 through carry             4    */
#define ALT_NOP3 0x20

#define LXI_H 0x21        /*      3                    H <- byte 3, L <- byte 2             10   */
#define SHLD 0x22         /*      3                    [adr] <- L, [adr + 1] <- H           16   */
#define INX_H 0x23        /*      1                    HL <- HL + 1                         5    */
#define INR_H 0x24        /*      1       Z,S,P,AC     H <- H + 1                           5    */
#define DCR_H 0x25        /*      1       Z,S,P,AC     H <- H - 1                           5    */
#define MVI_H 0x26        /*      2                    H <- byte 2                          7    */
#define DAA 0x27          /*      1       Z,S,P,AC     A to BCD, add 6 to lower nibble      
                                                       if AC or > 9, add 6 to higher
                                                       nibble if CY or > 9                  4    */
#define ALT_NOP4 0x28

#define DAD_H 0x29        /*      1       CY           HL <- HL + HL                        10   */
#define LHLD 0x2a         /*      3                    L <- [adr], H <- [adr + 1]           16   */
#define DCX_H 0x2b        /*      1                    HL <- HL - 1                         5    */
#define INR_L 0x2c        /*      1       Z,S,P,AC     L <- L + 1                           5    */
#define DCR_L 0x2d        /*      1       Z,S,P,AC     L <- L - 1                           5    */
#define MVI_L 0x2e        /*      2                    L <- byte 2                          7    */
#define CMA 0x2f          /*      1                    A <- !A                              4    */
#define ALT_NOP5 0x30

#define LXI_SP 0x31       /*      3                    SP <- {byte 3, byte 2}               10   */
#define STA 0x32          /*      3                    [adr] <- A                           13   */
#define INX_SP 0x33       /*      1                    SP <- SP + 1                         5    */
#define INR_M 0x34        /*      1       Z,S,P,AC     [HL] <- [HL] + 1                     10    */
#define DCR_M 0x35        /*      1       Z,S,P,AC     [HL] <- [HL] - 1                     10    */
#define MVI_M 0x36        /*      2                    [HL] <- byte 2                       10    */
#define STC 0x37          /*      1       CY           CY = 1                               4    */
#define ALT_NOP6 0x38

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
#define ADD_B 0x80        /*      1       Z,S,P,CY,AC  A <- A + B                           4    */
#define ADD_C 0x81        /*      1       Z,S,P,CY,AC  A <- A + C                           4    */
#define ADD_D 0x82        /*      1       Z,S,P,CY,AC  A <- A + D                           4    */
#define ADD_E 0x83        /*      1       Z,S,P,CY,AC  A <- A + E                           4    */
#define ADD_H 0x84        /*      1       Z,S,P,CY,AC  A <- A + H                           4    */
#define ADD_L 0x85        /*      1       Z,S,P,CY,AC  A <- A + L                           4    */
#define ADD_M 0x86        /*      1       Z,S,P,CY,AC  A <- A + [HL]                        7    */
#define ADD_A 0x87        /*      1       Z,S,P,CY,AC  A <- A + A                           4    */

#define ADC_B 0x88        /*      1       Z,S,P,CY,AC  A <- A + B + CY                      4    */
#define ADC_C 0x89        /*      1       Z,S,P,CY,AC  A <- A + C + CY                      4    */
#define ADC_D 0x8a        /*      1       Z,S,P,CY,AC  A <- A + D + CY                      4    */
#define ADC_E 0x8b        /*      1       Z,S,P,CY,AC  A <- A + E + CY                      4    */
#define ADC_H 0x8c        /*      1       Z,S,P,CY,AC  A <- A + H + CY                      4    */
#define ADC_L 0x8d        /*      1       Z,S,P,CY,AC  A <- A + L + CY                      4    */
#define ADC_M 0x8e        /*      1       Z,S,P,CY,AC  A <- A + [HL] + CY                   7    */
#define ADC_A 0x8f        /*      1       Z,S,P,CY,AC  A <- A + A + CY                      4    */

#define SUB_B 0x90        /*      1       Z,S,P,CY,AC  A <- A - B                           4    */
#define SUB_C 0x91        /*      1       Z,S,P,CY,AC  A <- A - C                           4    */
#define SUB_D 0x92        /*      1       Z,S,P,CY,AC  A <- A - D                           4    */
#define SUB_E 0x93        /*      1       Z,S,P,CY,AC  A <- A - E                           4    */
#define SUB_H 0x94        /*      1       Z,S,P,CY,AC  A <- A - H                           4    */
#define SUB_L 0x95        /*      1       Z,S,P,CY,AC  A <- A - L                           4    */
#define SUB_M 0x96        /*      1       Z,S,P,CY,AC  A <- A - [HL]                        7    */
#define SUB_A 0x97        /*      1       Z,S,P,CY,AC  A <- A - A                           4    */

#define SBB_B 0x98        /*      1       Z,S,P,CY,AC  A <- A - B - CY                      4    */
#define SBB_C 0x99        /*      1       Z,S,P,CY,AC  A <- A - C - CY                      4    */
#define SBB_D 0x9a        /*      1       Z,S,P,CY,AC  A <- A - D - CY                      4    */
#define SBB_E 0x9b        /*      1       Z,S,P,CY,AC  A <- A - E - CY                      4    */
#define SBB_H 0x9c        /*      1       Z,S,P,CY,AC  A <- A - H - CY                      4    */
#define SBB_L 0x9d        /*      1       Z,S,P,CY,AC  A <- A - L - CY                      4    */
#define SBB_M 0x9e        /*      1       Z,S,P,CY,AC  A <- A - [HL] - CY                   7    */
#define SBB_A 0x9f        /*      1       Z,S,P,CY,AC  A <- A - A - CY                      4    */

// ------- Logical ----------
#define ANA_B 0xa0        /*      1       Z,S,P,CY,AC  A <- A & B                           4    */
#define ANA_C 0xa1        /*      1       Z,S,P,CY,AC  A <- A & C                           4    */
#define ANA_D 0xa2        /*      1       Z,S,P,CY,AC  A <- A & D                           4    */
#define ANA_E 0xa3        /*      1       Z,S,P,CY,AC  A <- A & E                           4    */
#define ANA_H 0xa4        /*      1       Z,S,P,CY,AC  A <- A & H                           4    */
#define ANA_L 0xa5        /*      1       Z,S,P,CY,AC  A <- A & L                           4    */
#define ANA_M 0xa6        /*      1       Z,S,P,CY,AC  A <- A & [HL]                        7    */
#define ANA_A 0xa7        /*      1       Z,S,P,CY,AC  A <- A & A                           4    */

#define XRA_B 0xa8        /*      1       Z,S,P,CY,AC  A <- A ^ B                           4    */
#define XRA_C 0xa9        /*      1       Z,S,P,CY,AC  A <- A ^ C                           4    */
#define XRA_D 0xaa        /*      1       Z,S,P,CY,AC  A <- A ^ D                           4    */
#define XRA_E 0xab        /*      1       Z,S,P,CY,AC  A <- A ^ E                           4    */
#define XRA_H 0xac        /*      1       Z,S,P,CY,AC  A <- A ^ H                           4    */
#define XRA_L 0xad        /*      1       Z,S,P,CY,AC  A <- A ^ L                           4    */
#define XRA_M 0xae        /*      1       Z,S,P,CY,AC  A <- A ^ [HL]                        7    */
#define XRA_A 0xaf        /*      1       Z,S,P,CY,AC  A <- A ^ A                           4    */

#define ORA_B 0xb0        /*      1       Z,S,P,CY,AC  A <- A | B                           4    */
#define ORA_C 0xb1        /*      1       Z,S,P,CY,AC  A <- A | C                           4    */
#define ORA_D 0xb2        /*      1       Z,S,P,CY,AC  A <- A | D                           4    */
#define ORA_E 0xb3        /*      1       Z,S,P,CY,AC  A <- A | E                           4    */
#define ORA_H 0xb4        /*      1       Z,S,P,CY,AC  A <- A | H                           4    */
#define ORA_L 0xb5        /*      1       Z,S,P,CY,AC  A <- A | L                           4    */
#define ORA_M 0xb6        /*      1       Z,S,P,CY,AC  A <- A | [HL]                        7    */
#define ORA_A 0xb7        /*      1       Z,S,P,CY,AC  A <- A | A                           4    */

#define CMP_B 0xb8        /*      1       Z,S,P,CY,AC  A - B                                4    */
#define CMP_C 0xb9        /*      1       Z,S,P,CY,AC  A - C                                4    */
#define CMP_D 0xba        /*      1       Z,S,P,CY,AC  A - D                                4    */
#define CMP_E 0xbb        /*      1       Z,S,P,CY,AC  A - E                                4    */
#define CMP_H 0xbc        /*      1       Z,S,P,CY,AC  A - H                                4    */
#define CMP_L 0xbd        /*      1       Z,S,P,CY,AC  A - L                                4    */
#define CMP_M 0xbe        /*      1       Z,S,P,CY,AC  A - [HL]                             7    */
#define CMP_A 0xbf        /*      1       Z,S,P,CY,AC  A - A                                4    */

// --- Subroutines, stack, conditionals, I/O -----
#define RNZ 0xc0          /*      1                    if NZ, perform RET                   11/5 */
#define POP_B 0xc1        /*      1                    BC = {[SP + 1], [SP]}, SP <- SP + 2  10   */
#define JNZ 0xc2          /*      3                    if NZ, PC <- adr                     10   */
#define JMP 0xc3          /*      3                    PC <- adr                            10   */
#define CNZ 0xc4          /*      3                    if NZ, CALL adr                      17/11*/
#define PUSH_B 0xc5       /*      1                    {[SP-1],[SP-2]}={B,C}, SP <- SP - 2  11   */
#define ADI 0xc6          /*      2       Z,S,P,CY,AC  A <- A + byte 2                      7    */
#define RST_0 0xc7        /*      1                    CALL $0 interrupt vector             11   */
#define RZ 0xc8           /*      1                    if Z, perform RET                    11/5 */
#define RET 0xc9          /*      1                    PC = {[SP + 1], [SP]}, SP <- SP + 2  10   */
#define JZ 0xca           /*      3                    if Z, perform JMP                    10   */
#define ALT_JMP0 0xcb

#define CZ 0xcc           /*      3                    if Z, CALL adr                       17/11*/
#define CALL 0xcd         /*      3                    PUSH PC, PC <- adr                   17   */
#define ACI 0xce          /*      2       Z,S,P,CY,AC  A <- A + byte 2 + CY                 7    */
#define RST_1 0xcf        /*      1                    CALL $8 interrupt vector             11   */
#define RNC 0xd0          /*      1                    if NCY, perform RET                  11/5 */
#define POP_D 0xd1        /*      1                    DE = {[SP + 1], [SP]}, SP <- SP + 2  10   */
#define JNC 0xd2          /*      3                    if NCY, perform JMP                  10   */
#define OUT 0xd3          /*      2                    Serial I/O out, byte 2 is 8-bit adr  10   */
#define CNC 0xd4          /*      3                    if NCY, CALL adr                     17/11*/
#define PUSH_D 0xd5       /*      1                    {[SP-1],[SP-2]}={D,E}, SP <- SP - 2  11   */
#define SUI 0xd6          /*      2       Z,S,P,CY,AC  A <- A - byte 2                      7    */
#define RST_2 0xd7        /*      1                    CALL $10 interrupt vector            11   */
#define RC 0xd8           /*      1                    if CY, perform RET                   11/5 */
#define ALT_RET0 0xd9

#define JC 0xda           /*      3                    if CY, perform JMP                   10   */
#define IN 0xdb           /*      2                    Serial I/O in, byte 2 is 8-bit adr   10   */
#define CC 0xdc           /*      3                    if CY, CALL adr                      17/11*/
#define ALT_CALL0 0xdd

#define SBI 0xde          /*      2       Z,S,P,CY,AC  A <- A - byte 2 - CY                 7    */
#define RST_3 0xdf        /*      1                    CALL $18 interrupt vector            11   */
#define RPO 0xe0          /*      1                    if P odd, perform RET                11/5 */
#define POP_H 0xe1        /*      1                    HL = {[SP + 1], [SP]}, SP <- SP + 2  10   */
#define JPO 0xe2          /*      3                    if P odd, perform JMP                10   */
#define XTHL 0xe3         /*      1                    HL <-> {[SP + 1], [SP]}              18   */
#define CPO 0xe4          /*      3                    if P odd, CALL adr                   17/11*/
#define PUSH_H 0xe5       /*      1                    {[SP-1],[SP-2]}={H,L}, SP <- SP - 2  11   */
#define ANI 0xe6          /*      2       Z,S,P,CY,AC  A <- A & byte 2                      7    */
#define RST_4 0xe7        /*      1                    CALL $20 interrupt vector            11   */
#define RPE 0xe8          /*      1                    if P even, perform RET               11/5 */
#define PCHL 0xe9         /*      1                    PC <- HL                             5    */
#define JPE 0xea          /*      3                    if P even, perform JMP               10   */
#define XCHG 0xeb         /*      1                    HL <-> DE                            5    */
#define CPE 0xec          /*      3                    if P even, CALL adr                  17/11*/
#define ALT_CALL1 0xed

#define XRI 0xee          /*      2       Z,S,P,CY,AC  A <- A ^ byte 2                      7    */
#define RST_5 0xef        /*      1                    CALL $28 interrupt vector            11   */
#define RP 0xf0           /*      1                    if !S i.e. positive, perform RET     11/5 */
#define POP_PSW 0xf1      /*      1                    {A,flags}={[SP+1],[SP]},SP<-SP+2     10   */
#define JP 0xf2           /*      3                    if !S i.e. positive, perform JMP     10   */
#define DI 0xf3           /*      1                    disable interrupts,clr interrupt bit 4    */
#define CP 0xf4           /*      3                    if !S i.e. positive, CALL adr        17/11*/
#define PUSH_PSW 0xf5     /*      1                    {[SP-1],[SP-2]}={A,flags}, SP<-SP-2  11   */
#define ORI 0xf6          /*      2       Z,S,P,CY,AC  A <- A | byte 2                      7    */
#define RST_6 0xf7        /*      1                    CALL $30 interrupt vector            11   */
#define RM 0xf8           /*      1                    if S i.e. negative, perform RET      11/5 */
#define SPHL 0xf9         /*      1                    SP <- HL                             5    */
#define JM 0xfa           /*      3                    if S i.e. negative, perform JMP      10   */
#define EI 0xfb           /*      1                    enable interrupts, set interrupt bit 4    */
#define CM 0xfc           /*      3                    if S i.e. negative, CALL adr         17/11*/
#define ALT_CALL2 0xfd

#define CPI 0xfe          /*      2       Z,S,P,CY,AC  A - byte 2                           7    */
#define RST_7 0xff        /*      1                    CALL $38 interrupt vector            11   */

// Disassembly table and cycles table sourced from:
// https://github.com/superzazu/8080/blob/master/i8080.c

// Pretty print disassembly, indexed by opcode
// e.g. DEBUG_DISASSEMBLY_TABLE[RST_7]
static const char * DEBUG_DISASSEMBLY_TABLE[] = {
    "nop", "lxi b,#", "stax b", "inx b", "inr b", "dcr b", "mvi b,#", "rlc",
    "undocumented", "dad b", "ldax b", "dcx b", "inr c", "dcr c", "mvi c,#", "rrc",
    "undocumented", "lxi d,#", "stax d", "inx d", "inr d", "dcr d", "mvi d,#", "ral",
    "undocumented", "dad d", "ldax d", "dcx d", "inr e", "dcr e", "mvi e,#", "rar",
    "undocumented", "lxi h,#", "shld", "inx h", "inr h", "dcr h", "mvi h,#", "daa",
    "undocumented", "dad h", "lhld", "dcx h", "inr l", "dcr l", "mvi l,#", "cma",
    "undocumented", "lxi sp,#","sta $", "inx sp", "inr M", "dcr M", "mvi M,#", "stc",
    "undocumented", "dad sp", "lda $", "dcx sp", "inr a", "dcr a", "mvi a,#", "cmc",
    "mov b,b", "mov b,c", "mov b,d", "mov b,e", "mov b,h", "mov b,l",
    "mov b,M", "mov b,a", "mov c,b", "mov c,c", "mov c,d", "mov c,e",
    "mov c,h", "mov c,l", "mov c,M", "mov c,a", "mov d,b", "mov d,c",
    "mov d,d", "mov d,e", "mov d,h", "mov d,l", "mov d,M", "mov d,a",
    "mov e,b", "mov e,c", "mov e,d", "mov e,e", "mov e,h", "mov e,l",
    "mov e,M", "mov e,a", "mov h,b", "mov h,c", "mov h,d", "mov h,e",
    "mov h,h", "mov h,l", "mov h,M", "mov h,a", "mov l,b", "mov l,c",
    "mov l,d", "mov l,e", "mov l,h", "mov l,l", "mov l,M", "mov l,a",
    "mov M,b", "mov M,c", "mov M,d", "mov M,e", "mov M,h", "mov M,l", "hlt",
    "mov M,a", "mov a,b", "mov a,c", "mov a,d", "mov a,e", "mov a,h",
    "mov a,l", "mov a,M", "mov a,a", "add b", "add c", "add d", "add e",
    "add h", "add l", "add M", "add a", "adc b", "adc c", "adc d", "adc e",
    "adc h", "adc l", "adc M", "adc a", "sub b", "sub c", "sub d", "sub e",
    "sub h", "sub l", "sub M", "sub a", "sbb b", "sbb c", "sbb d", "sbb e",
    "sbb h", "sbb l", "sbb M", "sbb a", "ana b", "ana c", "ana d", "ana e",
    "ana h", "ana l", "ana M", "ana a", "xra b", "xra c", "xra d", "xra e",
    "xra h", "xra l", "xra M", "xra a", "ora b", "ora c", "ora d", "ora e",
    "ora h", "ora l", "ora M", "ora a", "cmp b", "cmp c", "cmp d", "cmp e",
    "cmp h", "cmp l", "cmp M", "cmp a", "rnz", "pop b", "jnz $", "jmp $",
    "cnz $", "push b", "adi #", "rst 0", "rz", "ret", "jz $", "undocumented", "cz $",
    "call $", "aci #", "rst 1", "rnc", "pop d", "jnc $", "out p", "cnc $",
    "push d", "sui #", "rst 2", "rc", "undocumented", "jc $", "in p", "cc $", "undocumented",
    "sbi #", "rst 3", "rpo", "pop h", "jpo $", "xthl", "cpo $", "push h",
    "ani #", "rst 4", "rpe", "pchl", "jpe $", "xchg", "cpe $", "undocumented", "xri #",
    "rst 5", "rp", "pop psw", "jp $", "di", "cp $", "push psw","ori #",
    "rst 6", "rm", "sphl", "jm $", "ei", "cm $", "undocumented", "cpi #", "rst 7"
};

static const word_t OPCODES_CYCLES[] = {
//  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    4,  10, 7,  5,  5,  5,  7,  4,  4,  10, 7,  5,  5,  5,  7,  4,  // 0
    4,  10, 7,  5,  5,  5,  7,  4,  4,  10, 7,  5,  5,  5,  7,  4,  // 1
    4,  10, 16, 5,  5,  5,  7,  4,  4,  10, 16, 5,  5,  5,  7,  4,  // 2
    4,  10, 13, 5,  10, 10, 10, 4,  4,  10, 13, 5,  5,  5,  7,  4,  // 3
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 4
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 5
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 6
    7,  7,  7,  7,  7,  7,  7,  7,  5,  5,  5,  5,  5,  5,  7,  5,  // 7
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // 8
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // 9
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // A
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // B
    5,  10, 10, 10, 11, 11, 7,  11, 5,  10, 10, 10, 11, 17, 7,  11, // C
    5,  10, 10, 10, 11, 11, 7,  11, 5,  10, 10, 10, 11, 17, 7,  11, // D
    5,  10, 10, 18, 11, 11, 7,  11, 5,  5,  10, 4,  11, 17, 7,  11, // E
    5,  10, 10, 4,  11, 11, 7,  11, 5,  5,  10, 4,  11, 17, 7,  11  // F
};

static word_t get_cycles_action_taken(word_t opcode) {
    
    if (opcode > 0xff) {
        // invalid opcode
        return WORD_T_MAX;
    }
    
    // Adjust by 6 if action taken for
    // conditional RETs and CALLs
    switch(opcode) {
        case RNZ: case CNZ:
        case RZ: case CZ:
        case RNC: case CNC:
        case RC: case CC:
        case RPO: case CPO:
        case RPE: case CPE:
        case RP: case CP:
        case RM: case CM:
            return OPCODES_CYCLES[opcode] + 6;
            
        default:
            return OPCODES_CYCLES[opcode];
    }
}

static word_t get_cycles_action_not_taken(word_t opcode) {
    
    if (opcode > 0xff) {
        // invalid opcode
        return WORD_T_MAX;
    }
    
    return OPCODES_CYCLES[opcode];
}

#endif /* OPCODES_H */