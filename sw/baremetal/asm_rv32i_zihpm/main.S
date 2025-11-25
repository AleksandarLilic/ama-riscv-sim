.section .text
.global main

#include "../common/asm_test.S"
#include "../common/csr.h"

main:
    li x13, 0
    csrwi CSR_TOHOST, 0;
    li x5, 0 # override spike

csr_rwi:
    csrwi CSR_MSCRATCH, 4 # x0 is rd
    li x11, 19
    csrrw x26, CSR_MSCRATCH, x11
    OP_END(4)

csr_rw:
    li x1, 0x4d
    csrrw x26, CSR_MSCRATCH, x1
    OP_END(19)

csr_rs:
    li x6, 0x12
    csrrs x0, CSR_MSCRATCH, x6 # store to CSR
    csrrwi x26, CSR_MSCRATCH, 21 # move CSR to x26, 0x15 to CSR for csrrc
    OP_END(0x5f)

csr_rc:
    li x6, 0x7
    csrrc x0, CSR_MSCRATCH, x6
    csrrwi x26, CSR_MSCRATCH, 17
    OP_END(0x10)

csr_rsi:
    csrrsi x0, CSR_MSCRATCH, 2
    csrrwi x26, CSR_MSCRATCH, 5 # move CSR to x26
    OP_END(19)

csr_rci:
    csrrci x0, CSR_MSCRATCH, 3
    csrrwi x26, CSR_MSCRATCH, 31 # move CSR to x26
    OP_END(4)

mhpmcounter3_read:
    li x26, 0x0
    csrw CSR_MHPMCOUNTER3, x26
    nop
    nop
    csrr x26, CSR_MHPMCOUNTER3
    TEST_INC
    bne x26, x0, fail # still zero, counter not set up

mhpmcounter3_write:
    li x26, 0x33
    csrw CSR_MHPMCOUNTER3, x26
    nop
    nop
    csrr x26, CSR_MHPMCOUNTER3
    TEST_INC
    li x27, 0x33
    bne x26, x27, fail # value written to counter, counter not enabled yet

mhpmevent3_write:
    li x26, (1 << 3) # event 3: fe bound
    csrw CSR_MHPMEVENT3, x26
    nop
    nop
    csrr x26, CSR_MHPMEVENT3
    TEST_INC
    li x27, (1 << 3)
    bne x26, x27, fail # value written to event

mhpmcounter8_read:
    li x26, 0x0
    csrw CSR_MHPMCOUNTER8, x26
    nop
    nop
    csrr x26, CSR_MHPMCOUNTER8
    TEST_INC
    bne x26, x0, fail # still zero, counter not set up

mhpmcounter8_write:
    li x26, 0x88
    csrw CSR_MHPMCOUNTER8, x26
    nop
    nop
    csrr x26, CSR_MHPMCOUNTER8
    TEST_INC
    li x27, 0x88
    bne x26, x27, fail # value written to counter, counter not enabled yet

mhpmevent8_write:
    li x26, (1 << 1) # event 1: be bound
    csrw CSR_MHPMEVENT8, x26
    nop
    nop
    csrr x26, CSR_MHPMEVENT8
    TEST_INC
    li x27, (1 << 1)
    bne x26, x27, fail # value written to event

    j pass

fail:
    FAIL

pass:
    PASS
