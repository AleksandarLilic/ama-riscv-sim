
AAPG_ARCH := rv32
MAKEFILE_INC := ../../Makefile.inc

SEED ?= 47

OPT := -O2 -flto
CFLAGS := -I../

TARGETS := $(addsuffix .elf, $(TYPES))

all: $(TARGETS)

codegen: main.S
clean_codegen: clean_aapg

main.S: $(CONFIG_FILE)
	@aapg setup 2> /dev/null
	aapg gen --arch $(AAPG_ARCH) --config_file $(CONFIG_FILE) --seed $(SEED) --asm_name $(basename $@) 2> /dev/null
	@mv work/asm/$(basename $@).S $@
	@sed -i 's/$(basename $@)_template.S/aapg_common.h/' $@
	@rm -rf work

clean_aapg:
	rm -rf main.S

.PHONY: setup clean_aapg

include $(MAKEFILE_INC)
