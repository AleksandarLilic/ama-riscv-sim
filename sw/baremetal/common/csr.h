#ifndef CSR_H
#define CSR_H

#define CSR_TOHOST 0x51E

// Machine-level CSR addresses
// Machine Information Registers
#define CSR_MVENDORID 0xF11 // MRO
#define CSR_MARCHID 0xF12 // MRO
#define CSR_MIMPID 0xF13 // MRO
#define CSR_MHARTID 0xF14 // MRO
#define CSR_MCONFIGPTR 0xF15 // MRO
// Machine Trap Setup
#define CSR_MSTATUS 0x300 // MRW
#define CSR_MISA 0x301 // MRW
#define CSR_MIE 0x304 // MWARL
//#define CSR_MTVEC 0x305 // MRW
// Machine Trap Handling
#define CSR_MSCRATCH 0x340 // MRW
//#define CSR_MEPC 0x341 // MRW
//#define CSR_MCAUSE 0x342 // MRW
//#define CSR_MTVAL 0x343 // MRW
#define CSR_MIP 0x344 // MRO
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

// MSTATUS bits
#define MSTATUS_MIE 0x8
#define MSTATUS_MPIE 0x80

// MIP bits - machine interrupt pending
#define MIP_MSIP (1 << 3) // software
#define MIP_MTIP (1 << 7) // timer
#define MIP_MEIP (1 << 11) // external
#define MIP_LCOFIP (1 << 13) // local counter overflow

// MIE bits - machine interrupt enable
#define MIE_MSIE MIP_MSIP
#define MIE_MTIE MIP_MTIP
#define MIE_MEIE MIP_MEIP
#define MIE_LCOFIE MIP_LCOFIP

// MCAUSE bits
// exception codes
#define MCAUSE_INST_ADDR_MISALIGNED 0X0
#define MCAUSE_INST_ACCESS_FAULT 0X1
#define MCAUSE_ILLEGAL_INST 0X2
#define MCAUSE_BREAKPOINT 0X3
#define MCAUSE_LOAD_ADDR_MISALIGNED 0X4
#define MCAUSE_LOAD_ACCESS_FAULT 0X5
#define MCAUSE_STORE_ADDR_MISALIGNED 0X6
#define MCAUSE_STORE_ACCESS_FAULT 0X7
#define MCAUSE_MACHINE_ECALL 0XB // 11
//#define MCAUSE_SOFTWARE_CHECK 0x12 // 18
#define MCAUSE_HARDWARE_ERROR 0x13 // 19

// interrupt codes
#define MCAUSE_MACHINE_SW_INT (uint32_t)((1 << 31) | MIP_MSIP)
#define MCAUSE_MACHINE_TIMER_INT (uint32_t)((1 << 31) | MIP_MTIP)
#define MCAUSE_MACHINE_EXT_INT (uint32_t)((1 << 31) | MIP_MEIP)
#define MCAUSE_MACHINE_LCOF_INT (uint32_t)((1 << 31) | MIP_LCOFIP)

#endif
