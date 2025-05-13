INC_PATHS += ../ # arch_test.h in parent

# imperas flags
CFLAGS := -DXLEN=32
#CFLAGS += -DRVMODEL_ASSERT

CFLAGS += -DBASIC_ASM_TEST
MARCH := rv32i_zicsr
COMMON_OBJ_NAMES = crt0.o
GCC_OPTS += -mno-relax # don't use gp for addressing, it's being used as gpr

all: $(TARGET).elf

include ../../Makefile.inc
