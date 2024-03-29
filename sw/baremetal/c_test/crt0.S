.section    .start
.global     _start

_start:
    li      sp, 0x80001000
    jal     main

.global fail
fail:
    slli x28, x28, 1
    or x28, x28, 1
    csrw 0x340, x28; # write failed test id
    nop
    nop
    nop
    nop
    ecall
