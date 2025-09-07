
REPO_ROOT := $(shell git rev-parse --show-toplevel)

RV_GNU := $(RV_GNU_LATEST)
# e.g. `export RV_GNU_LATEST=riscv64-unknown-elf`
SHELL  := /bin/bash

# Turn on increased build verbosity by defining BUILD_VERBOSE in test Makefile
# or use V=1 on the make command line

BUILD_VERBOSE ?= 0

ifeq ("$(origin V)", "command line")
BUILD_VERBOSE := $(V)
endif

ifeq ($(BUILD_VERBOSE), 0)
Q = @
else
Q =
endif

# if not set in test's Makefile, default to all object files w/o newlib
COMMON_OBJ_NAMES ?= crt0.o common.o common_math.o mini-printf.o # newlib_defs.o
COMMON_SRC_PATH := $(REPO_ROOT)/sw/baremetal/common
COMMON_BUILD_DIR := ./common_build
COMMON_OBJS = $(addprefix $(COMMON_BUILD_DIR)/, $(COMMON_OBJ_NAMES))
$(shell mkdir -p $(COMMON_BUILD_DIR))
vpath %.c $(COMMON_SRC_PATH)
vpath %.h $(COMMON_SRC_PATH)
vpath %.S $(COMMON_SRC_PATH)

C_SRCS += $(wildcard *.c)
S_SRCS += $(wildcard *.S)
LD_SRC := $(COMMON_SRC_PATH)/link.ld
SRCS := $(C_SRCS) $(S_SRCS)
INC_PATHS += $(COMMON_SRC_PATH)
INC_H := $(wildcard $(addsuffix /*.h, $(INC_PATHS)))
INC := $(addprefix -I, $(INC_PATHS))
LIBS += -lgcc
USER_DEFINES =

# if not set in test's Makefile, default to all supported ISA extensions
#MARCH ?= rv32imc_zicsr_zifencei_zicntr
MARCH ?= rv32i_zmmul_zicsr_zifencei_zicntr
#MARCH ?= rv32i_zicsr
MABI ?= ilp32
GCC_OPTS += -march=$(MARCH) -mabi=$(MABI) -mcmodel=medany -static -fno-pic
GCC_OPTS += -nostdlib -nostartfiles -ffreestanding -g

# aligning code and data
CACHE_LINE_BYTES := 64
GCC_OPTS += -ffunction-sections -fdata-sections -msmall-data-limit=8
#GCC_OPTS += -falign-functions=$(CACHE_LINE_BYTES)
#GCC_OPTS += -falign-loops=$(CACHE_LINE_BYTES)
#GCC_OPTS += -falign-jumps=$(CACHE_LINE_BYTES)
#GCC_OPTS += -falign-labels=$(CACHE_LINE_BYTES)

#GCC_OPTS += -fvisibility=hidden
#GCC_OPTS += -save-temps
GCC_OPTS += -T $(LD_SRC) -Wl,--build-id=none
GCC_OPTS += -Wl,--gc-sections
GCC_OPTS += -Wl,--no-warn-rwx-segments
#GCC_OPTS += -Wp,-w # disable preprocessor warnings
#GCC_OPTS += -Wl,--noinhibit-exec
#GCC_OPTS += -Wl,-Map=$(TARGET).map
# optimizations not set - expected to come from test's Makefile

OBJDUMP_OPTS := -Dhxtw --show-all-symbols
OBJDUMP_OPTS += --start-address=0x40000 --stop-address=0x50000
OBJDUMP_OPTS += -Mnumeric -Mno-aliases # rv specific
#OBJDUMP_OPTS += --visualize-jumps
#OBJDUMP_OPTS += -S # inline C source code in disassembly
MEM_WIDTH := 32

# for tests that are parameterized by defines
define GET_DEFINES
$(wordlist 2,999,$(subst :, ,$(filter $1:%,$(TEST_DEFINES))))
endef

# run with HEX=1 to generate hex file for RTL simulation
HEX ?= 1
RV_GNU_DEFAULT := riscv64-unknown-elf
BIN2HEX :=
ifeq ($(HEX), 1)
	ifneq (, $(shell command -v $(RV_GNU)-bin2hex 2> /dev/null))
		BIN2HEX := $(RV_GNU)-bin2hex
	else ifneq (, $(shell command -v $(RV_GNU_DEFAULT)-bin2hex 2> /dev/null))
		BIN2HEX := $(RV_GNU_DEFAULT)-bin2hex
	else
		$(error "No bin2hex tool found")
	endif
endif

# define as variable so it's not broken up across lines for stdout
BUILD_APP = \
	$(Q)$(RV_GNU)-gcc $(GCC_OPTS) $(OPT) $(CFLAGS) $(call GET_DEFINES,$*) \
	$(INC) $(SRCS) $(COMMON_OBJS) -o $@ $(USER_DEFINES) $(LIBS)

HEX_GEN = \
	$(Q)if [ "$(HEX)" = "1" ]; then \
		$(BIN2HEX) -w $(MEM_WIDTH) $(basename $@).bin $(basename $@).hex; \
	fi

%.elf: $(SRCS) $(INC_H) $(CRT0) $(LD_SRC) $(COMMON_OBJS) $(APP_REQS)
	$(BUILD_APP)
	$(Q)$(RV_GNU)-objdump $(OBJDUMP_OPTS) $@ > $(basename $@).dump
	$(Q)$(RV_GNU)-objcopy $@ -O binary $(basename $@).bin
	$(HEX_GEN)
	@echo "Executable built: $@"
	$(Q)$(RV_GNU)-size -G $@

build_common: $(COMMON_OBJS)

COMMON_FLAGS := -Wall -Wextra -Wunreachable-code -Wnull-dereference -Wcast-qual
COMMON_FLAGS += -Werror -pedantic

BUILD_COMMON = \
	$(Q)$(RV_GNU)-gcc $(GCC_OPTS) $(COMMON_FLAGS) $(OPT) $(CFLAGS) -c $< -o $@ \
	$(USER_DEFINES)

$(COMMON_BUILD_DIR)/%.o: %.c %.h
	$(BUILD_COMMON)

$(COMMON_BUILD_DIR)/%.o: %.c
	$(BUILD_COMMON)

$(COMMON_BUILD_DIR)/%.o: %.S
	$(BUILD_COMMON)

clean:
	rm -f *.elf* *.dump *.hex *.bin *.o *.d *.i *.s *.map

clean_common:
	rm -rf $(COMMON_BUILD_DIR)

cleanall: clean clean_common

.PHONY: all build_common clean clean_common cleanall
