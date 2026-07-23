TARGET := ustress
LOOPS ?= 1000
CFLAGS += -DLOOPS=$(LOOPS)

LIBS += -lc
USE_NEWLIB := 1

GCC_OPTS += -Wl,--defsym=__stack_size=1K

include ../Makefile.ustress.inc
