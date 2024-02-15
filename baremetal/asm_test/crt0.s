.section    .start
.global     _start

_start: j op_add

op_add:
li x11, 35
li x12, 65
add x10, x11, x12
li x30, 100
li x3, 1
bne x30, x10, fail

op_sub:
li x11, 257
li x12, 56
sub x10, x11, x12
li x30, 201
li x3, 2
bne x30, x10, fail

op_sll:
li x11, 0xF0
li x12, 3
sll x10, x11, x12
li x30, 0x780
li x3, 3
bne x30, x10, fail

op_srl:
li x11, 0x1F
li x12, 2
srl x10, x11, x12
li x30, 0x7
li x3, 4
bne x30, x10, fail

op_sra:
li x11, 0x83330000
li x12, 5
sra x10, x11, x12
li x30, 0xFC199800
li x3, 5
bne x30, x10, fail

op_slt:
li x11, -7
li x12, 7
slt x10, x11, x12
li x30, 1
li x3, 6
bne x30, x10, fail

op_sltu:
li x11, 775
li x12, 99
sltu x10, x11, x12
li x30, 0
li x3, 7
bne x30, x10, fail

op_xor:
li x11, 0xF1
li x12, 0xF2
xor x10, x11, x12
li x30, 0x3
li x3, 8
bne x30, x10, fail

op_or:
li x11, 0xAA
li x12, 0x05
or x10, x11, x12
li x30, 0xAF
li x3, 9
bne x30, x10, fail

op_and:
li x11, 0x202
li x12, 0x304
and x10, x11, x12
li x30, 0x200
li x3, 10
bne x30, x10, fail

load_type: j op_lb

op_lb:
lla x11, dat1
lb x10, 0(x11)
li x30, 0x12
li x3, 20
bne x30, x10, fail

op_lb2:
lla x11, dat2
lb x10, 2(x11)
li x30, 0xfffffff1
li x3, 21
bne x30, x10, fail

op_lh:
lla x11, dat1
lh x10, 0(x11)
li x30, 0x1f12
li x3, 22
bne x30, x10, fail

op_lh2:
lla x11, dat3
lh x10, 2(x11)
li x30, 0xffffe0e1
li x3, 23
bne x30, x10, fail

op_lw:
lla x11, dat3
lw x10, 0(x11)
li x30, 0xe0e142e2
li x3, 24
bne x30, x10, fail

op_lbu:
lla x11, dat2
lbu x10, 3(x11)
li x30, 0xff
li x3, 25
bne x30, x10, fail

op_lhu:
lla x11, dat3
lhu x10, 1(x11)
li x30, 0xe142
li x3, 26
bne x30, x10, fail

store_type: j op_sb

op_sb:
li x9, 0x33
lla x11, dat4
sb x9, 3(x11)
li x30, 0x33
li x3, 27
lbu x10, 3(x11)
bne x30, x10, fail

op_sh:
li x9, 0xa1a2
lla x11, dat4
sh x9, 1(x11)
li x30, 0xa1a2
li x3, 28
lhu x10, 1(x11)
bne x30, x10, fail

op_sw:
li x9, 0x00c0ffee
lla x11, dat4
sw x9, 0(x11)
li x30, 0x00c0ffee
li x3, 29
lw x10, 0(x11)
bne x30, x10, fail

done: j done

fail: 
add x25, x0, x3 # store failed test id in x25

failed: j failed

.data
dat4: .word 0x00000000
dat1: .word 0x55551f12
dat2: .word 0xfff10357
dat3: .word 0xe0e142e2
