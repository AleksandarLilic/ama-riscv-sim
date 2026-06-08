TYPES := uint8 int8 uint16 int16 uint32 int32 uint64 int64 float32 float64
OPS := add sub mul div
LOOPS := 1

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py --groups "$(OPS):OP" "$(TYPES):NF" > $@

HEADER_PREFIX := test_arrays_
HEADERS := $(addsuffix .h, $(addprefix $(HEADER_PREFIX), $(OPS)))
PYVER := python3
CODEGEN_PY := codegen.py

OPT := -O1 -flto
CFLAGS := -DLOOPS=$(LOOPS)

#MATH_LIB_FLAGS := -DLOAD_OPT
#MATH_LIB_FLAGS :=
#MATH_LIB_FLAGS += -DFORCE_INLINE
#MATH_LIB_FLAGS += -DSIMD_UNROLL
CFLAGS += $(MATH_LIB_FLAGS)

all: add sub mul div

codegen: headers
clean_codegen: clean_headers

add: $(TARGETS_ADD)
sub: $(TARGETS_SUB)
mul: $(TARGETS_MUL)
div: $(TARGETS_DIV)

headers: $(HEADERS)

$(HEADERS): $(CODEGEN_PY)
	$(PYVER) $(CODEGEN_PY) $(word 3, $(subst _, ,$*))

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
