LOOPS := 1
TARGET := int8

MATH_LIB_FLAGS ?= -DCUSTOM_ISA

OPT := -O3 -flto
CFLAGS := -DLOOPS=$(LOOPS) $(MATH_LIB_FLAGS)

all: $(TARGET).elf

include ../Makefile.inc
