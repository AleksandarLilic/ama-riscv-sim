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

# all zeros
op_dot16_zero:
    lw x11, dat_zero1
    lw x12, dat_zero2
    dot16 x26, x11, x12
    OP_END(0)

# single active low lane, small positive
op_dot16_single_lane_pos:
    lw x11, dat_single1
    lw x12, dat_single2
    dot16 x26, x11, x12
    OP_END(35)

# alternating bytes: dot16 large negative, dot8 zero
op_dot16_alt_mixed_neg:
    lw x11, dat_alt1
    lw x12, dat_alt2
    dot16 x26, x11, x12
    OP_END(-130560) # = 2 * (0xff00 * 0x00ff) as signed 16x16

# sign-heavy 0x80 vs 0x01 (all lanes negative * positive)
op_dot16_sign_large_negative:
    lw x11, dat_sign1
    lw x12, dat_sign2
    dot16 x26, x11, x12
    OP_END(-16776960) # 2 * (0x8080 * 0x0101) as signed 16x16

# all -1 lanes (0xffffffff)
op_dot16_all_minus1_pos:
    lw x11, dat_allm1_1
    lw x12, dat_allm1_2
    dot16 x26, x11, x12
    OP_END(2)

# medium mixed pattern
op_dot16_mid_mixed:
    lw x11, dat3
    lw x12, dat4
    dot16 x26, x11, x12
    OP_END(527622)

op_dot16_max_pos_pos:
    lw x11, dat_max_p16
    lw x12, dat_max_p16
    dot16 x26, x11, x12
    OP_END(2147352578) # 2 * (32767 * 32767)

op_dot16_max_neg_pos:
    lw x11, dat_max_n16
    lw x12, dat_max_p16
    dot16 x26, x11, x12
    OP_END(-2147352578) # 2 * (-32767 * 32767)

op_dot8:
    lw x11, dat1
    lw x12, dat2
    dot8 x26, x11, x12
    OP_END(60)

op_dot8_zero:
    lw x11, dat_zero1
    lw x12, dat_zero2
    dot8 x26, x11, x12
    OP_END(0)

op_dot8_single_lane_pos:
    lw x11, dat_single1
    lw x12, dat_single2
    dot8 x26, x11, x12
    OP_END(35)

op_dot8_alt_mixed_neg:
    lw x11, dat_alt1
    lw x12, dat_alt2
    dot8 x26, x11, x12
    OP_END(0)

op_dot8_sign_large_negative:
    lw x11, dat_sign1
    lw x12, dat_sign2
    dot8 x26, x11, x12
    OP_END(-512) # 4 * (-128 * 1)

op_dot8_mid_mixed:
    lw x11, dat3
    lw x12, dat4
    dot8 x26, x11, x12
    OP_END(14)

op_dot8_all_minus1_pos:
    lw x11, dat_allm1_1
    lw x12, dat_allm1_2
    dot8 x26, x11, x12
    OP_END(4)

op_dot8_max_pos_pos:
    lw x11, dat_max_p8
    lw x12, dat_max_p8
    dot8 x26, x11, x12
    OP_END(64516) # 4 * (127 * 127)

op_dot8_max_neg_pos:
    lw x11, dat_max_n8
    lw x12, dat_max_p8
    dot8 x26, x11, x12
    OP_END(-64516) # 4 * (-127 * 127)

#op_dot4:
#    lw x11, dat3
#    lw x12, dat4
#    dot4 x26, x11, x12
#    OP_END(14)

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

dat_zero1: .word 0x00000000
dat_zero2: .word 0x00000000

dat_single1: .word 0x00000005 # single low byte non-zero
dat_single2: .word 0x00000007

dat_alt1: .word 0xFF00FF00  # alternating bytes
dat_alt2: .word 0x00FF00FF

dat_sign1: .word 0x80808080 # all bytes 0x80 (signed -128)
dat_sign2: .word 0x01010101

dat_allm1_1: .word 0xFFFFFFFF # all bytes 0xFF (signed -1)
dat_allm1_2: .word 0xFFFFFFFF

dat_max_p16: .word 0x7FFF7FFF # all halfwords 0x7FFF (signed 32767)
dat_max_n16: .word 0x80018001 # all halfwords 0x8001 (signed -32767)

dat_max_p8: .word 0x7F7F7F7F # all bytes 0x7F (signed 127)
dat_max_n8: .word 0x81818181 # all bytes 0x81 (signed -127)
