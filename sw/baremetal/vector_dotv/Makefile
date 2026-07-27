LOOPS := 10
TYPES := int2 int4 int8 int16 int16_int8 int16_int4 int16_int2 int8_int4 int8_int2 int4_int2

ARR_LEN := 256

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):NF" "al ual:" > $@

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

OPT := -O3 -flto
CFLAGS += -DLOOPS=$(LOOPS) $(MATH_LIB_FLAGS)

APP_REQS := test_arrays.h
CODEGEN_PY := ./codegen.py

all: $(APP_REQS) $(TARGETS)

codegen: $(APP_REQS)
clean_codegen: clean_headers

$(APP_REQS): $(CODEGEN_PY)
	$(CODEGEN_PY) $(ARR_LEN)

clean_headers:
	rm -f $(APP_REQS)

include ../Makefile.inc
