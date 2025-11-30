# SPDX-License-Identifier: Apache-2.0
# Copyright 2022-2025 Arm Limited

HEADERS = $(wildcard *.h)
SOURCES = $(wildcard *.c)
TARGETS = $(SOURCES:.c=)

CFLAGS = -std=c11 -O2 -g -Wall -pedantic
LINKER_FLAGS = -lm

ifeq ($(WERROR), 1)
	CFLAGS += -Werror
endif

# make CPU=NEOVERSE-V1 --> gcc -DCPU_NEOVERSE_V1
ifdef CPU
CPUINFO = $(shell echo $(CPU) | tr '[a-z\-]' '[A-Z_]')
else
CPUINFO = NONE
endif
CFLAGS += -DCPU_$(CPUINFO)

ifeq ($(USE_C), 1)
CFLAGS += -DUSE_C
endif

ifeq ($(OS),Windows_NT)
CC=clang
CFLAGS += --target=arm64-pc-windows-msvc
LINKER_FLAGS =

TMPVAR := $(TARGETS)
TARGETS = $(filter-out l1i_cache_workload, $(TMPVAR))

TMPVAR := $(TARGETS)
TARGETS = $(filter-out memcpy_workload, $(TMPVAR))

TARGETS_PDB = $(TARGETS:=.pdb)
TARGETS_ILK = $(TARGETS:=.ilk)
TARGETS_EXE = $(TARGETS:=.exe)
else
TARGETS_EXE = $(TARGETS)
endif
$(info $(TARGETS))
dummy:
	echo "CPU defined as: $(CPUINFO)"

.PHONY: clean all

all: $(TARGETS_EXE)

clean:
	rm -f $(TARGETS_EXE) $(TARGETS_PDB) $(TARGETS_ILK)

%: %.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(LINKER_FLAGS)

%.exe: %.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(LINKER_FLAGS)
