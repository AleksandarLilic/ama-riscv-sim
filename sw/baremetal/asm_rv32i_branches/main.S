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

branch_type:
    li x13, 400

op_beq_3:
    li x10, -55
    li x11, -55
    addi x13, x13, 1
    // branches that are not taken
    bne x0, x0, op_bne_4
    bne x10, x11, op_bne_4
    blt x10, x11, op_bne_4
    // taken
    beq x10, x11, op_bne_4
    j fail

op_bne_4:
    li x10, -47
    li x11, 47
    addi x13, x13, 1
    beq x10, x11, op_blt_5
    blt x11, x10, op_blt_5
    beq x10, x11, op_blt_5
    blt x11, x10, op_blt_5
    // taken
    bne x10, x11, op_blt_5
    j fail

op_blt_5:
    li x10, -999
    li x11, 999
    addi x13, x13, 1
    beq x0, x11, op_bge_3_gaps
    beq x10, x11, op_bge_3_gaps
    bltu x10, x11, op_bge_3_gaps
    beq x10, x0, op_bge_3_gaps
    bne x0, x0, op_bge_3_gaps
    // taken
    blt x10, x11, op_bge_3_gaps
    j fail

op_bge_3_gaps:
    li x10, -37
    li x11, -38
    addi x13, x13, 1
    beq x10, x11, op_bltu_4_gaps
    nop
    beq x0, x11, op_bltu_4_gaps
    nop
    blt x0, x11, op_bltu_4_gaps
    nop
    // taken
    bge x10, x11, op_bltu_4_gaps
    j fail

op_bltu_4_gaps:
    li x10, 10565
    li x11, 10566
    addi x13, x13, 1
    bltu x11, x10, op_bgeu_5_gaps
    bltu x11, x0, op_bgeu_5_gaps
    nop
    beq x10, x11, op_bgeu_5_gaps
    beq x0, x11, op_bgeu_5_gaps
    nop
    // taken
    bltu x10, x11, op_bgeu_5_gaps
    j fail

op_bgeu_5_gaps:
    li x10, 1
    li x11, 1
    addi x13, x13, 1
    bne x10, x11, op_b_done
    beq x0, x11, op_b_done
    beq x10, x0, op_b_done
    nop
    bltu x10, x11, op_b_done
    bltu x10, x0, op_b_done
    bne x0, x0, op_b_done
    // taken
    bgeu x10, x11, op_b_done
    j fail

op_b_done:
    nop
    li x24, 0x123456
    bne x4, x5, loop
    j pass

fail:
    FAIL

pass:
    PASS

.data
.align 6 # align to cache line
dat1: .word 0x55551f12
dat2: .word 0xfff10357
dat3: .word 0xe0e142e2
dat4: .word 0x00000000
dat5: .word 0xf0e0d0c0
# fill the rest of the 64B cache line with known values
# 16-5 words left, each 4 bytes
buf1: .fill 3, 4, 0x0000DEAD
buf2: .fill 4, 4, 0x0000BEEF
buf3: .fill 4, 4, 0x0000C0DE
