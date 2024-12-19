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

op_add:
    li x11, 35
    li x12, 65
    add x26, x11, x12
    OP_END(100)

op_max:
    li x11, 35
    li x12, -65
    //max x26, x11, x12
    .insn r 0x33, 0x6, 0x5, x26, x11, x12
    OP_END(35)

op_maxu:
    li x11, -1
    la x12, 150
    //maxu x26, x11, x12
    .insn r 0x33, 0x7, 0x5, x26, x11, x12
    OP_END(-1) // since it's unsigned

op_min:
    li x11, 88
    li x12, -33
    //min x26, x11, x12
    .insn r 0x33, 0x4, 0x5, x26, x11, x12
    OP_END(-33)

op_minu:
    li x11, 88
    la x12, -99
    //minu x26, x11, x12
    .insn r 0x33, 0x5, 0x5, x26, x11, x12
    OP_END(88) // since it's unsigned

loop_end:
    li x24, 0x123456
    bne x4, x5, loop
    j pass

fail:
    FAIL

pass:
    PASS

.data
dat1: .word 0x08070605
dat2: .word 0x01020304
dat3: .word 0x02020201
dat4: .word 0x03020a07

dat1n: .word 0xFFFBFFE7
