SHELL := /bin/bash

NPROCS_DEF := 8
NPROCS := $(shell nproc 2>/dev/null || echo $(NPROCS_DEF))
# only add -j if MAKEFLAGS doesn't already contain a job limit
ifeq ($(filter -j% --jobserver%, $(MAKEFLAGS)),)
    MAKEFLAGS += -j$(NPROCS)
endif

REPO_ROOT := $(shell git rev-parse --show-toplevel)

RV_GNU := $(RV_GNU_DEV)
# e.g. `export RV_GNU_DEV=/home/tools/rv_gcc_16/bin`
BIN2HEX := $(REPO_ROOT)/sw/bin2hex.py

GCC := $(RV_GNU)/riscv32-unknown-elf-gcc
CXX := $(RV_GNU)/riscv32-unknown-elf-g++
OBJDUMP := $(RV_GNU)/riscv32-unknown-elf-objdump
OBJCOPY := $(RV_GNU)/riscv32-unknown-elf-objcopy
SIZE := $(RV_GNU)/riscv32-unknown-elf-size
