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

op_widen16:
    lla x11, dat1
    lw x12, 0(x11)
    widen16 RD, x12
    mv x26, RD
    OP_END(0x00000605)
    mv x26, RDP
    OP_END(0x00000807)

op_widen16_fwd: // test operand forwarding in RTL
    lla x11, dat3 # 0x01020304
    lw x12, 0(x11)
    widen16 RD, x12
    mv x26, RDP
    OP_END(0x00000102)
    mv x26, RD
    OP_END(0x00000304)

op_widen16_msb_set:
    lla x11, dat2
    lw x12, 0(x11)
    widen16 RD, x12
    mv x26, RD
    OP_END(0xffffa701)
    mv x26, RDP
    OP_END(0x00000282)

op_widen16u:
    lla x11, dat1
    lw x12, 0(x11)
    widen16u RD, x12
    mv x26, RD
    OP_END(0x00000605)
    mv x26, RDP
    OP_END(0x00000807)

op_widen16u_msb_set:
    lla x11, dat2
    lw x12, 0(x11)
    widen16u RD, x12
    mv x26, RD
    OP_END(0x0000a701)
    mv x26, RDP
    OP_END(0x00000282)

op_widen8:
    lla x11, dat1
    lw x12, 0(x11)
    widen8 RD, x12
    mv x26, RD
    OP_END(0x00060005)
    mv x26, RDP
    OP_END(0x00080007)

op_widen8_fwd:
    lla x11, dat3 # 0x01020304
    lw x12, 0(x11)
    widen8 RD, x12
    nop
    mv x26, RDP
    OP_END(0x00010002)
    mv x26, RD
    OP_END(0x00030004)

op_widen8_msb_set:
    lla x11, dat2
    lw x12, 0(x11)
    widen8 RD, x12
    mv x26, RD
    OP_END(0xffa70001)
    mv x26, RDP
    OP_END(0x0002ff82)

op_widen8u:
    lla x11, dat1
    lw x12, 0(x11)
    widen8u RD, x12
    mv x26, RD
    OP_END(0x00060005)
    mv x26, RDP
    OP_END(0x00080007)

op_widen8u_msb_set:
    lla x11, dat2
    lw x12, 0(x11)
    widen8u RD, x12
    mv x26, RD
    OP_END(0x00a70001)
    mv x26, RDP
    OP_END(0x00020082)

op_widen4:
    lla x11, dat3
    lw x12, 0(x11)
    widen4 RD, x12
    mv x26, RD
    OP_END(0x00030004)
    mv x26, RDP
    OP_END(0x00010002)

op_widen4_msb_set:
    lla x11, dat1
    lw x12, 0(x11)
    widen4 RD, x12
    mv x26, RD
    OP_END(0x00060005)
    mv x26, RDP
    OP_END(0x00f80007)

op_widen4u:
    lla x11, dat3
    lw x12, 0(x11)
    widen4u RD, x12
    mv x26, RD
    OP_END(0x00030004)
    mv x26, RDP
    OP_END(0x00010002)

op_widen4u_msb_set:
    lla x11, dat1
    lw x12, 0(x11)
    widen4u RD, x12
    mv x26, RD
    OP_END(0x00060005)
    mv x26, RDP
    OP_END(0x00080007)

op_widen2:
    lla x11, dat4
    lw x12, 0(x11)
    widen2 RD, x12
    mv x26, RD
    OP_END(0x01100010)
    mv x26, RDP
    OP_END(0x00010000)

op_widen2_msb_set:
    lla x11, dat1
    lw x12, 0(x11)
    widen2 RD, x12
    mv x26, RD
    OP_END(0x001e0011)
    mv x26, RDP
    OP_END(0x00e0001f)

op_widen2u:
    lla x11, dat4
    lw x12, 0(x11)
    widen2u RD, x12
    mv x26, RD
    OP_END(0x01100010)
    mv x26, RDP
    OP_END(0x00010000)

op_widen2u_msb_set:
    lla x11, dat1
    lw x12, 0(x11)
    widen2u RD, x12
    mv x26, RD
    OP_END(0x00120011)
    mv x26, RDP
    OP_END(0x00200013)

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
dat2: .word 0x0282a701
dat3: .word 0x01020304
dat4: .word 0x01001404
