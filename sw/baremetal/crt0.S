#include "defines.h"

.section    .start
.global     _start

#define RAM_START 0x80000000
#define RAM_SIZE  0x4000

_start:
    li      sp, RAM_START + RAM_SIZE
    jal     main

.global fail
fail:
    RVTEST_FAIL

.global pass
pass:
    RVTEST_PASS

