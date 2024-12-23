TARGET := basic
OPT := -O3 -flto

# so the icache is happy
CODE_ALIGN := -falign-loops=64 # -falign-functions=64

#MATH_LIB_FLAGS ?= -DLOAD_OPT
MATH_LIB_FLAGS ?= -DCUSTOM_ISA
MATH_LIB_FLAGS += -DFORCE_INLINE
MATH_LIB_FLAGS += -DSTREAMING
CFLAGS += $(MATH_LIB_FLAGS) $(CODE_ALIGN)

APP_REQS := model_w8a8_64-64-64-10.h

model_%.h: model_%.pth
	python convert_model.py $(basename $@).pth

all: $(APP_REQS) $(TARGET).elf

include ../Makefile.inc
