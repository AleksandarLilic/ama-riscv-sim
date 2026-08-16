WARMUP ?= 1
LOOPS ?= 1

# types as (matrix, matrix)
TYPES := int16 int8 int4 int2

SQUARE ?= 0
CODEGEN_FLAGS = --max_bytes 98304

# source is M x N, destination is N x M;
# both must be multiples of the widest block, which is i2's 16:
# a partial block has no kernel in any flavor

# these are ~min dimensions where GCC doesn't unroll aggressively
# i.e. around (N/BLK) > 3, which for i2 (M_TXP_I2_BLK 16) is N_COL >= 48
#M_ROWS := 32
#N_COLS := 48

ifeq ($(strip $(SQUARE)), 0)
M_ROWS := 128
N_COLS := 96
else
M_ROWS := $(SQUARE)
N_COLS := $(SQUARE)
endif

USE_PADDED_STRIDE ?= 0 # purely to test out codegen and padding, add if needed
ifeq ($(strip $(USE_PADDED_STRIDE)), 1)
LDA ?= $(shell expr $(N_COLS) + 16)
LDC ?= $(shell expr $(M_ROWS) + 16)
CODEGEN_FLAGS += --lda $(LDA) --ldc $(LDC)
endif

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):NF" > $@

MATH_LIB_UNROLL_DOTV ?= 1
include ../Makefile.math_lib_opts.mk

OPT := -O3 -flto
CFLAGS += -DLOOPS=$(LOOPS) $(MATH_LIB_FLAGS) -DFORCE_INLINE

APP_REQS := test_arrays.h
CODEGEN_PY := ./codegen.py

all: $(APP_REQS) $(TARGETS)

codegen: $(APP_REQS)
clean_codegen: clean_headers

CODEGEN_DATA_DIMS := -M $(M_ROWS) -N $(N_COLS) $(CODEGEN_FLAGS)
$(APP_REQS): $(CODEGEN_PY) ../codegen_common.py ../codegen_txp.py
	$(CODEGEN_PY) $(CODEGEN_DATA_DIMS) --types $(TYPES)

clean_headers:
	rm -f $(APP_REQS)

include ../Makefile.inc
