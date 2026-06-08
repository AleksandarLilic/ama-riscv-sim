#TARGET := n_$(N_IN)
LOOPS := 100
TYPES := load store mixed

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):" > $@

CACHE_ALIGNED := 1

ifeq ($(CACHE_ALIGNED), 1)
CFLAGS += -DCACHE_ALIGNED
endif

CFLAGS += -DLOOPS=$(LOOPS)

OPT := -O3 -flto

all: $(TARGETS)

include ../Makefile.inc
