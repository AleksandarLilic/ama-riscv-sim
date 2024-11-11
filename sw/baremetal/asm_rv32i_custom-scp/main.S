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

op_scp_ld:
lla x11, dat3
scp.ld x0, x11

op_add2:
li x9, 0x00c0ffee
sw x9, 0(x11)
// scp is now dirty

op_scp_rel:
scp.rel x0, x11

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
