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

simd_add_sub:
    li x18, 0x44
    li x19, 0x33
    add16 x14, x18, x19
    add8 x14, x18, x19
    sub16 x14, x18, x19
    sub8 x14, x18, x19

simd_saturating_add_sub:
    qadd16 x14, x18, x19
    qadd16u x14, x18, x19
    qadd8 x14, x18, x19
    qadd8u x14, x18, x19
    qsub16 x14, x18, x19
    qsub16u x14, x18, x19
    qsub8 x14, x18, x19
    qsub8u x14, x18, x19

simd_widening_mul:
    wmul16 x14, x18, x19
    wmul16u x14, x18, x19
    wmul8 x14, x18, x19
    wmul8u x14, x18, x19

simd_dot:
    dot16 x14, x18, x19
    dot16u x14, x18, x19
    dot8 x14, x18, x19
    dot8u x14, x18, x19
    dot4 x14, x18, x19
    dot4u x14, x18, x19
    dot2 x14, x18, x19
    dot2u x14, x18, x19

simd_compare:
    min16 x14, x18, x19
    min16u x14, x18, x19
    min8 x14, x18, x19
    min8u x14, x18, x19
    max16 x14, x18, x19
    max16u x14, x18, x19
    max8 x14, x18, x19
    max8u x14, x18, x19

simd_shift:
    li x18, 0xAA55AA55
    slli16 x14, x18, 8
    slli8 x14, x18, 3
    srli16 x14, x18, 8
    srli8 x14, x18, 3
    srai16 x14, x18, 8
    srai8 x14, x18, 3

simd_widening:
    // v1
    widen16 x14, x18
    widen16u x14, x18
    widen8 x14, x18
    widen8u x14, x18
    widen4 x14, x18
    widen4u x14, x18
    widen2 x14, x18
    widen2u x14, x18

/*
unsupported_simd_widening:
    // v2
    widen16 x14, x18, 8
    widen16u x14, x18, 3
    widen8 x14, x18, 8
    widen8u x14, x18, 3
    widen4 x14, x18, 8
    widen4u x14, x18, 3
    widen2 x14, x18, 8
    widen2u x14, x18, 3
*/

simd_narrowing:
    li x18, 0x12345678
    li x19, 0x0000FFFF
    narrow32 x14, x18, x19
    narrow16 x14, x18, x19
    narrow8 x14, x18, x19
    narrow4 x14, x18, x19

simd_saturating_narrowing:
    qnarrow32 x14, x18, x19
    qnarrow32u x14, x18, x19
    qnarrow16 x14, x18, x19
    qnarrow16u x14, x18, x19
    qnarrow8 x14, x18, x19
    qnarrow8u x14, x18, x19
    qnarrow4 x14, x18, x19
    qnarrow4u x14, x18, x19

simd_swap_antidiagonal:
    swapad16 x14, x18, x19
    swapad8 x14, x18, x19
    swapad4 x14, x18, x19
    swapad2 x14, x18, x19

simd_duplicate_scalar_to_vector:
    li x18, 0x109
    dup16 x14, x18
    dup8 x14, x18
    dup4 x14, x18
    dup2 x14, x18

simd_insert_scalar:
    li x18, 0xa
    li x14, 0xffffffff
    vins16 x14, x18, 0
    li x14, 0xffffffff
    vins8 x14, x18, 2
    li x14, 0xffffffff
    vins4 x14, x18, 5
    li x18, 0x0
    li x14, 0xffffffff
    vins2 x14, x18, 13

simd_extract_scalar:
    li x18, 0x12345678
    vext16 x14, x18, 1
    vext16u x14, x18, 1
    vext8 x14, x18, 2
    vext8u x14, x18, 3
    vext4 x14, x18, 5
    vext4u x14, x18, 7
    li x18, 0xffffffff
    vext2 x14, x18, 11
    vext2u x14, x18, 15

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
