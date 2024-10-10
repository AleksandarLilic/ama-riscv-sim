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

op_add_neg:
li x11, -35
li x12, -65
add x26, x11, x12
OP_END(-100)

op_sub:
li x11, 257
li x12, 56
sub x26, x11, x12
OP_END(201)

op_sll:
li x11, 0xF0
li x12, 3
sll x26, x11, x12
OP_END(0x780)

op_srl:
li x11, 0x1F
li x12, 2
srl x26, x11, x12
OP_END(0x7)

op_sra:
li x11, 0x83330000
li x12, 5
sra x26, x11, x12
OP_END(0xFC199800)

op_slt:
li x11, -7
li x12, 7
slt x26, x11, x12
OP_END(1)

op_sltu:
li x11, 775
li x12, 99
sltu x26, x11, x12
OP_END(0)

op_xor:
li x11, 0xF1
li x12, 0xF2
xor x26, x11, x12
OP_END(0x3)

op_or:
li x11, 0xAA
li x12, 0x05
or x26, x11, x12
OP_END(0xAF)

op_and:
li x11, 0x202
li x12, 0x304
and x26, x11, x12
OP_END(0x200)

op_add_to_x0:
li x11, 17
li x12, 4
add x0, x11, x12
li x30, 0
addi x13, x13, 1
bne x30, x0, fail

i_type: li x13, 100

op_addi:
li x11, 35
addi x26, x11, -36
OP_END(-1)

op_slti:
li x11, -18
slti x26, x11, -17
OP_END(1)

op_sltiu:
li x11, 683
sltiu x26, x11, 682
OP_END(0)

op_xori:
li x11, 0xFFFFAAAA
xori x26, x11, 0x555
OP_END(0xFFFFAFFF)

op_ori:
li x11, 0xFFFF00FF
ori x26, x11, 0x100
OP_END(0xFFFF01FF)

op_andi:
li x11, 0xA7A70202
andi x26, x11, 0x0FF
OP_END(0x00000002)

op_slli:
li x11, 0xFFFF0001
slli x26, x11, 31
OP_END(0x80000000)

op_srli:
li x11, 0xFF001100
srli x26, x11, 29
OP_END(7)

op_srai:
li x11, 0xF0000000
srai x26, x11, 13
OP_END(0xFFFF8000)

load_type: li x13, 200

op_lb1:
lla x11, dat1
lb x26, 0(x11)
OP_END(0x12)

op_lb2:
lla x11, dat5
lb x26, 0(x11)
OP_END(0xffffffc0)

op_lb3:
lla x11, dat5
lb x26, 1(x11)
OP_END(0xffffffd0)

op_lb4:
lla x11, dat5
lb x26, 2(x11)
OP_END(0xffffffe0)

op_lb5:
lla x11, dat5
lb x26, 3(x11)
OP_END(0xfffffff0)

op_lh:
lla x11, dat1
lh x26, 0(x11)
OP_END(0x1f12)

op_lh2:
lla x11, dat3
lh x26, 2(x11)
OP_END(0xffffe0e1)

op_lw:
lla x11, dat3
lw x26, 0(x11)
OP_END(0xe0e142e2)

op_lbu:
lla x11, dat2
lbu x26, 3(x11)
OP_END(0xff)

op_lhu:
lla x11, dat3
# lhu x26, 1(x11) # TBD: unaligned access
# li x30, 0xe142
lhu x26, 0(x11)
OP_END(0x42e2)

store_type: li x13, 300

op_sb:
li x9, 0x33
lla x11, dat4
sb x9, 3(x11)
lbu x26, 3(x11)
OP_END(0x33)

op_sh:
li x9, 0xa5a6
lla x11, dat4
sh x9, 0(x11)
lhu x26, 0(x11)
OP_END(0xa5a6)

op_sw:
li x9, 0x00c0ffee
lla x11, dat4
sw x9, 0(x11)
lw x26, 0(x11)
OP_END(0x00c0ffee)

branch_type: li x13, 400

op_beq:
li x10, -55
li x11, -55
addi x13, x13, 1
beq x10, x11, op_bne
j fail

op_bne:
li x10, -47
li x11, 47
addi x13, x13, 1
bne x10, x11, op_blt
j fail

op_blt:
li x10, -999
li x11, 999
addi x13, x13, 1
blt x10, x11, op_bge
j fail

op_bge:
li x10, -37
li x11, -38
addi x13, x13, 1
bge x10, x11, op_bltu
j fail

op_bltu:
li x10, 10565
li x11, 10566
addi x13, x13, 1
bltu x10, x11, op_bgeu
j fail

op_bgeu:
li x10, 1
li x11, 1
addi x13, x13, 1
bgeu x10, x11, op_b_done
j fail

op_b_done:
nop

jump_type: li x13, 500

op_jal:
addi x13, x13, 1
jal op_jalr # execute tested op, will store PC+4 in x1
j fail

op_jalr:
addi x13, x13, 1
jalr x1, x1, 16 # should jump to nop at jalr_done label, 16 from x1 from above
j fail

jalr_done:
nop

upper_type:  li x13, 600

op_lui:
li x20, 0xf # load expected result, part1
slli x20, x20, 12 # load expected result, part2
lui x10, 0xf
addi x13, x13, 1
bne x20, x10, fail

op_auipc:
auipc x20, 0x1F4 # x20 = PC + (0x1F4 << 12)
lui x23, 0xFFF
srli x23, x23, 12
and x22, x20, x23 # save lower 12 bits of x20
; # reverse auipc
srai x20, x20, 12
addi x20, x20, -0x1F4
slli x20, x20, 12
or x20, x20, x22 # reapply lower 12 bits
lla x21, op_auipc
addi x13, x13, 1
bne x20, x21, fail

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
