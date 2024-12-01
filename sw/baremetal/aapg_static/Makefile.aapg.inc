
AAPG_ARCH := rv32
MAKEFILE_INC := ../../Makefile.inc

SEED ?= 47

FLAGS := -I../
export CFLAGS
TARGET :=
export TARGET

TARGETS := $(addsuffix .elf, $(TYPES))

all: setup main.S clean_setup $(TARGETS)

%.elf: main.S
	$(eval TARGET := $(basename $@))
	$(eval CFLAGS := $(FLAGS))
	@echo Building $@ with $(CFLAGS)
	@$(MAKE) -f $(MAKEFILE_INC) --no-print-directory

main.S: $(CONFIG_FILE)
	aapg gen --arch $(AAPG_ARCH) --config_file $(CONFIG_FILE) --seed $(SEED) --asm_name $(basename $@) 2> /dev/null
	@mv work/asm/$(basename $@).S $@
	@sed -i 's/$(basename $@)_template.S/aapg_common.h/' $@

setup:
	@aapg setup 2> /dev/null

clean_src:
	rm -rf main.S

clean_setup:
	rm -rf work

clean_aapg: clean_src
	@$(MAKE) -f $(MAKEFILE_INC) clean --no-print-directory

.PHONY: setup clean_setup clean_src clean_aapg

include $(MAKEFILE_INC)
