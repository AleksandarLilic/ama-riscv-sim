N_IN := 3000
LOOPS := 1
TARGET := n_$(N_IN)

CFLAGS += -DN_IN=$(N_IN) -DLOOPS=$(LOOPS)
CRT0 := ../crt0.S
GCC_OPTS += -O2

include ../Makefile
