
#ifndef I8080_OPCODES_H
#define I8080_OPCODES_H
                                /*    size       flags       details                           cycles  */
#define i8080_NOP 0x00          /*      1                    no operation                         4    */
#define i8080_LXI_B 0x01        /*      3                    B <- byte 3, C <- byte 2             10   */
#define i8080_STAX_B 0x02       /*      1                    [BC] <- A                            7    */
#define i8080_INX_B 0x03        /*      1                    BC <- BC + 1                         5    */
#define i8080_INR_B 0x04        /*      1       Z,S,P,AC     B <- B + 1                           5    */
#define i8080_DCR_B 0x05        /*      1       Z,S,P,AC     B <- B - 1                           5    */
#define i8080_MVI_B 0x06        /*      2                    B <- byte 2                          7    */
#define i8080_RLC 0x07          /*      1       CY           CY <- bit 7, A = A << 1              4    */
#define i8080_UD_NOP1 0x08      /*      1                    undocumented NOP                     4    */                                                                       

#define i8080_DAD_B 0x09        /*      1       CY           HL <- HL + BC                        10   */
#define i8080_LDAX_B 0x0a       /*      1                    A <- [BC]                            7    */
#define i8080_DCX_B 0x0b        /*      1                    BC <- BC - 1                         5    */
#define i8080_INR_C 0x0c        /*      1       Z,S,P,AC     C <- C + 1                           5    */
#define i8080_DCR_C 0x0d        /*      1       Z,S,P,AC     C <- C - 1                           5    */
#define i8080_MVI_C 0x0e        /*      2                    C <- byte 2                          7    */
#define i8080_RRC 0x0f          /*      1       CY           CY <- bit 0, A = A >> 1              4    */
#define i8080_UD_NOP2 0x10      /*      1                    undocumented NOP                     4    */                                                                        

#define i8080_LXI_D 0x11        /*      3                    D <- byte 3, E <- byte 2             10   */
#define i8080_STAX_D 0x12       /*      1                    [DE] <- A                            7    */
#define i8080_INX_D 0x13        /*      1                    DE <- DE + 1                         5    */
#define i8080_INR_D 0x14        /*      1       Z,S,P,AC     D <- D + 1                           5    */
#define i8080_DCR_D 0x15        /*      1       Z,S,P,AC     D <- D - 1                           5    */
#define i8080_MVI_D 0x16        /*      2                    D <- byte 2                          7    */
#define i8080_RAL 0x17          /*      1       CY           A = A << 1 through carry             4    */
#define i8080_UD_NOP3 0x18      /*      1                    undocumented NOP                     4    */                                                                        

#define i8080_DAD_D 0x19        /*      1       CY           HL <- HL + DE                        10   */
#define i8080_LDAX_D 0x1a       /*      1                    A <- [DE]                            7    */
#define i8080_DCX_D 0x1b        /*      1                    DE <- DE - 1                         5    */
#define i8080_INR_E 0x1c        /*      1       Z,S,P,AC     E <- E + 1                           5    */
#define i8080_DCR_E 0x1d        /*      1       Z,S,P,AC     E <- E - 1                           5    */
#define i8080_MVI_E 0x1e        /*      2                    E <- byte 2                          7    */
#define i8080_RAR 0x1f          /*      1       CY           A = A >> 1 through carry             4    */
#define i8080_UD_NOP4 0x20      /*      1                    undocumented NOP                     4    */                                                                      

#define i8080_LXI_H 0x21        /*      3                    H <- byte 3, L <- byte 2             10   */
#define i8080_SHLD 0x22         /*      3                    [adr] <- L, [adr + 1] <- H           16   */
#define i8080_INX_H 0x23        /*      1                    HL <- HL + 1                         5    */
#define i8080_INR_H 0x24        /*      1       Z,S,P,AC     H <- H + 1                           5    */
#define i8080_DCR_H 0x25        /*      1       Z,S,P,AC     H <- H - 1                           5    */
#define i8080_MVI_H 0x26        /*      2                    H <- byte 2                          7    */
#define i8080_DAA 0x27          /*      1       Z,S,P,AC     cvt A to bcd as {cy,A[7:4],A[3:0]}   4    */
#define i8080_UD_NOP5 0x28      /*      1                    undocumented NOP                     4    */                                                                       

#define i8080_DAD_H 0x29        /*      1       CY           HL <- HL + HL                        10   */
#define i8080_LHLD 0x2a         /*      3                    L <- [adr], H <- [adr + 1]           16   */
#define i8080_DCX_H 0x2b        /*      1                    HL <- HL - 1                         5    */
#define i8080_INR_L 0x2c        /*      1       Z,S,P,AC     L <- L + 1                           5    */
#define i8080_DCR_L 0x2d        /*      1       Z,S,P,AC     L <- L - 1                           5    */
#define i8080_MVI_L 0x2e        /*      2                    L <- byte 2                          7    */
#define i8080_CMA 0x2f          /*      1                    A <- !A                              4    */
#define i8080_UD_NOP6 0x30      /*      1                    undocumented NOP                     4    */                                                                     

#define i8080_LXI_SP 0x31       /*      3                    SP <- {byte 3, byte 2}               10   */
#define i8080_STA 0x32          /*      3                    [adr] <- A                           13   */
#define i8080_INX_SP 0x33       /*      1                    SP <- SP + 1                         5    */
#define i8080_INR_M 0x34        /*      1       Z,S,P,AC     [HL] <- [HL] + 1                     10   */
#define i8080_DCR_M 0x35        /*      1       Z,S,P,AC     [HL] <- [HL] - 1                     10   */
#define i8080_MVI_M 0x36        /*      2                    [HL] <- byte 2                       10   */
#define i8080_STC 0x37          /*      1       CY           CY = 1                               4    */
#define i8080_UD_NOP7 0x38      /*      1                    undocumented NOP                     4    */                                                                        

#define i8080_DAD_SP 0x39       /*      1       CY           HL <- HL + SP                        10   */
#define i8080_LDA 0x3a          /*      3                    A <- [adr]                           13   */
#define i8080_DCX_SP 0x3b       /*      1                    SP <- SP - 1                         5    */
#define i8080_INR_A 0x3c        /*      1       Z,S,P,AC     A <- A + 1                           5    */
#define i8080_DCR_A 0x3d        /*      1       Z,S,P,AC     A <- A - 1                           5    */
#define i8080_MVI_A 0x3e        /*      2                    A <- byte 2                          7    */
#define i8080_CMC 0x3f          /*      1       CY           CY = !CY                             4    */

/* --- 8-bit load/store/move ----- */
#define i8080_MOV_B_B 0x40      /*      1                    B <- B                               5    */
#define i8080_MOV_B_C 0x41      /*      1                    B <- C                               5    */
#define i8080_MOV_B_D 0x42      /*      1                    B <- D                               5    */
#define i8080_MOV_B_E 0x43      /*      1                    B <- E                               5    */
#define i8080_MOV_B_H 0x44      /*      1                    B <- H                               5    */
#define i8080_MOV_B_L 0x45      /*      1                    B <- L                               5    */
#define i8080_MOV_B_M 0x46      /*      1                    B <- [HL]                            7    */
#define i8080_MOV_B_A 0x47      /*      1                    B <- A                               5    */
#define i8080_MOV_C_B 0x48      /*      1                    C <- B                               5    */
#define i8080_MOV_C_C 0x49      /*      1                    C <- C                               5    */
#define i8080_MOV_C_D 0x4a      /*      1                    C <- D                               5    */
#define i8080_MOV_C_E 0x4b      /*      1                    C <- E                               5    */
#define i8080_MOV_C_H 0x4c      /*      1                    C <- H                               5    */
#define i8080_MOV_C_L 0x4d      /*      1                    C <- L                               5    */
#define i8080_MOV_C_M 0x4e      /*      1                    C <- [HL]                            7    */
#define i8080_MOV_C_A 0x4f      /*      1                    C <- A                               5    */
#define i8080_MOV_D_B 0x50      /*      1                    D <- B                               5    */
#define i8080_MOV_D_C 0x51      /*      1                    D <- C                               5    */
#define i8080_MOV_D_D 0x52      /*      1                    D <- D                               5    */
#define i8080_MOV_D_E 0x53      /*      1                    D <- E                               5    */
#define i8080_MOV_D_H 0x54      /*      1                    D <- H                               5    */
#define i8080_MOV_D_L 0x55      /*      1                    D <- L                               5    */
#define i8080_MOV_D_M 0x56      /*      1                    D <- [HL]                            7    */
#define i8080_MOV_D_A 0x57      /*      1                    D <- A                               5    */
#define i8080_MOV_E_B 0x58      /*      1                    E <- B                               5    */
#define i8080_MOV_E_C 0x59      /*      1                    E <- C                               5    */
#define i8080_MOV_E_D 0x5a      /*      1                    E <- D                               5    */
#define i8080_MOV_E_E 0x5b      /*      1                    E <- E                               5    */
#define i8080_MOV_E_H 0x5c      /*      1                    E <- H                               5    */
#define i8080_MOV_E_L 0x5d      /*      1                    E <- L                               5    */
#define i8080_MOV_E_M 0x5e      /*      1                    E <- [HL]                            7    */
#define i8080_MOV_E_A 0x5f      /*      1                    E <- A                               5    */
#define i8080_MOV_H_B 0x60      /*      1                    H <- B                               5    */
#define i8080_MOV_H_C 0x61      /*      1                    H <- C                               5    */
#define i8080_MOV_H_D 0x62      /*      1                    H <- D                               5    */
#define i8080_MOV_H_E 0x63      /*      1                    H <- E                               5    */
#define i8080_MOV_H_H 0x64      /*      1                    H <- H                               5    */
#define i8080_MOV_H_L 0x65      /*      1                    H <- L                               5    */
#define i8080_MOV_H_M 0x66      /*      1                    H <- [HL]                            7    */
#define i8080_MOV_H_A 0x67      /*      1                    H <- A                               5    */
#define i8080_MOV_L_B 0x68      /*      1                    L <- B                               5    */
#define i8080_MOV_L_C 0x69      /*      1                    L <- C                               5    */
#define i8080_MOV_L_D 0x6a      /*      1                    L <- D                               5    */
#define i8080_MOV_L_E 0x6b      /*      1                    L <- E                               5    */
#define i8080_MOV_L_H 0x6c      /*      1                    L <- H                               5    */
#define i8080_MOV_L_L 0x6d      /*      1                    L <- L                               5    */
#define i8080_MOV_L_M 0x6e      /*      1                    L <- [HL]                            7    */
#define i8080_MOV_L_A 0x6f      /*      1                    L <- A                               5    */
#define i8080_MOV_M_B 0x70      /*      1                    [HL] <- B                            7    */
#define i8080_MOV_M_C 0x71      /*      1                    [HL] <- C                            7    */
#define i8080_MOV_M_D 0x72      /*      1                    [HL] <- D                            7    */
#define i8080_MOV_M_E 0x73      /*      1                    [HL] <- E                            7    */
#define i8080_MOV_M_H 0x74      /*      1                    [HL] <- H                            7    */
#define i8080_MOV_M_L 0x75      /*      1                    [HL] <- L                            7    */
#define i8080_HLT 0x76          /*      1                    halt until next interrupt            7    */

#define i8080_MOV_M_A 0x77      /*      1                    [HL] <- A                            7    */
#define i8080_MOV_A_B 0x78      /*      1                    A <- B                               5    */
#define i8080_MOV_A_C 0x79      /*      1                    A <- C                               5    */
#define i8080_MOV_A_D 0x7a      /*      1                    A <- D                               5    */
#define i8080_MOV_A_E 0x7b      /*      1                    A <- E                               5    */
#define i8080_MOV_A_H 0x7c      /*      1                    A <- H                               5    */
#define i8080_MOV_A_L 0x7d      /*      1                    A <- L                               5    */
#define i8080_MOV_A_M 0x7e      /*      1                    A <- [HL]                            7    */
#define i8080_MOV_A_A 0x7f      /*      1                    A <- A                               5    */

/* ------- Arithmetic ---------- */
#define i8080_ADD_B 0x80        /*      1       Z,S,P,CY,AC  A <- A + B                           4    */
#define i8080_ADD_C 0x81        /*      1       Z,S,P,CY,AC  A <- A + C                           4    */
#define i8080_ADD_D 0x82        /*      1       Z,S,P,CY,AC  A <- A + D                           4    */
#define i8080_ADD_E 0x83        /*      1       Z,S,P,CY,AC  A <- A + E                           4    */
#define i8080_ADD_H 0x84        /*      1       Z,S,P,CY,AC  A <- A + H                           4    */
#define i8080_ADD_L 0x85        /*      1       Z,S,P,CY,AC  A <- A + L                           4    */
#define i8080_ADD_M 0x86        /*      1       Z,S,P,CY,AC  A <- A + [HL]                        7    */
#define i8080_ADD_A 0x87        /*      1       Z,S,P,CY,AC  A <- A + A                           4    */

#define i8080_ADC_B 0x88        /*      1       Z,S,P,CY,AC  A <- A + B + CY                      4    */
#define i8080_ADC_C 0x89        /*      1       Z,S,P,CY,AC  A <- A + C + CY                      4    */
#define i8080_ADC_D 0x8a        /*      1       Z,S,P,CY,AC  A <- A + D + CY                      4    */
#define i8080_ADC_E 0x8b        /*      1       Z,S,P,CY,AC  A <- A + E + CY                      4    */
#define i8080_ADC_H 0x8c        /*      1       Z,S,P,CY,AC  A <- A + H + CY                      4    */
#define i8080_ADC_L 0x8d        /*      1       Z,S,P,CY,AC  A <- A + L + CY                      4    */
#define i8080_ADC_M 0x8e        /*      1       Z,S,P,CY,AC  A <- A + [HL] + CY                   7    */
#define i8080_ADC_A 0x8f        /*      1       Z,S,P,CY,AC  A <- A + A + CY                      4    */

#define i8080_SUB_B 0x90        /*      1       Z,S,P,CY,AC  A <- A - B                           4    */
#define i8080_SUB_C 0x91        /*      1       Z,S,P,CY,AC  A <- A - C                           4    */
#define i8080_SUB_D 0x92        /*      1       Z,S,P,CY,AC  A <- A - D                           4    */
#define i8080_SUB_E 0x93        /*      1       Z,S,P,CY,AC  A <- A - E                           4    */
#define i8080_SUB_H 0x94        /*      1       Z,S,P,CY,AC  A <- A - H                           4    */
#define i8080_SUB_L 0x95        /*      1       Z,S,P,CY,AC  A <- A - L                           4    */
#define i8080_SUB_M 0x96        /*      1       Z,S,P,CY,AC  A <- A - [HL]                        7    */
#define i8080_SUB_A 0x97        /*      1       Z,S,P,CY,AC  A <- A - A                           4    */

#define i8080_SBB_B 0x98        /*      1       Z,S,P,CY,AC  A <- A - B - CY                      4    */
#define i8080_SBB_C 0x99        /*      1       Z,S,P,CY,AC  A <- A - C - CY                      4    */
#define i8080_SBB_D 0x9a        /*      1       Z,S,P,CY,AC  A <- A - D - CY                      4    */
#define i8080_SBB_E 0x9b        /*      1       Z,S,P,CY,AC  A <- A - E - CY                      4    */
#define i8080_SBB_H 0x9c        /*      1       Z,S,P,CY,AC  A <- A - H - CY                      4    */
#define i8080_SBB_L 0x9d        /*      1       Z,S,P,CY,AC  A <- A - L - CY                      4    */
#define i8080_SBB_M 0x9e        /*      1       Z,S,P,CY,AC  A <- A - [HL] - CY                   7    */
#define i8080_SBB_A 0x9f        /*      1       Z,S,P,CY,AC  A <- A - A - CY                      4    */

/* ------- Logical ---------- */
#define i8080_ANA_B 0xa0        /*      1       Z,S,P,CY,AC  A <- A & B                           4    */
#define i8080_ANA_C 0xa1        /*      1       Z,S,P,CY,AC  A <- A & C                           4    */
#define i8080_ANA_D 0xa2        /*      1       Z,S,P,CY,AC  A <- A & D                           4    */
#define i8080_ANA_E 0xa3        /*      1       Z,S,P,CY,AC  A <- A & E                           4    */
#define i8080_ANA_H 0xa4        /*      1       Z,S,P,CY,AC  A <- A & H                           4    */
#define i8080_ANA_L 0xa5        /*      1       Z,S,P,CY,AC  A <- A & L                           4    */
#define i8080_ANA_M 0xa6        /*      1       Z,S,P,CY,AC  A <- A & [HL]                        7    */
#define i8080_ANA_A 0xa7        /*      1       Z,S,P,CY,AC  A <- A & A                           4    */

#define i8080_XRA_B 0xa8        /*      1       Z,S,P,CY,AC  A <- A ^ B                           4    */
#define i8080_XRA_C 0xa9        /*      1       Z,S,P,CY,AC  A <- A ^ C                           4    */
#define i8080_XRA_D 0xaa        /*      1       Z,S,P,CY,AC  A <- A ^ D                           4    */
#define i8080_XRA_E 0xab        /*      1       Z,S,P,CY,AC  A <- A ^ E                           4    */
#define i8080_XRA_H 0xac        /*      1       Z,S,P,CY,AC  A <- A ^ H                           4    */
#define i8080_XRA_L 0xad        /*      1       Z,S,P,CY,AC  A <- A ^ L                           4    */
#define i8080_XRA_M 0xae        /*      1       Z,S,P,CY,AC  A <- A ^ [HL]                        7    */
#define i8080_XRA_A 0xaf        /*      1       Z,S,P,CY,AC  A <- A ^ A                           4    */

#define i8080_ORA_B 0xb0        /*      1       Z,S,P,CY,AC  A <- A | B                           4    */
#define i8080_ORA_C 0xb1        /*      1       Z,S,P,CY,AC  A <- A | C                           4    */
#define i8080_ORA_D 0xb2        /*      1       Z,S,P,CY,AC  A <- A | D                           4    */
#define i8080_ORA_E 0xb3        /*      1       Z,S,P,CY,AC  A <- A | E                           4    */
#define i8080_ORA_H 0xb4        /*      1       Z,S,P,CY,AC  A <- A | H                           4    */
#define i8080_ORA_L 0xb5        /*      1       Z,S,P,CY,AC  A <- A | L                           4    */
#define i8080_ORA_M 0xb6        /*      1       Z,S,P,CY,AC  A <- A | [HL]                        7    */
#define i8080_ORA_A 0xb7        /*      1       Z,S,P,CY,AC  A <- A | A                           4    */

#define i8080_CMP_B 0xb8        /*      1       Z,S,P,CY,AC  A - B                                4    */
#define i8080_CMP_C 0xb9        /*      1       Z,S,P,CY,AC  A - C                                4    */
#define i8080_CMP_D 0xba        /*      1       Z,S,P,CY,AC  A - D                                4    */
#define i8080_CMP_E 0xbb        /*      1       Z,S,P,CY,AC  A - E                                4    */
#define i8080_CMP_H 0xbc        /*      1       Z,S,P,CY,AC  A - H                                4    */
#define i8080_CMP_L 0xbd        /*      1       Z,S,P,CY,AC  A - L                                4    */
#define i8080_CMP_M 0xbe        /*      1       Z,S,P,CY,AC  A - [HL]                             7    */
#define i8080_CMP_A 0xbf        /*      1       Z,S,P,CY,AC  A - A                                4    */

/* --- Subroutines, stack, conditionals, I/O, misc ----- */
#define i8080_RNZ 0xc0          /*      1                    if NZ, perform RET                   11/5 */
#define i8080_POP_B 0xc1        /*      1                    BC = {[SP + 1], [SP]}, SP <- SP + 2  10   */
#define i8080_JNZ 0xc2          /*      3                    if NZ, PC <- adr                     10   */
#define i8080_JMP 0xc3          /*      3                    PC <- adr                            10   */
#define i8080_CNZ 0xc4          /*      3                    if NZ, CALL adr                      17/11*/
#define i8080_PUSH_B 0xc5       /*      1                    {[SP-1],[SP-2]}={B,C}, SP <- SP - 2  11   */
#define i8080_ADI 0xc6          /*      2       Z,S,P,CY,AC  A <- A + byte 2                      7    */
#define i8080_RST_0 0xc7        /*      1                    CALL interrupt handler at 0x0        11   */
#define i8080_RZ 0xc8           /*      1                    if Z, perform RET                    11/5 */
#define i8080_RET 0xc9          /*      1                    PC = {[SP + 1], [SP]}, SP <- SP + 2  10   */
#define i8080_JZ 0xca           /*      3                    if Z, perform JMP                    10   */
#define i8080_UD_JMP 0xcb       /*      3                    undocumented JMP                     10   */                                                                       
#define i8080_CZ 0xcc           /*      3                    if Z, CALL adr                       17/11*/
#define i8080_CALL 0xcd         /*      3                    PUSH PC, PC <- adr                   17   */
#define i8080_ACI 0xce          /*      2       Z,S,P,CY,AC  A <- A + byte 2 + CY                 7    */
#define i8080_RST_1 0xcf        /*      1                    CALL interrupt handler at 0x8        11   */

#define i8080_RNC 0xd0          /*      1                    if NCY, perform RET                  11/5 */
#define i8080_POP_D 0xd1        /*      1                    DE = {[SP + 1], [SP]}, SP <- SP + 2  10   */
#define i8080_JNC 0xd2          /*      3                    if NCY, perform JMP                  10   */
#define i8080_OUT 0xd3          /*      2                    Serial I/O out, byte 2 is 8-bit adr  10   */
#define i8080_CNC 0xd4          /*      3                    if NCY, CALL adr                     17/11*/
#define i8080_PUSH_D 0xd5       /*      1                    {[SP-1],[SP-2]}={D,E}, SP <- SP - 2  11   */
#define i8080_SUI 0xd6          /*      2       Z,S,P,CY,AC  A <- A - byte 2                      7    */
#define i8080_RST_2 0xd7        /*      1                    CALL interrupt handler at 0x10       11   */
#define i8080_RC 0xd8           /*      1                    if CY, perform RET                   11/5 */
#define i8080_UD_RET 0xd9       /*      1                    undocumented RET                     10   */                                                                    
#define i8080_JC 0xda           /*      3                    if CY, perform JMP                   10   */
#define i8080_IN 0xdb           /*      2                    Serial I/O in, byte 2 is 8-bit adr   10   */
#define i8080_CC 0xdc           /*      3                    if CY, CALL adr                      17/11*/
#define i8080_UD_CALL1 0xdd     /*      3                    undocumented CALL                    17   */                                                                    
#define i8080_SBI 0xde          /*      2       Z,S,P,CY,AC  A <- A - byte 2 - CY                 7    */
#define i8080_RST_3 0xdf        /*      1                    CALL interrupt handler at 0x18       11   */

#define i8080_RPO 0xe0          /*      1                    if P odd, perform RET                11/5 */
#define i8080_POP_H 0xe1        /*      1                    HL = {[SP + 1], [SP]}, SP <- SP + 2  10   */
#define i8080_JPO 0xe2          /*      3                    if P odd, perform JMP                10   */
#define i8080_XTHL 0xe3         /*      1                    HL <-> {[SP + 1], [SP]}              18   */
#define i8080_CPO 0xe4          /*      3                    if P odd, CALL adr                   17/11*/
#define i8080_PUSH_H 0xe5       /*      1                    {[SP-1],[SP-2]}={H,L}, SP <- SP - 2  11   */
#define i8080_ANI 0xe6          /*      2       Z,S,P,CY,AC  A <- A & byte 2                      7    */
#define i8080_RST_4 0xe7        /*      1                    CALL interrupt handler at 0x20       11   */
#define i8080_RPE 0xe8          /*      1                    if P even, perform RET               11/5 */
#define i8080_PCHL 0xe9         /*      1                    PC <- HL                             5    */
#define i8080_JPE 0xea          /*      3                    if P even, perform JMP               10   */
#define i8080_XCHG 0xeb         /*      1                    HL <-> DE                            4    */
#define i8080_CPE 0xec          /*      3                    if P even, CALL adr                  17/11*/
#define i8080_UD_CALL2 0xed     /*      3                    undocumented CALL                    17   */                                                                      
#define i8080_XRI 0xee          /*      2       Z,S,P,CY,AC  A <- A ^ byte 2                      7    */
#define i8080_RST_5 0xef        /*      1                    CALL interrupt handler at 0x28       11   */

#define i8080_RP 0xf0           /*      1                    if !S i.e. positive, perform RET     11/5 */
#define i8080_POP_PSW 0xf1      /*      1                    {A,flags}={[SP+1],[SP]},SP<-SP+2     10   */
#define i8080_JP 0xf2           /*      3                    if !S i.e. positive, perform JMP     10   */
#define i8080_DI 0xf3           /*      1                    disable interrupts,clr interrupt bit 4    */
#define i8080_CP 0xf4           /*      3                    if !S i.e. positive, CALL adr        17/11*/
#define i8080_PUSH_PSW 0xf5     /*      1                    {[SP-1],[SP-2]}={A,flags}, SP<-SP-2  11   */
#define i8080_ORI 0xf6          /*      2       Z,S,P,CY,AC  A <- A | byte 2                      7    */
#define i8080_RST_6 0xf7        /*      1                    CALL interrupt handler at 0x30       11   */
#define i8080_RM 0xf8           /*      1                    if S i.e. negative, perform RET      11/5 */
#define i8080_SPHL 0xf9         /*      1                    SP <- HL                             5    */
#define i8080_JM 0xfa           /*      3                    if S i.e. negative, perform JMP      10   */
#define i8080_EI 0xfb           /*      1                    enable interrupts, set interrupt bit 4    */
#define i8080_CM 0xfc           /*      3                    if S i.e. negative, CALL adr         17/11*/
#define i8080_UD_CALL3 0xfd     /*      3                    undocumented CALL                    17   */                                                                      
#define i8080_CPI 0xfe          /*      2       Z,S,P,CY,AC  A - byte 2                           7    */
#define i8080_RST_7 0xff        /*      1                    CALL interrupt handler at 0x38       11   */

#endif
