#include "defines.h"

.section    .start
.global     _start

_start:
    li      sp, 0x80001000
    jal     main

.global fail
fail:
    RVTEST_FAIL

.global pass
pass:
    RVTEST_PASS

