TARGET := basic
OPT := -O3 -flto

# so the icache is happy
CODE_ALIGN := -falign-loops=64 # -falign-functions=64

#MATH_LIB_FLAGS ?= -DLOAD_OPT
MATH_LIB_FLAGS ?= -DCUSTOM_ISA
MATH_LIB_FLAGS += -DFORCE_INLINE
MATH_LIB_FLAGS += -DSTREAMING
CFLAGS += $(MATH_LIB_FLAGS) $(CODE_ALIGN)

all: $(TARGET).elf

include ../Makefile.inc
