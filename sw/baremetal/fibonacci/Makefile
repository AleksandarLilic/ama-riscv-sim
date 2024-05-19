N_IN := 18
LOOPS := 1
TARGET := n_$(N_IN)

CFLAGS += -DN_IN=$(N_IN) -DLOOPS=$(LOOPS)
GCC_OPTS += -O2

include ../Makefile.inc

#define EXPECTED_FIB_RESULT 55 // for n = 10
#define EXPECTED_FIB_RESULT 610 // for n = 15
#define EXPECTED_FIB_RESULT 2584 // for n = 18
#define EXPECTED_FIB_RESULT 6765 // for n = 20
#define EXPECTED_FIB_RESULT 832040 // for n = 30
#define EXPECTED_FIB_RESULT 102334155 // for n = 40
