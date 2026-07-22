LOOPS := 10
TYPES := int4 int8 int16 int16_int8 int16_int4 int8_int4 int8_int2

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):NF" > $@

#MATH_LIB_FLAGS := -DLOAD_OPT
MATH_LIB_FLAGS :=
MATH_LIB_FLAGS += -DFORCE_INLINE
MATH_LIB_FLAGS += -DSIMD_UNROLL

OPT := -O3 -flto
CFLAGS := -DLOOPS=$(LOOPS)

all: $(TARGETS)

include ../Makefile.inc
