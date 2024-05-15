#include "defines.h"

.section    .start
.global     _start

#define RAM_START 0x80000000
#define RAM_SIZE  0x4000

_start:
    li      sp, RAM_START + RAM_SIZE
.option push
.option norelax
	la		gp,__global_pointer$
.option pop
    la      a0,__bss_start
    la      a1,__BSS_END__
clear_bss:
    bgeu    a0,a1,done_bss # while (!(a0 >= a1))
    sb      x0,0(a0)       # *a0 = 0;
    addi    a0,a0,1        # a0++;
    j       clear_bss
done_bss:   
    jal     main

.global fail
fail:
    RVTEST_FAIL

.global pass
pass:
    RVTEST_PASS

