TARGET := basic
CFLAGS := -DLOOPS=1u
# no Zbb, instructions are used with .insn
MARCH := rv32i_zicsr
COMMON_OBJ_NAMES = crt0.o

all: $(TARGET).elf

include ../Makefile.inc
