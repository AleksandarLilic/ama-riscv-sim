TARGET := basic
CFLAGS := -DLOOPS=1u -DBASIC_ASM_TEST
#CFLAGS := -DLOOPS=1000000u  -DBASIC_ASM_TEST
MARCH := rv32i_zicsr
COMMON_OBJ_NAMES = crt0.o

all: $(TARGET).elf

include ../Makefile.inc
