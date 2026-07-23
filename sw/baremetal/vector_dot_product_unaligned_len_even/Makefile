LOOPS := 10
TYPES := int2 int4 int8 int16 int16_int8 int16_int4 int16_int2 int8_int4 int8_int2 int4_int2

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):NF" > $@

MARCH := rv32im_zicsr_zifencei_zicntr
MATH_LIB_FLAGS :=

ifeq ($(strip $(LOAD_OPT)), 1)
MATH_LIB_FLAGS += -DLOAD_OPT
$(info Building LOAD_OPT)
else ifeq ($(strip $(SIMD)), 1)
$(info Building SIMD)
MARCH := rv32im_zicsr_zifencei_zicntr_xsimd
#MATH_LIB_FLAGS += -DFORCE_INLINE
#MATH_LIB_FLAGS += -DSIMD_UNROLL
else
$(info Building generic)
endif

OPT := -O2 -flto
CFLAGS += -DLOOPS=$(LOOPS) $(MATH_LIB_FLAGS)

all: $(TARGETS)

include ../Makefile.inc
