TYPES := w8a8 w4a8 w2a8
TARGETS := $(addsuffix .elf, $(TYPES))
OPT := -O3 -flto

TEST_DEFINES := $(foreach t,$(TYPES), $(t):-D$(shell echo $(t) | tr a-z A-Z))

# so the icache is happy
#CODE_ALIGN := -falign-loops=64 # -falign-functions=64
PERF_COUNTERS ?= -DMHPM

#MATH_LIB_FLAGS := -DLOAD_OPT # drop xsimd before using this
MATH_LIB_FLAGS := -DPARTIAL_ZBB_SUPPORT -DSIMD_UNROLL
MATH_LIB_FLAGS += -DFORCE_INLINE
CFLAGS += $(MATH_LIB_FLAGS) $(CODE_ALIGN) $(PERF_COUNTERS)
GCC_OPTS += -Wall -Werror

APP_REQS := model_w8a8_64-64-64-10.h \
			model_w4a8_64-64-64-10.h \
			model_w2a8_64-64-64-10.h

model_%.h: model_%.pth
	python convert_model.py $(basename $@).pth

all: $(APP_REQS) $(TARGETS)

include ../Makefile.inc
