LOOPS := 10
# types read (matrix, matrix),
# or (weights, activations) as MLP's batching where weights are narrow types
TYPES := int8 int4_int8 int2_int8

ARR_LEN := 256
# deliberately *not* multiples of the ukr's MR (4) / NR (2), so that both the
# m % MR and n % NR edges are exercised; N is the batch
M_OUT := 10
N_BATCH := 5
# widest kernel K tile: i8_i8 is K_ATOMIC 4 * K_UNROLL 4, i4_i8 is 8 * 2,
# i2_i8 is 16 * 1 - all land on 16 on the SIMD flavor
K_STEP := 16

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):NF" "al ual:" > $@

MATH_LIB_UNROLL ?= 1
include ../Makefile.math_lib_opts.mk

OPT := -O3 -flto
CFLAGS += -DLOOPS=$(LOOPS) $(MATH_LIB_FLAGS) -DFORCE_INLINE

APP_REQS := test_arrays.h
CODEGEN_PY := ./codegen.py

all: $(APP_REQS) $(TARGETS)

codegen: $(APP_REQS)
clean_codegen: clean_headers

# --b_t: activations arrive as N rows of k-contiguous data, same as the weights,
#        so the ukr walks both operands along k and nothing needs transposing
# --c_t: C is written batch-major, C[n][m], so a column of it is contiguous
CODEGEN_DATA_DIMS := \
    -K $(ARR_LEN) -M $(M_OUT) -N $(N_BATCH) \
    --k_step $(K_STEP) --b_t --c_t
$(APP_REQS): $(CODEGEN_PY) ../codegen_common.py ../codegen_gemm.py
	$(CODEGEN_PY) $(CODEGEN_DATA_DIMS) --types $(TYPES) --ual --out_buf c poison

clean_headers:
	rm -f $(APP_REQS)

include ../Makefile.inc
