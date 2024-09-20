.section    .text
.global     main

#include "../common/asm_test.S"

main: li x13, 0
csrwi 0x340, 0;
li x5, 0 # override spike

csr_rwi:
csrwi 0x340, 4 # x0 is rd
li x11, 19
csrrw x26, 0x340, x11
OP_END(4)

csr_rw:
li x1, 0x4d
csrrw x26, 0x340, x1
OP_END(19)

csr_rs:
li x6, 0x12
csrrs x0, 0x340, x6 # store to CSR
csrrwi x26, 0x340, 21 # move CSR to x26, 0x15 to CSR for csrrc
OP_END(0x5f)

csr_rc:
li x6, 0x7
csrrc x0, 0x340, x6
csrrwi x26, 0x340, 17
OP_END(0x10)

csr_rsi:
csrrsi x0, 0x340, 2
csrrwi x26, 0x340, 5 # move CSR to x26
OP_END(19)

csr_rci:
csrrci x0, 0x340, 3
csrrwi x26, 0x340, 31 # move CSR to x26
OP_END(4)

j pass

fail:
add x28, x0, x13 # store failed test id in x28
slli x28, x28, 1
or x28, x28, 1
csrw 0x340, x28; # write failed test id
nop
nop
nop
nop
ecall

pass:
li x28, 0x1
csrw 0x340, x28;
nop
nop
nop
nop
ecall
