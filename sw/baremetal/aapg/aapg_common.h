.macro test_entry_macro
.endm

.macro pre_program_macro
.endm
.macro post_program_macro
    csrwi 0x51e, 0x1
.endm

.macro pre_branch_macro
.endm

.macro post_branch_macro
.endm

# dont_use_regs = [5,6,10,11,12,13,30]
# and 29, used for branches
    .text
    .globl randomize
randomize:
    # load persistent state
    la   t0, saved_data
    lw   x4, 0(t0)

    # Weyl increment
    li x4, 0x1648a7dd

    # cheap avalanche: x ^= x >> 13
    srli t1, x4, 13
    xor  x4, x4, t1

    # store updated state
    sw   x4, 0(t0)

    # derive register values (chained, no immediates)
    mv   x7, x4
    srli t1, x7, 7
    xor  x7, x7, t1

    addi x8, x7, -1450
    srli t1, x8, 9
    xor  x8, x8, t1

    addi x9, x8, 331
    srli t1, x9, 11
    xor  x9, x9, t1

    addi x14, x9, 1471
    srli t1, x14, 13
    xor  x14, x14, t1

    addi x15, x14, 1286
    srli t1, x15, 15
    xor  x15, x15, t1

    addi x16, x15, 1128
    srli t1, x16, 17
    xor  x16, x16, t1

    addi x17, x16, -1742
    srli t1, x17, 19
    xor  x17, x17, t1

    addi x18, x17, -956
    srli t1, x18, 21
    xor  x18, x18, t1

    addi x19, x18, -1518
    srli t1, x19, 23
    xor  x19, x19, t1

    addi x20, x19, 29
    srli t1, x20, 25
    xor  x20, x20, t1

    addi x21, x20, 1116
    srli t1, x21, 27
    xor  x21, x21, t1

    addi x22, x21, -159
    srli t1, x22, 29
    xor  x22, x22, t1

    addi x23, x22, -66
    srli t1, x23, 31
    xor  x23, x23, t1

    addi x24, x23, 668
    srli t1, x24, 17
    xor  x24, x24, t1

    addi x25, x24, -446
    srli t1, x25, 13
    xor  x25, x25, t1

    addi x26, x25, 1230
    srli t1, x26, 11
    xor  x26, x26, t1

    addi x27, x26, -1141
    srli t1, x27, 7
    xor  x27, x27, t1

    addi x28, x27, -1616
    srli t1, x28, 9
    xor  x28, x28, t1

    addi x31, x29, -1884
    srli t1, x31, 21
    xor  x31, x31, t1

    ret

    .data
    .align 2
saved_data:
    .word 0x00000000
