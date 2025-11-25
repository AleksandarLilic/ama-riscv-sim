TARGET := basic

GCC_OPTS += -O1

MATH_LIB_FLAGS ?= -DCUSTOM_ISA
CFLAGS += $(MATH_LIB_FLAGS)

all: $(TARGET).elf

include ../Makefile.inc
