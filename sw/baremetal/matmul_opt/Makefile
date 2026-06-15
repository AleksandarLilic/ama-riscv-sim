# optimization variants
OPT_V := load_b tiling txp_b
LOOPS := 1

NF := int8 int16

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(NF):NF" "$(OPT_V):OPT_V" > $@

OPT := -O3 -flto
CFLAGS += -DLOOPS=$(LOOPS) -DUSE_PROF_MARKERS

HEADER_PREFIX := test_matrices_
HEADERS := $(addsuffix .h, $(addprefix $(HEADER_PREFIX), $(NF)))
CODEGEN_PY := ./codegen.py
INC_PATHS += .

all: int8

int8: $(TARGETS)

codegen: headers
clean_codegen: clean_headers

headers: $(HEADERS)

# dependent on the dcache size
A_ROWS = 64
B_COLS = 64
A_COLS_B_ROWS = 64

$(HEADERS): $(CODEGEN_PY)
	$(CODEGEN_PY) $(word 3, $(subst _, ,$*)) $(A_ROWS) $(B_COLS) $(A_COLS_B_ROWS)

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
