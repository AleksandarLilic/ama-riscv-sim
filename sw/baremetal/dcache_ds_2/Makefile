DS := aos soa
LOOPS := 1

BASE_NAME := test
TARGETS := $(addsuffix .elf, $(addprefix $(BASE_NAME)_, $(DS)))

TEST_DEFINES := $(foreach d,$(DS), \
				$(BASE_NAME)_$(d):-DDS_$(shell echo $(d) | tr a-z A-Z))

HEADERS := test_arrays.h
CODEGEN_PY := ./codegen.py

OPT := -O1 -flto
CFLAGS := -DLOOPS=$(LOOPS)

#MATH_LIB_FLAGS := -DLOAD_OPT
#MATH_LIB_FLAGS :=
#MATH_LIB_FLAGS += -DFORCE_INLINE
#MATH_LIB_FLAGS += -DSIMD_UNROLL
CFLAGS += $(MATH_LIB_FLAGS)

all: $(TARGETS)

codegen: headers
clean_codegen: clean_headers

headers: $(HEADERS)

ARR_LEN = 1000
$(HEADERS): $(CODEGEN_PY)
	$(CODEGEN_PY) $(ARR_LEN)

clean_headers:
	rm -f $(HEADERS) data.png

include ../Makefile.inc
