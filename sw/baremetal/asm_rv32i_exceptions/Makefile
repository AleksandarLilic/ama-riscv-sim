TYPES := trap_ecall trap_ebreak trap_unsupported \
		 trap_load_access_fault  trap_store_access_fault \
		 trap_load_address_misaligned trap_store_address_misaligned \
		 trap_instruction_address_misaligned_jump \
		 trap_instruction_address_misaligned_branch

TARGETS := $(addsuffix .elf, $(TYPES))

TEST_DEFINES := $(foreach t,$(TYPES), $(t):-D$(shell echo $(t) | tr a-z A-Z))

MARCH := rv32i_zicsr
OPT := -O1 -flto

all: $(TARGETS)

include ../Makefile.inc
