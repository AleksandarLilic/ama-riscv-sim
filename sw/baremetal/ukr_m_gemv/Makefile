WARMUP ?= 1
LOOPS ?= 1

# types read (matrix, vector), so the narrow weight combs are <narrow>_int8
TYPES := int8 int4_int8 int2_int8

ARR_LEN := 256
# deliberately *not* a multiple of M_DOTF_<TYPES>_MR (4) to test the tail
M_OUT := 10
# kernel K tile: K_ATOMIC 4 * K_UNROLL 4 on the SIMD flavor
K_STEP := 16

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):NF" "al ual:" > $@

MATH_LIB_UNROLL_DOTV ?= 1
include ../Makefile.math_lib_opts.mk

OPT := -O3 -flto
CFLAGS += -DLOOPS=$(LOOPS) $(MATH_LIB_FLAGS) -DFORCE_INLINE

APP_REQS := test_arrays.h
CODEGEN_PY := ./codegen.py

all: $(APP_REQS) $(TARGETS)

codegen: $(APP_REQS)
clean_codegen: clean_headers

CODEGEN_DATA_DIMS := -K $(ARR_LEN) -M $(M_OUT) --k_step $(K_STEP)
$(APP_REQS): $(CODEGEN_PY) ../codegen_common.py ../codegen_gemm.py
	$(CODEGEN_PY) $(CODEGEN_DATA_DIMS) --types $(TYPES) --ual --out_buf y poison

clean_headers:
	rm -f $(APP_REQS)

include ../Makefile.inc
