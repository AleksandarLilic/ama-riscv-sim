# shared ISA sim source/header/object lists

# set ISA_SIM_SRC_ROOT and ISA_SIM_OBJ_ROOT before including
# when this file is used outside src/Makefile
ISA_SIM_SRC_ROOT ?= .
ISA_SIM_OBJ_ROOT ?= build

ISA_SIM_SRC_ROOT_N := $(patsubst %/,%,$(ISA_SIM_SRC_ROOT))
ifeq ($(ISA_SIM_SRC_ROOT_N),.)
ISA_SIM_SRC_PREFIX :=
else
ISA_SIM_SRC_PREFIX := $(ISA_SIM_SRC_ROOT_N)/
endif

ISA_SIM_SRCS := $(wildcard $(ISA_SIM_SRC_PREFIX)*.cpp)
ISA_SIM_SRCS += $(wildcard $(ISA_SIM_SRC_PREFIX)devices/*.cpp)
ISA_SIM_SRCS += $(wildcard $(ISA_SIM_SRC_PREFIX)profilers/*.cpp)
ISA_SIM_SRCS += $(wildcard $(ISA_SIM_SRC_PREFIX)hw_models/*.cpp)

# drop disabled features' sources so their objects are never built/linked
ifeq ($(RV32C), 0)
ISA_SIM_SRCS := $(filter-out \
    $(ISA_SIM_SRC_PREFIX)core_exec_c.cpp, \
    $(ISA_SIM_SRCS))
endif
ifeq ($(SIMD), 0)
ISA_SIM_SRCS := $(filter-out \
    $(ISA_SIM_SRC_PREFIX)core_exec_custom_simd_%.cpp \
    $(ISA_SIM_SRC_PREFIX)dasm_simd.cpp, \
    $(ISA_SIM_SRCS))
endif

ISA_SIM_OBJS := $(patsubst \
    $(ISA_SIM_SRC_PREFIX)%.cpp, \
    $(ISA_SIM_OBJ_ROOT)/%.o, \
    $(ISA_SIM_SRCS))
ISA_SIM_DEPS := $(patsubst %.o, %.d, $(ISA_SIM_OBJS))

# hw_models unsupported for cosim/DPI build
ISA_SIM_COSIM_SRCS := $(filter-out \
    $(ISA_SIM_SRC_PREFIX)hw_models/%.cpp \
    $(ISA_SIM_SRC_PREFIX)main.cpp, \
    $(ISA_SIM_SRCS))
ISA_SIM_COSIM_OBJS := $(patsubst \
    $(ISA_SIM_SRC_PREFIX)%.cpp, \
    $(ISA_SIM_OBJ_ROOT)/%.o, \
    $(ISA_SIM_COSIM_SRCS))

ISA_SIM_H := $(wildcard $(ISA_SIM_SRC_PREFIX)*.h)
ISA_SIM_H += $(wildcard $(ISA_SIM_SRC_PREFIX)devices/*.h)
ISA_SIM_H += $(wildcard $(ISA_SIM_SRC_PREFIX)profilers/*.h)
ISA_SIM_H += $(wildcard $(ISA_SIM_SRC_PREFIX)hw_models/*.h)

ISA_SIM_INC := -I$(ISA_SIM_SRC_ROOT_N)
ISA_SIM_INC += -I$(ISA_SIM_SRC_ROOT_N)/devices
ISA_SIM_INC += -I$(ISA_SIM_SRC_ROOT_N)/profilers
ISA_SIM_INC += -I$(ISA_SIM_SRC_ROOT_N)/hw_models
ISA_SIM_INC += -I$(ISA_SIM_OBJ_ROOT)
ISA_SIM_INC += -I$(ISA_SIM_SRC_ROOT_N)/external/cxxopts/include
# because it goes haywire on old-style-cast
ISA_SIM_INC += -isystem $(ISA_SIM_SRC_ROOT_N)/external/ELFIO
