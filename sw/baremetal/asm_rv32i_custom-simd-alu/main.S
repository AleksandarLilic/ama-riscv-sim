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

op_add16:
    lw x11, dat1
    lw x12, dat2
    add16 x26, x11, x12
    OP_END(151587081) # 0x09090909

op_add16n:
    lw x11, dat1n
    lw x12, dat3
    add16 x26, x11, x12
    OP_END(33358312) # 0x01fd01e8

op_add8:
    lw x11, dat2
    lw x12, dat3
    add8 x26, x11, x12
    OP_END(50595077) # 0x03040505

op_add8n:
    lw x11, dat1n
    lw x12, dat2
    add8 x26, x11, x12
    OP_END(16581355) # 0x00fd02eb

op_sub16:
    lw x11, dat1
    lw x12, dat2
    sub16 x26, x11, x12
    OP_END(117768961) # 0x07050301

op_sub16n:
    lw x11, dat1
    lw x12, dat1n
    sub16 x26, x11, x12
    OP_END(135005726) # 0x080C061E

op_sub8:
    lw x11, dat4
    lw x12, dat3
    sub8 x26, x11, x12
    OP_END(16779270) # 0x01000806

op_sub8n:
    lw x11, dat4
    lw x12, dat1n
    sub8 x26, x11, x12
    OP_END(67570464) # 0x04070b20

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
