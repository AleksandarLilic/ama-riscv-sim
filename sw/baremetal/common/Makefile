
# set up for "gcc version 13.2.0 (gc891d8dc23e)"
RV_GNU := $(RV_GNU_LATEST)
# e.g. export RV_GNU_LATEST=riscv64-unknown-elf
SHELL  := /bin/bash
C_SRCS := $(wildcard *.c)
S_SRCS := $(wildcard *.S)
OBJECTS_C := $(C_SRCS:.c=.o)
OBJECTS_S := $(S_SRCS:.S=.o)

#MARCH ?= rv32imc_zicsr_zifencei
MARCH ?= rv32i_zmmul_zicsr_zifencei_zicntr
MABI ?= ilp32
GCC_OPTS += -march=$(MARCH) -mabi=${MABI} -mcmodel=medany
GCC_OPTS += -static -fno-pic
GCC_OPTS += -fvisibility=hidden -ffreestanding
#GCC_OPTS += -save-temps
GCC_OPTS += -O3 -flto # -ffunction-sections -fdata-sections
#GCC_OPTS += g

#MATH_LIB_OPT ?= -DCUSTOM_ISA
#MATH_LIB_OPT ?= -DLOAD_OPT
#CFLAGS += $(MATH_LIB_OPT)

# set all target to make all objects
all: $(OBJECTS_C) $(OBJECTS_S)

%.o: %.c
	@$(RV_GNU)-gcc $(GCC_OPTS) $(CFLAGS) -c $< -o $@ $(USER_DEFINES)

%.o: %.S
	@$(RV_GNU)-gcc $(GCC_OPTS) $(CFLAGS) -c $< -o $@ $(USER_DEFINES)

clean:
	rm -f *.o *.d *.i *.s

.PHONY: all clean
