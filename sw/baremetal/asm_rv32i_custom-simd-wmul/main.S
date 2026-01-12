.section .text
.global main

#include "../common/asm_test.S"
#include "../common/csr.h"

#define RD t1 // x6
#define RDP t2 // x7, the paired register

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

op_wmul16:
    li x11, -55
    sll x11, x11, 16
    ori x11, x11, 10
    li x12, 2
    sll x12, x12, 16
    ori x12, x12, 3
    wmul16 RD, x11, x12
    mv x26, RD
    OP_END(30)
    mv x26, RDP
    OP_END(-110)

op_wmul16u:
    li x11, 32768
    sll x11, x11, 16
    ori x11, x11, 11
    li x12, 2
    sll x12, x12, 16
    ori x12, x12, 7
    wmul16u RD, x11, x12
    mv x26, RD
    OP_END(77)
    mv x26, RDP
    OP_END(65536)

op_wmul8:
    li x11, -55
    sll x11, x11, 16
    ori x11, x11, 10
    li x12, 2
    sll x12, x12, 16
    ori x12, x12, 3
    wmul8 RD, x11, x12
    mv x26, RD
    OP_END(30)
    mv x26, RDP
    OP_END(0xff92) # 0x0000ffff & -110

op_wmul8u:
    li x11, 32768
    sll x11, x11, 16
    ori x11, x11, 11
    li x12, 1
    sll x12, x12, 24
    ori x12, x12, 7
    wmul8u RD, x11, x12
    mv x26, RD
    OP_END(77)
    mv x26, RDP
    OP_END(0x800000) # high half = 128 (0x80), low half = 0

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
