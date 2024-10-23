.section    .text
.global     main

#include "../common/asm_test.S"
#include "../common/csr.h"

main: li x13, 0
csrwi CSR_TOHOST, 0
li x5, 0 # override spike

li x4, 0 # loop var
li x5, LOOP_COUNT # loop end

loop: li x24, 0
addi x4, x4, 1

op_add:
li x11, 35
li x12, 65
add x26, x11, x12
OP_END(100)

op_fma16:
lw x11, dat1
lw x12, dat2
fma16 x26, x11, x12
OP_END(1719842)

op_fma8:
lw x11, dat1
lw x12, dat2
fma8 x26, x11, x12
OP_END(60)

op_fma4:
lw x11, dat3
lw x12, dat4
fma4 x26, x11, x12
OP_END(14)

li x24, 0x123456
bne x4, x5, loop
j pass

fail:
add x28, x0, x13 # store failed test id in x28
slli x28, x28, 1
or x28, x28, 1
csrw CSR_TOHOST, x28; # write failed test id
nop
nop
nop
nop
ecall

pass:
li x28, 0x1
csrw CSR_TOHOST, x28;
nop
nop
nop
nop
ecall

.data
dat1: .word 0x08070605
dat2: .word 0x01020304
dat3: .word 0x02020201
dat4: .word 0x01020302
