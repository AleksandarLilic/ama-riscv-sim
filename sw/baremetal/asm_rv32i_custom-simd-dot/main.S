.section .text
.global main

#include "../common/asm_test.S"
#include "../common/csr.h"

main:
    li x13, 0
    csrwi CSR_TOHOST, 0
    li x5, 0 # override spike

    li x4, 0 # loop var
    li x5, LOOPS # loop end

loop:
    li x24, 0
    addi x4, x4, 1

op_add:
    li x11, 35
    li x12, 65
    add x26, x11, x12
    OP_END(100)

op_dot16:
    lw x11, dat1
    lw x12, dat2
    dot16 x26, x11, x12
    OP_END(1719842)

op_dot8:
    lw x11, dat1
    lw x12, dat2
    dot8 x26, x11, x12
    OP_END(60)

op_dot4:
    lw x11, dat3
    lw x12, dat4
    dot4 x26, x11, x12
    OP_END(14)

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
dat4: .word 0x01020302
