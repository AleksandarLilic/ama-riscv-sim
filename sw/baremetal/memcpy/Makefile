TYPES := int8 int16 int32 int64
#LENGTHS := tiny small medium large
LENGTHS := small large
LOOPS := 1
TARGETS := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(TYPES)))

TEST_DEFINES := $(foreach t,$(TYPES), \
				$(foreach l,$(LENGTHS), \
				$(t)_$(l):-DNF_$(shell echo $(t) | tr a-z A-Z):-DLEN_$(shell echo $(l) | tr a-z A-Z)))

HEADER_PREFIX := test_arrays_
HEADERS := $(addsuffix .h, $(addprefix $(HEADER_PREFIX), $(LENGTHS)))
CODEGEN_PY := ./codegen.py

LIBS += -lc
COMMON_OBJ_NAMES = crt0.o common.o mini-printf.o newlib_defs.o
CFLAGS += -D_USE_NEWLIB

CFLAGS += -DLOOPS=$(LOOPS)
#CFLAGS += -DCHECK_RESULTS
GCC_OPTS += -O3 -flto

all: $(TARGETS)

codegen: headers
clean_codegen: clean_headers

headers: $(HEADERS)

$(HEADERS): $(CODEGEN_PY)
	$(CODEGEN_PY) $(word 3, $(subst _, ,$*))

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
