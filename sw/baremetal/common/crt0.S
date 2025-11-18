.section .start
.global _start

_start:
    li  x1, 0
    li  x2, 0
    li  x3, 0
    li  x4, 0
    li  x5, 0
    li  x6, 0
    li  x7, 0
    li  x8, 0
    li  x9, 0
    li  x10, 0
    li  x11, 0
    li  x12, 0
    li  x13, 0
    li  x14, 0
    li  x15, 0
    li  x16, 0
    li  x17, 0
    li  x18, 0
    li  x19, 0
    li  x20, 0
    li  x21, 0
    li  x22, 0
    li  x23, 0
    li  x24, 0
    li  x25, 0
    li  x26, 0
    li  x27, 0
    li  x28, 0
    li  x29, 0
    li  x30, 0
    li  x31, 0

    #ifndef BASIC_ASM_TEST
    #ifndef NO_TRAP
    # initialize trap vector
    la t0, trap_entry
    csrw mtvec, t0
    #endif
    #endif

    # initialize gp, sp, and clear bss
    .option push
    .option norelax
    la gp, __global_pointer$
    la sp, __stack_top
    la a0, __bss_start
    la a1, __BSS_END__
    .option pop

    # a0 = start, a1 = end
    sub a2, a1, a0 # a2 = len
    srli a3, a2, 2 # a3 = nw = len / 4
    andi a4, a2, 3 # a4 = nb = len % 4

    beqz a3, clear_bss_b_check # skip word loop if none
clear_bss_w_loop:
    sw x0, 0(a0)
    addi a0, a0, 4
    addi a3, a3, -1
    bnez a3, clear_bss_w_loop

clear_bss_b_check:
    beqz a4, call_main # skip byte loop if none
clear_bss_b_loop:
    sb x0, 0(a0)
    addi a0, a0, 1
    addi a4, a4, -1
    bnez a4, clear_bss_b_loop

call_main:
    #ifdef _USE_NEWLIB
    # optionally, initialize the C library if used
    call __libc_init_array
    #endif
    call main

    ecall # try to trap if main returns

# in case main returns and ecall doesn't trap
# (both of which should never happen)
# prevent the cpu from progressing further
forever:
    j forever

#ifndef BASIC_ASM_TEST
#ifndef NO_TRAP
trap_entry:
    // save state
    addi sp, sp, -272
    sw x1, 1*4(sp)
    sw x2, 2*4(sp)
    sw x3, 3*4(sp)
    sw x4, 4*4(sp)
    sw x5, 5*4(sp)
    sw x6, 6*4(sp)
    sw x7, 7*4(sp)
    sw x8, 8*4(sp)
    sw x9, 9*4(sp)
    sw x10, 10*4(sp)
    sw x11, 11*4(sp)
    sw x12, 12*4(sp)
    sw x13, 13*4(sp)
    sw x14, 14*4(sp)
    sw x15, 15*4(sp)
    sw x16, 16*4(sp)
    sw x17, 17*4(sp)
    sw x18, 18*4(sp)
    sw x19, 19*4(sp)
    sw x20, 20*4(sp)
    sw x21, 21*4(sp)
    sw x22, 22*4(sp)
    sw x23, 23*4(sp)
    sw x24, 24*4(sp)
    sw x25, 25*4(sp)
    sw x26, 26*4(sp)
    sw x27, 27*4(sp)
    sw x28, 28*4(sp)
    sw x29, 29*4(sp)
    sw x30, 30*4(sp)
    sw x31, 31*4(sp)

    // handle trap
    csrr a0, mcause
    csrr a1, mepc
    mv a2, sp
    jal trap_handler
    // workaround for debug, skip the trapped instruction
    // get mepc, inc by 4, and write back
    //csrr t0, mepc
    //addi t0, t0, 4
    //csrw mepc, t0

    // restore state
    lw x1, 1*4(sp)
    lw x2, 2*4(sp)
    lw x3, 3*4(sp)
    lw x4, 4*4(sp)
    lw x5, 5*4(sp)
    lw x6, 6*4(sp)
    lw x7, 7*4(sp)
    lw x8, 8*4(sp)
    lw x9, 9*4(sp)
    lw x10, 10*4(sp)
    lw x11, 11*4(sp)
    lw x12, 12*4(sp)
    lw x13, 13*4(sp)
    lw x14, 14*4(sp)
    lw x15, 15*4(sp)
    lw x16, 16*4(sp)
    lw x17, 17*4(sp)
    lw x18, 18*4(sp)
    lw x19, 19*4(sp)
    lw x20, 20*4(sp)
    lw x21, 21*4(sp)
    lw x22, 22*4(sp)
    lw x23, 23*4(sp)
    lw x24, 24*4(sp)
    lw x25, 25*4(sp)
    lw x26, 26*4(sp)
    lw x27, 27*4(sp)
    lw x28, 28*4(sp)
    lw x29, 29*4(sp)
    lw x30, 30*4(sp)
    lw x31, 31*4(sp)

    addi sp, sp, 272
    mret
#endif
#endif
