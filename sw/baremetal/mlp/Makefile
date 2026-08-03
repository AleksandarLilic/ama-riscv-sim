TYPES := w8a8 w4a8 w2a8

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):" > $@

MATH_LIB_UNROLL ?= 1
include ../Makefile.math_lib_opts.mk
MATH_LIB_FLAGS += -DPARTIAL_ZBB_SUPPORT
MATH_LIB_FLAGS += -DFORCE_INLINE
#MATH_LIB_FLAGS += -DCUSTOM_ISA_SCP

# so the icache is happy
#CODE_ALIGN := -falign-loops=64 # -falign-functions=64
CFLAGS += $(MATH_LIB_FLAGS) $(CODE_ALIGN)
GCC_OPTS += -Wall -Werror
OPT := -O3 -flto

APP_REQS := model_w8a8_64-64-64-10.h \
			model_w4a8_64-64-64-10.h \
			model_w2a8_64-64-64-10.h

model_%.h: model_%.pth
	python convert_model.py $(basename $@).pth

all: $(APP_REQS) $(TARGETS)

include ../Makefile.inc
