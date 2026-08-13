
MATH_LIB_FLAGS ?=
MATH_LIB_FLAGS_STR =

# select math library flavor

ifeq ("$(strip $(MATH_LIB_LOAD_OPT)) $(strip $(MATH_LIB_SIMD))", "1 1")
$(error MATH_LIB_LOAD_OPT and MATH_LIB_SIMD cannot be set at the same time)
endif

ifeq ($(strip $(MATH_LIB_LOAD_OPT)), 1)
MATH_LIB_FLAGS_STR = SCALAR_LOAD_OPT
MARCH := rv32im_zicsr_zifencei_zicntr
MATH_LIB_FLAGS += -DLOAD_OPT
else ifeq ($(strip $(MATH_LIB_SIMD)), 1)
MATH_LIB_FLAGS_STR = SIMD
MARCH := rv32im_zicsr_zifencei_zicntr_xsimd
else
MATH_LIB_FLAGS_STR = SCALAR
MARCH := rv32im_zicsr_zifencei_zicntr
endif

ifeq ($(strip $(MATH_LIB_UNROLL_DOTV)), 1)
MATH_LIB_FLAGS += -DM_UNROLL_DOTV
MATH_LIB_FLAGS_STR += UNROLLED
endif
