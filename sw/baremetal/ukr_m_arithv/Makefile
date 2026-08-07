
VEC_ONLY ?= 1 # not every (type x op) combination has vectorized flavor
NFS := uint8 int8 uint16 int16
OPS := add sub mul

ifeq ($(strip $(VEC_ONLY)), 0)
NFS += uint32 int32 uint64 int64 float32 float64
OPS += div
endif

LOOPS := 1

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py --groups "$(OPS):OP" "$(NFS):NF" > $@

HEADER_PREFIX := test_arrays_
HEADERS := $(addsuffix .h, $(addprefix $(HEADER_PREFIX), $(OPS)))
CODEGEN_PY := codegen.py

OPT := -O1 -flto
CFLAGS := -DLOOPS=$(LOOPS)

MATH_LIB_UNROLL_DOTV ?= 1
include ../Makefile.math_lib_opts.mk

CFLAGS += $(MATH_LIB_FLAGS)

APP_REQS := test_arrays_add.h test_arrays_sub.h \
    test_arrays_mul.h test_arrays_div.h
CODEGEN_PY := ./codegen.py

all: $(APP_REQS) add sub mul div

codegen: $(APP_REQS)
clean_codegen: clean_headers

add: $(TARGETS_ADD)
sub: $(TARGETS_SUB)
mul: $(TARGETS_MUL)
div: $(TARGETS_DIV)

headers: $(HEADERS)

$(APP_REQS): $(CODEGEN_PY) ../codegen_common.py
	$(CODEGEN_PY) $(word 3, $(subst _, ,$*))

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
