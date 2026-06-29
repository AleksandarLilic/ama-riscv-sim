# rvmodel_macros.h

#ifndef _RVMODEL_MACROS_H
#define _RVMODEL_MACROS_H

#define RVMODEL_DATA_SECTION

#define STANDARD_SM_SUPPORTED

##### STARTUP #####
//#define RVMODEL_BOOT \

// Custom RVMODEL_BOOT_TO_MMODE overrides default RVTEST_BOOT_TO_MMODE
// if defined.  For most DUTs, the default should work and this macro
// should not be defined.  If no M-mode or CSRs are implemented, define this
// macro as blank to bypass the boot process.  If a nonconforming
// M-mode is implemented, define this macro to set up the necessary
// state in a fashion similar to RVTEST_BOOT_TO_MMODE.
//#define RVMODEL_BOOT_TO_MMODE

# Address to use for load/store fault tests that should cause an access fault on the DUT.
// This DUT does not generate access faults.  Comment out RVMODEL_ACCESS_FAULT_ADDRESS to prevent testing them.
//#define RVMODEL_ACCESS_FAULT_ADDRESS 0x00000000

##### TERMINATION #####

# Terminate test with a pass indication.
#define RVMODEL_HALT_PASS  \
  li x1, 1                ;\
  csrw 0x51e, x1          ;\
  loop_pass:              ;\
    j loop_pass           ;\

# Terminate test with a fail indication.
#define RVMODEL_HALT_FAIL \
  li x1, 13               ;\
  csrw 0x51e, x1          ;\
  loop_fail:              ;\
    j loop_fail           ;\

##### IO #####

#define RVMODEL_IO_INIT(_R1, _R2, _R3)

# UART0 base address; ctrl reg at +0 (bit 0 = TX_READY), tx_data reg at +8
#define RVMODEL_IO_WRITE_STR(_R1, _R2, _R3, _STR_PTR) \
1:                           ;                        \
  lbu  _R1, 0(_STR_PTR)      ; /* Load byte */        \
  beqz _R1, 3f               ; /* Exit if null */     \
2:                           ;                        \
  li   _R2, 0x10013000       ; /* UART0 base */       \
  4:                         ;                        \
    lw   _R3, 0(_R2)         ; /* ctrl reg */         \
    andi _R3, _R3, 0x1       ; /* TX_READY bit */     \
    beqz _R3, 4b             ; /* wait until ready */ \
  sw   _R1, 8(_R2)           ; /* tx_data */          \
  addi _STR_PTR, _STR_PTR, 1 ; /* Next char */        \
  j 1b                       ; /* Loop */             \
3:

##### Interrupt Latency #####

#define RVMODEL_INTERRUPT_LATENCY 10

##### Machine Timer #####
#define RVMODEL_MAX_CYCLES_PER_TIMER_TICK 1

#define RVMODEL_TIMER_INT_SOON_DELAY 100

//#define RVMODEL_MTIME_ADDRESS     0x02000010
//#define RVMODEL_MTIMECMP_ADDRESS  0x02000008

##### Machine Interrupts #####

#define RVMODEL_SET_MEXT_INT(_R1, _R2)
#define RVMODEL_CLR_MEXT_INT(_R1, _R2)
#define RVMODEL_SET_MSW_INT(_R1, _R2)
#define RVMODEL_CLR_MSW_INT(_R1, _R2)

##### Supervisor Interrupts #####

#define RVMODEL_SET_SEXT_INT(_R1, _R2)
#define RVMODEL_CLR_SEXT_INT(_R1, _R2)
#define RVMODEL_SET_SSW_INT(_R1, _R2)
#define RVMODEL_CLR_SSW_INT(_R1, _R2)

#endif // _RVMODEL_MACROS_H
