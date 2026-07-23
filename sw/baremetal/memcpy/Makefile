TYPES := int8 int16 int32 int64
#LENGTHS := tiny small medium large
LENGTHS := small large
LOOPS := 1

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):NF" "$(LENGTHS):LEN" > $@

HEADER_PREFIX := test_arrays_
HEADERS := $(addsuffix .h, $(addprefix $(HEADER_PREFIX), $(LENGTHS)))
CODEGEN_PY := ./codegen.py

LIBS += -lc
USE_NEWLIB := 1

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
