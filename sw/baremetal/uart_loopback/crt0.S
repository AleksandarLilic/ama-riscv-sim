#include "defines.h"

.section    .start
.global     _start

_start:
    li      sp, 0x80001000
    jal     main
