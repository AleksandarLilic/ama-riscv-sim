
# set up for "gcc version 13.2.0 (gc891d8dc23e)"
RV_GNU := $(RV_GNU_LATEST)
# e.g. export RV_GNU_LATEST=riscv64-unknown-elf
SHELL  := /bin/bash

COMMON = ../common
COMMON_SRC = $(COMMON)/common.c
CRT0 ?= $(COMMON)/crt0.S
C_SRCS := $(wildcard *.c)
S_SRCS := $(wildcard *.S)
LD_SRC := $(COMMON)/link.ld
SRCS := $(C_SRCS) $(S_SRCS) $(CRT0) $(COMMON_SRC) $(LIB_SRCS)
INC := $(COMMON)
LIBS += -lgcc
USER_DEFINES =

GCC_OPTS += -march=rv32i_zicsr -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -ffreestanding -nostdlib -nostartfiles -T $(LD_SRC) -Wl,--build-id=none -save-temps
#GCC_OPTS += g

# run with RTL_SIM=1 to generate hex file for RTL simulation
$(TARGET).elf: $(SRCS) $(INC)/*.h $(CRT0) $(LD_SRC)
	@$(RV_GNU)-gcc $(GCC_OPTS) $(CFLAGS) -I$(INC) $(SRCS) -o $@ $(USER_DEFINES) $(LIBS)
	@$(RV_GNU)-objdump -D -Mnumeric -t --disassembler-options=no-aliases $@ > $(basename $@).dump
	@$(RV_GNU)-objcopy $(basename $@).elf -O binary $(basename $@).bin
	@if [ -n "$(RTL_SIM)" -a "$(RTL_SIM)" == "1" ]; then \
		$(RV_GNU)-bin2hex -w 32 $(basename $@).bin $(basename $@).hex; \
	fi
	@echo "Generated $@"

clean:
	rm -f *.elf *.dump *.hex *.bin *.o *.d *.i *.s

.PHONY: all clean
