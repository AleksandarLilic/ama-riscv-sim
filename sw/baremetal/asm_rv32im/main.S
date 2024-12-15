.section .text
.global main

#include "../common/asm_test.S"
#include "../common/csr.h"

main:
    li x13, 0
    csrwi CSR_TOHOST, 0
    li x5, 0 # override spike

    li x4, 0 # loop var
    li x5, LOOP_COUNT # loop end

loop:
    li x24, 0
    addi x4, x4, 1

op_mul:
    li x11, 35
    li x12, 65
    mul x26, x11, x12
    OP_END(2275)

op_mul_of:
    li x11, 75000
    li x12, 65000
    # results fits in 33 bits: 4,875,000,000
    # returns [31:0] of the result
    mul x26, x11, x12
    OP_END(580032704)

op_mulh:
    li x11, 75000
    li x12, 65000
    # results fits in 33 bits: 4,875,000,000
    # returns [63:32] of the result
    mulh x26, x11, x12
    OP_END(1)

op_mulh_neg:
    li x11, -75000
    li x12, 65000
    # results fits in 33 bits: -4,875,000,000
    # returns [63:32] of the result
    mulh x26, x11, x12
    OP_END(-2)

op_mulhsu:
    li x11, -75000
    li x12, 65000
    # results fits in 33 bits: -4,875,000,000
    # returns [63:32] of the result
    mulhsu x26, x11, x12
    OP_END(-2)

op_mulhu:
    li x11, 75000
    li x12, 65000
    # results fits in 33 bits: 4,875,000,000
    # returns [63:32] of the result
    mulhu x26, x11, x12
    OP_END(1)

op_div:
    li x11, 75000
    li x12, 9375
    div x26, x11, x12
    OP_END(8)

op_div_neg:
    li x11, -75000
    li x12, 9375
    div x26, x11, x12
    OP_END(-8)

op_divu:
    li x11, 75000
    li x12, 9375
    divu x26, x11, x12
    OP_END(8)

op_rem:
    li x11, 75000
    li x12, 9500
    rem x26, x11, x12
    OP_END(8500)

op_rem_neg:
    li x11, -75000
    li x12, 9500
    rem x26, x11, x12
    OP_END(-8500)

op_remu:
    li x11, 75000
    li x12, 9500
    remu x26, x11, x12
    OP_END(8500)

    li x24, 0x123456
    bne x4, x5, loop
    j pass

fail:
    FAIL

pass:
    PASS

.data
dat1: .word 0x55551f12
dat2: .word 0xfff10357
dat3: .word 0xe0e142e2
dat4: .word 0x00000000
dat5: .word 0xf0e0d0c0
