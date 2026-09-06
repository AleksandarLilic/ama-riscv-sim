WARMUP ?= 1
LOOPS ?= 1

TYPES := int2 int4 int8 int16 int16_int8 int16_int4 int16_int2 int8_int4 int8_int2 int4_int2

ARR_LEN := 512
# widest kernel K tile: M_UNROLL_DOTV tiles int2 by 128 elements
# (common_math_dotv_simd.c, deg=7),
# so ARR_LEN must clear that for a tail-free 'al';
# every other variant tiles by 64 or less
K_STEP := 128

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):NF" "al ual:" > $@

MATH_LIB_UNROLL_DOTV ?= 1
include ../Makefile.math_lib_opts.mk

OPT := -O3 -flto
CFLAGS += -DLOOPS=$(LOOPS) $(MATH_LIB_FLAGS)
# measure the kernel's steady-state loop, not a flattened copy of it:
# gcc wants to unroll aggresively otherwise, and measurement gets muddy.
# size-based, not 'max-completely-peel-times': the LOAD_OPT mixed kernels have
# small inner loops that must still flatten to make sense
CFLAGS += --param max-completely-peeled-insns=100

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
