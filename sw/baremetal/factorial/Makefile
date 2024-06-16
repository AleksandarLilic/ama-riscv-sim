N_IN := 20
LOOPS := 1
TARGET := n_$(N_IN)

CFLAGS += -DN_IN=$(N_IN) -DLOOPS=$(LOOPS)
GCC_OPTS += -O2 -flto

include ../Makefile.inc
