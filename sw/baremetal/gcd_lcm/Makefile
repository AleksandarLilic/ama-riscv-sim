N_NAME := large
LOOPS := 1
N_IN := N_$(shell echo $(N_NAME) | tr '[:lower:]' '[:upper:]')
TARGET := n_$(N_NAME)

CFLAGS += -D$(N_IN) -DLOOPS=$(LOOPS)
GCC_OPTS += -O3 -flto

include ../Makefile.inc
