LOOPS := 1
TYPES := int16 # int4 int8 int16 int16_int8 int16_int4 int8_int4 int8_int2
LENGTHS := small large

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):NF" "$(LENGTHS):LEN" > $@

#MATH_LIB_FLAGS := -DLOAD_OPT
#MATH_LIB_FLAGS :=
#MATH_LIB_FLAGS += -DFORCE_INLINE
#MATH_LIB_FLAGS += -DSIMD_UNROLL
OPT := -O2 -flto
CFLAGS := -DLOOPS=$(LOOPS) $(MATH_LIB_FLAGS)
CFLAGS += -DALIGNED

all: $(TARGETS)

include ../Makefile.inc
