NF = "uint8"
OP = "add"
TARGET := $(OP)_$(NF)
CRT0 := ../crt0.S
NF_DEF = $(shell echo $(NF) | tr '[:lower:]' '[:upper:]')
OP_DEF = $(shell echo $(OP) | tr '[:lower:]' '[:upper:]')
GCC_OPTS += -O0 -DNF_$(NF_DEF) -DOP_$(OP_DEF)

include ../Makefile
