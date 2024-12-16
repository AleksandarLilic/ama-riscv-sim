N_IN := 20
LOOPS := 1
TARGET := n_$(N_IN).elf

OPT := -O3 -flto
CFLAGS += -DN_IN=$(N_IN) -DLOOPS=$(LOOPS)

all: $(TARGET)

include ../Makefile.inc
