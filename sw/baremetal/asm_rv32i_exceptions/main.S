.section .text
.global main

#include "../common/asm_test.S"
#include "../common/csr.h"

main:
    li x13, 0
    csrwi CSR_TOHOST, 0;
    li x5, 0 # override spike

op_add:
    li x11, 35
    li x12, 65
    add x26, x11, x12
    OP_END(100)

should_trap:

#ifdef TRAP_ECALL
    ecall
#endif

#ifdef TRAP_EBREAK
    ebreak
#endif

#ifdef TRAP_UNSUPPORTED
    li x11, -1
    la x12, 150
    .insn r 0x33, 0x3, 0x5, x26, x11, x12
#endif

#ifdef TRAP_LOAD_ACCESS_FAULT
    la x11, 0x3004 # memory actually starts at 0x10000
    lw x12, 0(x11)
#endif

#ifdef TRAP_STORE_ACCESS_FAULT
    la x11, 0x20008 # memory actually ends at 0x10000 + 32KB + peripherals
    sw x12, 0(x11)
#endif

#ifdef TRAP_LOAD_ADDRESS_MISALIGNED
    lla x11, dat1
    lhu x26, 1(x11)
#endif

#ifdef TRAP_STORE_ADDRESS_MISALIGNED
    lla x11, dat1
    sh x26, 1(x11)
#endif

#ifdef TRAP_INSTRUCTION_ADDRESS_MISALIGNED_JUMP
    la x11, to_nowhere_j
    addi x11, x11, 2 # misaligned address for rv32i, but not rv32ic
    jalr x0, x11, 0
to_nowhere_j:
    nop
    nop
    nop
#endif

#ifdef TRAP_INSTRUCTION_ADDRESS_MISALIGNED_BRANCH
    #beq x0, x0, 6 # misaligned address for rv32i, but not rv32ic
    .word 0x00000363
to_nowhere_b:
    nop
    nop
    nop
#endif

fail:
    FAIL

pass:
    PASS

.data
dat1: .word 0x55551f12
