
REPO_ROOT := $(shell git rev-parse --show-toplevel)

# set up for gcc14
RV_GNU := $(RV_GNU_LATEST)
# e.g. `export RV_GNU_LATEST=riscv64-unknown-elf`
SHELL  := /bin/bash

COMMON_OBJ_NAMES += crt0.o common.o common_math.o mini-printf.o
COMMON_PATH := $(REPO_ROOT)/sw/baremetal/common
COMMON_OBJS = $(addprefix $(COMMON_PATH)/, $(COMMON_OBJ_NAMES))

C_SRCS := $(wildcard *.c)
S_SRCS := $(wildcard *.S)
LD_SRC := $(COMMON_PATH)/link.ld
SRCS := $(C_SRCS) $(S_SRCS)
INC += $(COMMON_PATH)
LIBS += -lgcc
USER_DEFINES =

# if not set in test's Makefile, default to all supported ISA extensions
#export MARCH ?= rv32imc_zicsr_zifencei_zicntr
export MARCH ?= rv32i_zmmul_zicsr_zifencei_zicntr
#export MARCH ?= rv32i_zicsr
export MABI ?= ilp32
export CFLAGS
GCC_OPTS += -march=$(MARCH) -mabi=${MABI} -mcmodel=medany
GCC_OPTS += -static -fno-pic
GCC_OPTS += -fvisibility=hidden -ffreestanding -nostdlib -nostartfiles -g
#GCC_OPTS += -save-temps
GCC_OPTS += -T $(LD_SRC) -Wl,--build-id=none
GCC_OPTS += -Wl,--no-warn-rwx-segments
GCC_OPTS += -Wl,-Map=$(TARGET).map

OBJDUMP_OPTS := -D -Mnumeric -t --disassembler-options=no-aliases
#OBJDUMP_OPTS := -S # inline C source code in disassembly
MEM_WIDTH := 32

RV_GNU_DEFAULT := riscv64-unknown-elf
BIN2HEX :=
HEX ?= 1
ifeq ($(HEX), 1)
	ifneq (, $(shell command -v $(RV_GNU)-bin2hex 2> /dev/null))
		BIN2HEX := $(RV_GNU)-bin2hex
	else ifneq (, $(shell command -v $(RV_GNU_DEFAULT)-bin2hex 2> /dev/null))
		BIN2HEX := $(RV_GNU_DEFAULT)-bin2hex
	else
		$(error "No bin2hex tool found")
	endif
endif

# run with HEX=1 to generate hex file for RTL simulation
$(TARGET).elf: $(SRCS) $(INC)/*.h $(CRT0) $(LD_SRC) $(COMMON_OBJS)
	@$(RV_GNU)-gcc $(GCC_OPTS) $(CFLAGS) -I$(INC) $(SRCS) $(COMMON_OBJS) \
	-o $@ $(USER_DEFINES) $(LIBS)
	@$(RV_GNU)-objdump $(OBJDUMP_OPTS) $@ > $(TARGET).dump
	@$(RV_GNU)-objcopy $@ -O binary $(TARGET).bin
	@if [ "$(HEX)" = "1" ]; then \
		$(BIN2HEX) -w $(MEM_WIDTH) $(TARGET).bin $(TARGET).hex; \
	fi
	@echo "Generated $@"
	@$(RV_GNU)-size $@

common:
	@make -C $(COMMON_PATH) --no-print-directory all -j

$(COMMON_OBJS):
	@make -C $(COMMON_PATH) --no-print-directory $@

clean:
	rm -f *.elf* *.dump *.hex *.bin *.o *.d *.i *.s *_symbols.json *.map

clean_common:
	@make -C $(COMMON_PATH) --no-print-directory clean

cleanall: clean clean_common

.PHONY: all clean
