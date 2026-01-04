TYPES := uint8 int8 uint16 int16 uint32 int32 uint64 int64 float32 float64
OPS := add sub mul div
LOOPS := 1

TARGETS_ADD := $(addsuffix .elf, $(addprefix $(word 1, $(OPS))_, $(TYPES)))
TARGETS_SUB := $(addsuffix .elf, $(addprefix $(word 2, $(OPS))_, $(TYPES)))
TARGETS_MUL := $(addsuffix .elf, $(addprefix $(word 3, $(OPS))_, $(TYPES)))
TARGETS_DIV := $(addsuffix .elf, $(addprefix $(word 4, $(OPS))_, $(TYPES)))

# Generate TEST_DEFINES by iterating over each type and op
# For each (type, op) pair, produce a line like:
#   add_uint8:-DNF_UINT8:-DOP_ADD
TEST_DEFINES := $(foreach t,$(TYPES), \
                $(foreach o,$(OPS), \
                $(o)_$(t):-DNF_$(shell echo $(t) | tr a-z A-Z):-DOP_$(shell echo $(o) | tr a-z A-Z)))

HEADER_PREFIX := test_arrays_
HEADERS := $(addsuffix .h, $(addprefix $(HEADER_PREFIX), $(OPS)))
PYVER := python3
CODEGEN_PY := codegen.py

OPT := -O1 -flto
CFLAGS := -DLOOPS=$(LOOPS)

#MATH_LIB_FLAGS ?= -DLOAD_OPT
MATH_LIB_FLAGS ?= -DCUSTOM_ISA
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
