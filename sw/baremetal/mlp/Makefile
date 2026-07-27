TYPES := w8a8 w4a8 w2a8
OPT := -O3 -flto

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(TYPES):" > $@

MARCH := rv32im_zicsr_zifencei_zicntr
MATH_LIB_FLAGS := -DPARTIAL_ZBB_SUPPORT

ifeq ($(strip $(LOAD_OPT)), 1)
MATH_LIB_FLAGS += -DLOAD_OPT
$(info Building LOAD_OPT)
else ifeq ($(strip $(SIMD)), 1)
$(info Building SIMD)
MARCH := rv32im_zicsr_zifencei_zicntr_xsimd
MATH_LIB_FLAGS += -DFORCE_INLINE
MATH_LIB_FLAGS += -DSIMD_UNROLL
else
$(info Building generic)
endif

# so the icache is happy
#CODE_ALIGN := -falign-loops=64 # -falign-functions=64
CFLAGS += $(MATH_LIB_FLAGS) $(CODE_ALIGN)
GCC_OPTS += -Wall -Werror

APP_REQS := model_w8a8_64-64-64-10.h \
			model_w4a8_64-64-64-10.h \
			model_w2a8_64-64-64-10.h

model_%.h: model_%.pth
	python convert_model.py $(basename $@).pth

all: $(APP_REQS) $(TARGETS)

include ../Makefile.inc
