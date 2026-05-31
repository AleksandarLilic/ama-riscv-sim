
OPT ?= -O2 # -flto

GCC_OPTS += $(OPT)
CFLAGS += -DCPU_AMA_RISCV -DUSE_C
CFLAGS += -DUSTRESS_TEST_NAME='"$(notdir $(CURDIR))"'

all: $(TARGET).elf

include ../../Makefile.inc
