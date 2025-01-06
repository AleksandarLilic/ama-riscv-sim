N_IN := 18
LOOPS := 1
TARGET := n_$(N_IN)

CFLAGS += -DN_IN=$(N_IN) -DLOOPS=$(LOOPS)
GCC_OPTS += -O3 -flto

all: $(TARGET).elf

include ../Makefile.inc
