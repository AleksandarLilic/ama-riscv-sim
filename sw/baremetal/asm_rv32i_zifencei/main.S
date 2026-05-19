.section .text
.global main

#include "../common/asm_test.S"
#include "../common/csr.h"

// fences are NOPs in the core, just check they are treated as such

main:
    li x13, 0
    csrwi CSR_TOHOST, 0;
    li x5, 0 # override spike

op_add:
    li x11, 35
    li x12, 65
    add x26, x11, x12
    OP_END(100)

op_fence_i:
    fence.i
    add x26, x0, x11
    add x26, x1, x26
    nop
    nop

op_fence:
    fence
    add x26, x0, x12
    add x26, x2, x26
    nop
    nop

    j pass

fail:
    FAIL

pass:
    PASS
