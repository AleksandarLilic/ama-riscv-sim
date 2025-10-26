#TARGET := n_$(N_IN)
LOOPS := 2
TARGET := basic
CACHE_ALIGNED := 1

ifeq ($(CACHE_ALIGNED), 1)
CFLAGS += -DCACHE_ALIGNED
endif
CFLAGS += -DLOOPS=$(LOOPS)

OPT := -O3 -flto

all: $(TARGET).elf

include ../Makefile.inc
