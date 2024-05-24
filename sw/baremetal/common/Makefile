
# set up for "gcc version 13.2.0 (gc891d8dc23e)"
RV_GNU := $(RV_GNU_LATEST)
# e.g. export RV_GNU_LATEST=riscv64-unknown-elf
SHELL  := /bin/bash
C_SRCS := $(wildcard *.c)
S_SRCS := $(wildcard *.S)
OBJECTS_C := $(C_SRCS:.c=.o)
OBJECTS_S := $(S_SRCS:.S=.o)

GCC_OPTS += -march=rv32i_zicsr -mabi=ilp32 -static -mcmodel=medany -fno-pic
GCC_OPTS += -fvisibility=hidden -ffreestanding -save-temps
#GCC_OPTS += g

# set all target to make all objects
all: $(OBJECTS_C) $(OBJECTS_S)

%.o: %.c
	@$(RV_GNU)-gcc $(GCC_OPTS) -c $< -o $@ $(USER_DEFINES)

%.o: %.S
	@$(RV_GNU)-gcc $(GCC_OPTS) -c $< -o $@ $(USER_DEFINES)

clean:
	rm -f *.o *.d *.i *.s

.PHONY: all clean
