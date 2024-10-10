.section    .text
.global     main

#include "../common/asm_test.S"
#include "../common/csr.h"

main: li x13, 0
csrwi CSR_TOHOST, 0;
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

csr_rs_x0:
li x7, 0x36
li x6, 0xa0a0
csrrw x0, 0x340, x7
TEST_INC
csrrs x6, 0x340, x0
bne x6, x7, fail
csrrs x6, 0x340, x0
bne x6, x7, fail # x7 value should still be in 0x340 on x0 'writes' to CSR

misa_read:
csrr x26, CSR_MISA
TEST_INC
bne x26, x0, fail # misa is 0 when not implemented

misa_write:
li x26, 0x80000000
csrw CSR_MISA, x26
TEST_INC
csrr x26, CSR_MISA
bne x26, x0, fail # still 0 when writing to unimplemented misa

mhartid_read:
csrr x26, CSR_MHARTID
TEST_INC
bne x26, x0, fail # mhartid is 0, only one hart

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
