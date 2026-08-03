LOOPS := 10
TYPES := int2 int4 int8 int16 int16_int8 int16_int4 int16_int2 int8_int4 int8_int2 int4_int2

ARR_LEN := 512
# widest kernel K tile: M_UNROLL tiles int8_int4/int8_int2 by 64 elements
# (common_math_simd.c, deg=6), so ARR_LEN must clear that for a tail-free 'al'
K_STEP := 64

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):NF" "al ual:" > $@

MATH_LIB_UNROLL ?= 1
include ../Makefile.math_lib_opts.mk

OPT := -O3 -flto
CFLAGS += -DLOOPS=$(LOOPS) $(MATH_LIB_FLAGS)

APP_REQS := test_arrays.h
CODEGEN_PY := ./codegen.py

all: $(APP_REQS) $(TARGETS)

codegen: $(APP_REQS)
clean_codegen: clean_headers

$(APP_REQS): $(CODEGEN_PY) ../codegen_common.py ../codegen_gemm.py
	$(CODEGEN_PY) -K $(ARR_LEN) --types $(TYPES) --k_step $(K_STEP) --ual

clean_headers:
	rm -f $(APP_REQS)

include ../Makefile.inc
