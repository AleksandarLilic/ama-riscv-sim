.section    .text
.global     main

#include "../common/asm_test.S"

# only section testing compressed isa has rvc enabled, through macro TEST_C
.option norvc

main: li x13, 0
csrwi CSR_TOHOST, 0
li x5, 0 # override spike

li x4, 0 # loop var
li x5, LOOP_COUNT # loop end

loop: li x24, 0
addi x4, x4, 1

# compressed instruction are compared with the results of the expanded version
op_c_li:
TEST_C2(c.li, x26, 12)
addi x30, x30, 12
OP_END_C

op_c_lui:
TEST_C2(c.lui, x26, 0x1b)
lui x30, 0x1b
OP_END_C

op_c_addi:
TEST_C2(c.addi, x26, 7)
addi x30, x30, 7
OP_END_C

op_c_addi16sp:
li x2, 0
li x30, 0
TEST_C2(c.addi16sp, x2, 0x20)
addi x30, x30, 0x20
TEST_INC
bne x30, x2, fail

op_c_nop:
TEST_C0(c.nop)

op_c_srli:
li x14, 0xFF001100
mv x30, x14
TEST_C2(c.srli, x14, 4)
srli x30, x30, 4
OP_END_CS

op_c_srai:
li x14, 0xF0000000
mv x30, x14
TEST_C2(c.srai, x14, 5)
srai x30, x30, 5
OP_END_CS

op_c_andi:
li x14, 0xA7A70202
mv x30, x14
TEST_C2(c.andi, x14, 0x1A)
andi x30, x30, 0x1A
OP_END_CS

op_c_and:
li x14, 0x202
li x12, 0x304
mv x30, x14
TEST_C2(c.and, x14, x12)
and x30, x30, x12
OP_END_CS

op_c_or:
li x14, 0xFFFF00F3
li x12, 0x104
mv x30, x14
TEST_C2(c.or, x14, x12)
or x30, x30, x12
OP_END_CS

op_c_xor:
li x14, 0xFFFF00F3
li x12, 0xA50
mv x30, x14
TEST_C2(c.xor, x14, x12)
xor x30, x30, x12
OP_END_CS

op_c_sub:
li x14, 152
li x12, 33
mv x30, x14
TEST_C2(c.sub, x14, x12)
sub x30, x30, x12
OP_END_CS

op_c_addi4spn:
li x2, 0
li x30, 0
TEST_C3(c.addi4spn, x14, x2, 0x20)
addi x30, x30, 0x20
TEST_INC
bne x30, x14, fail

op_c_slli:
li x26, 0xFFFF0001
mv x30, x26
TEST_C2(c.slli, x26, 11)
slli x30, x30, 11
OP_END_C

op_c_mv:
li x29, 0xa500a500
mv x30, x29
TEST_C2(c.mv, x26, x29)
OP_END_C

op_c_add:
li x14, 0x202
li x26, 0x304
add x30, x14, x26
TEST_C2(c.add, x26, x14)
OP_END_C

# load and store instructions
op_c_lw:
lla x11, dat3
lw x30, 0(x11)
TEST_C2(c.lw, x14, 0(x11))
OP_END_CS

op_c_sw:
li x14, 0xa2b4c6d8 # value to store
lla x11, dat4 # address to store to
TEST_C2(c.sw, x14, 0(x11))
lw x26, 0(x11) # load value from memory
# repeat with expanded instruction
sw x14, 0(x11)
lw x30, 0(x11)
OP_END_CS

op_c_lwsp:
lla x2, dat3
TEST_C2(c.lwsp, x26, 0(x2))
lw x30, 0(x2)
OP_END_C

op_c_swsp:
li x25, 0xa2b4c6d8
lla x2, dat4
TEST_C2(c.swsp, x25, 0(x2))
lw x26, 0(x2)
# repeat with expanded instruction
sw x25, 0(x2)
lw x30, 0(x2)
OP_END_C

# flow change instructions
op_c_beqz:
li x10, 0
TEST_INC
TEST_C2(c.beqz, x10, op_c_bnez)
j fail

op_c_bnez:
li x10, 0xffff
TEST_INC
TEST_C2(c.bnez, x10, op_c_j)
j fail

op_c_j:
TEST_INC
TEST_C1(c.j, op_c_jal)
j fail

op_c_jal:
TEST_INC
TEST_C1(c.jal, op_c_jr) # stores PC+2 in x1
j fail # i.e. this PC is in x1

op_c_jr:
TEST_INC
addi x1, x1, 18 # x1 now has op_c_jalr address
TEST_C1(c.jr, x1)
j fail

op_c_jalr:
mv x10, x1
TEST_INC
addi x10, x10, 18 # x10 now has c_jal_done address
TEST_C1(c.jalr, x10) # stores PC+2 in x1
j fail

c_jal_done:
nop

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
dat1: .word 0x55551f12
dat2: .word 0xfff10357
dat3: .word 0xe0e142e2
dat4: .word 0x00000000
dat5: .word 0xf0e0d0c0
