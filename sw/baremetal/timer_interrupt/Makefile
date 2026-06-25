TYPES := idling wfi wfi_no_global
NAME := test

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(NAME):-" "$(TYPES):TYPE" > $@

GCC_OPTS += -O1 -flto

all: $(TARGETS)

include ../Makefile.inc
