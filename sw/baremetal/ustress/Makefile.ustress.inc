
OPT ?= -O2 # -flto

GCC_OPTS += $(OPT)
CFLAGS += -DCPU_AMA_RISCV -DUSE_C

all: $(TARGET).elf

include ../../Makefile.inc
