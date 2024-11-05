.section    .start
.global     _start

_start:
    li  x1,0
    li  x2,0
    li  x3,0
    li  x4,0
    li  x5,0
    li  x6,0
    li  x7,0
    li  x8,0
    li  x9,0
    li  x10,0
    li  x11,0
    li  x12,0
    li  x13,0
    li  x14,0
    li  x15,0
    li  x16,0
    li  x17,0
    li  x18,0
    li  x19,0
    li  x20,0
    li  x21,0
    li  x22,0
    li  x23,0
    li  x24,0
    li  x25,0
    li  x26,0
    li  x27,0
    li  x28,0
    li  x29,0
    li  x30,0
    li  x31,0
.option push
.option norelax
    la		gp,__global_pointer$
.option pop
    la      sp,__stack_top
    la      a0,__bss_start
    la      a1,__BSS_END__
clear_bss_w: # word at a time
    addi    a2,a0,4
    bgtu    a2,a1,clear_bss_b
    sw      x0,0(a0)
    addi    a0,a0,4
    j       clear_bss_w
clear_bss_b: # remaining bytes
    bgeu    a0,a1,done_bss # while (!(a0 >= a1))
    sb      x0,0(a0)       # *a0 = 0;
    addi    a0,a0,1        # a0++;
    j       clear_bss_b
done_bss:
#ifdef _USE_NEWLIB
    call    __libc_init_array
#endif
    call    main

.global test_end
test_end:
    nop
    nop
    nop
    nop
    ecall
