TARGET := coremark

GCC_OPTS += -O2 -flto
CFLAGS += -DFLAGS_STR="\"$(GCC_OPTS)\"" -DITERATIONS=6500 -DPERFORMANCE_RUN=1

all: $(TARGET).elf

.PHONY: check
check:
	md5sum -c coremark.md5

include ../Makefile.inc
