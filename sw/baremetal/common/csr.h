#ifndef CSR_H
#define CSR_H

#define CSR_TOHOST 0x51E

// Machine-level CSR addresses
// Machine Information Registers
#define CSR_MVENDORID 0xF11 // MRO
#define CSR_MARCHID 0xF12 // MRO
#define CSR_MIMPID 0xF13 // MRO
#define CSR_MHARTID 0xF14 // MRO
// Machine Trap Setup
//#define CSR_MSTATUS 0x300 // MRW
#define CSR_MISA 0x301 // MRW
//#define CSR_MIE 0x304 // MRW
//#define CSR_MTVEC 0x305 // MRW
// Machine Trap Handling
#define CSR_MSCRATCH 0x340 // MRW
//#define CSR_MEPC 0x341 // MRW
//#define CSR_MCAUSE 0x342 // MRW
//#define CSR_MTVAL 0x343 // MRW
//#define CSR_MIP 0x344 // MRW
// Machine Counter/Timers
#define CSR_MCYCLE 0XB00 // MRW
#define CSR_MINSTRET 0XB02 // MRW
#define CSR_MCYCLEH 0XB80 // MRW
#define CSR_MINSTRETH 0XB82 // MRW

// Unprivileged CSR addresses
// Unprivileged Counter/Timers
#define CSR_CYCLE 0xC00 // URO
#define CSR_TIME 0xC01 // URO
#define CSR_INSTRET 0xC02 // URO
#define CSR_CYCLEH 0xC80 // URO
#define CSR_TIMEH 0xC81 // URO
#define CSR_INSTRETH 0xC82 // URO

#endif
