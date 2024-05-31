.section    .text
.global     main

main: li x13, 0
csrwi 0x340, 0;
li x5, 0 # override spike

csr_rwi:
csrwi 0x340, 4 # x0 is rd
li x11, 19
csrrw x10, 0x340, x11
li x30, 4
li x13, 101
bne x30, x10, fail

csr_rw:
li x1, 0x4d
csrrw x10, 0x340, x1
li x30, 19
li x13, 102
bne x30, x10, fail

csr_rs:
li x6, 0x12
csrrs x0, 0x340, x6 # store to CSR
csrrwi x10, 0x340, 21 # move CSR to x10, 0x15 to CSR for csrrc
li x30, 0x5f
li x13, 103
bne x30, x10, fail

csr_rc:
li x6, 0x7
csrrc x0, 0x340, x6
csrrwi x10, 0x340, 17
li x30, 0x10
li x13, 104
bne x30, x10, fail

csr_rsi:
csrrsi x0, 0x340, 2
csrrwi x10, 0x340, 5 # move CSR to x10
li x30, 19
li x13, 105
bne x30, x10, fail

csr_rci:
csrrci x0, 0x340, 3
csrrwi x10, 0x340, 31 # move CSR to x10
li x30, 4
li x13, 106
bne x30, x10, fail

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
