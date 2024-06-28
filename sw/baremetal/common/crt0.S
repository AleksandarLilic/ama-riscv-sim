.section    .start
.global     _start

_start:
    la      sp,__stack_top
.option push
.option norelax
    la		gp,__global_pointer$
.option pop
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
