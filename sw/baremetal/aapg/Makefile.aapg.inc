
AAPG_ARCH := rv32
MAKEFILE_INC := ../../Makefile.inc

SEED ?= 47

CODEGEN_LOG := codegen.log

LOG_AAPG ?= 1
ifeq ($(LOG_AAPG),1) # to file
LOG_AAPG_ARG := 2> $(CODEGEN_LOG)
else ifeq ($(LOG_AAPG),2) # to console
LOG_AAPG_ARG :=
else # sink
LOG_AAPG_ARG := 2> /dev/null
endif

OPT := -O2 -flto
CFLAGS := -I../

TARGETS := $(addsuffix .elf, $(TYPES))

all: $(TARGETS)

codegen: main.S
clean_codegen: clean_aapg

main.S: $(CONFIG_FILE)
	@aapg setup 2> /dev/null
	aapg gen --arch $(AAPG_ARCH) --config_file $(CONFIG_FILE) --seed $(SEED) --asm_name $(basename $@) $(LOG_AAPG_ARG)
	@mv work/asm/$(basename $@).S $@
	@sed -i 's/$(basename $@)_template.S/aapg_common.h/' $@
	@rm -rf work

clean_aapg:
	rm -rf main.S $(CODEGEN_LOG)

.PHONY: setup clean_aapg

include $(MAKEFILE_INC)
