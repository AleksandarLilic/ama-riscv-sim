WARMUP ?= 1
LOOPS ?= 1

VEC_ONLY ?= 1 # not every (type x op) combination has vectorized flavor
NFS := uint8 int8 uint16 int16
OPS := add sub wmul

ifeq ($(strip $(VEC_ONLY)), 0)
NFS += uint32 int32 uint64 int64 float32 float64
OPS += div
endif

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py --groups "$(OPS):OP" "$(NFS):NF" > $@

HEADER_PREFIX := test_arrays_
HEADERS := $(addsuffix .h, $(addprefix $(HEADER_PREFIX), $(OPS)))
CODEGEN_PY := codegen.py

# at -O3, compiler would do SWAR for int8 through `-ftree-loop-vectorize`
# which is a different point of comparison
# use -O1 and add scheduling explicitly instead
OPT := -O1 -fschedule-insns -fschedule-insns2
OPT += -flto
CFLAGS := -DLOOPS=$(LOOPS)

MATH_LIB_UNROLL_DOTV ?= 1
include ../Makefile.math_lib_opts.mk

CFLAGS += $(MATH_LIB_FLAGS)

APP_REQS := test_arrays_add.h test_arrays_sub.h \
    test_arrays_wmul.h test_arrays_div.h
CODEGEN_PY := ./codegen.py

all: $(APP_REQS) add sub wmul div

codegen: $(APP_REQS)
clean_codegen: clean_headers

add: $(TARGETS_ADD)
sub: $(TARGETS_SUB)
wmul: $(TARGETS_WMUL)
div: $(TARGETS_DIV)

headers: $(HEADERS)

$(APP_REQS): $(CODEGEN_PY) ../codegen_common.py
	$(CODEGEN_PY) $(word 3, $(subst _, ,$*))

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
