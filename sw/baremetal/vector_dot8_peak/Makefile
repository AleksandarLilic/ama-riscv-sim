LOOPS := 10
TYPES := single_acc multiple_acc

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):" "int8:-" > $@

OPT := -O3 -flto
CFLAGS := -DLOOPS=$(LOOPS) $(MATH_LIB_FLAGS)

all: $(TARGETS)

include ../Makefile.inc
