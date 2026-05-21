# Baremetal tests

C and ASM programs targeting the RV32 core, used by both the ISA simulator and the RTL testbench. All programs are freestanding.

- [Baremetal tests](#baremetal-tests)
- [Pass/fail protocol](#passfail-protocol)
- [Common files](#common-files)
- [Build system](#build-system)
  - [Toolchain](#toolchain)
  - [Makefile.inc](#makefileinc)
- [Test organization](#test-organization)
  - [LOOPS](#loops)
  - [Multi-binary tests](#multi-binary-tests)
- [Python codegen flow](#python-codegen-flow)
  - [`codegen_common.py`](#codegen_commonpy)
  - [Per-test `codegen.py`](#per-test-codegenpy)
- [AAPG flow](#aapg-flow)
  - [Directory layout](#directory-layout)
  - [Variant configuration](#variant-configuration)
  - [Generating variant directories](#generating-variant-directories)
  - [Building a variant](#building-a-variant)
- [Building all tests](#building-all-tests)

# Pass/fail protocol

Most of the tests are self-checking: a reference value is provided, and the end result is compared against it. Setting `tohost` LSB ends simulation
If test passes, `tohost[31:1]` is set to `0x0`. If test fails, `tohost[31:1]` is set to index of the failed check/test.

# Common files

Shared startup and support code lives under [`common/`](./common/):

| File | Purpose |
|------|---------|
| `crt0.S` | Entry point; sets up `gp`/`sp`, clears BSS, optional trap vector |
| `link.ld` | Linker script; RAM at `0x80000000`, 128 KB |
| `mem_map.h` | Memory map constants |
| `csr.h` | CSR access helpers |
| `common.{c,h}` | Shared utility functions |
| `common_math.{c,h}` | Shared math utilities, including custom SIMD ISA wrappers |
| `c_test_common.h` | Macros for C test assertions |
| `mini-printf.{c,h}` | Embedded printf |
| `memset.S` | Optimized memset |
| `newlib_defs.c` | Newlib syscall stubs, when newlib is used |
| `arena_alloc.h` | Simple arena allocator |
| `asm_test.S` | Shared infrastructure for ASM tests |

# Build system

## Toolchain

Makefiles are set up for `gcc-14`. Set `RV_GNU_LATEST` to the prefix of the RISC-V toolchain binary, e.g. when already in `PATH`:

```sh
export RV_GNU_LATEST=riscv64-unknown-elf
```

Or when built from source and not in `PATH`:

```sh
export RV_GNU_LATEST=/home/tools/rv_gcc_2024-05-15/bin/riscv32-unknown-elf
```

`bin2hex.py` (`sw/bin2hex.py`) is used for generating hex files needed for loading the app into the memory in the RTL environment.

## Makefile.inc

[`Makefile.inc`](./Makefile.inc) contains the shared build rules. Every per-test `Makefile` ends with:

```makefile
include ../Makefile.inc
```

`Makefile.inc` handles source discovery, compilation, linking, disassembly, and hex generation. Key behaviors:

- **Source discovery**: picks up all `*.c` and `*.S` files in the test directory automatically, unless `EXPLICIT_SRCS=1` is set, in which case the test Makefile is responsible for setting `C_SRCS` / `S_SRCS`
- **Common objects**: compiles and caches `crt0`, `memset`, `common`, `common_math`, `mini-printf` into `./common_build/`
- **Outputs per target**: `.elf`, `.dasm` (full objdump), `.mem` (128-bit wide hex, via `bin2hex.py`; `.mem` extension required by Xilinx `updatemem` for bitstream patching); hex generation is on by default (`HEX=1`) and can be disabled with `HEX=0`
- **Verbosity**: `V=1` on the make command line enables full command echo

Key variables a per-test `Makefile` may set before the `include`:

| Variable | Purpose | Default |
|----------|---------|---------|
| `TARGET` / `TARGETS` | Output ELF name(s) without extension | *none* |
| `OPT` | Optimization flags | *none* |
| `MARCH` | ISA string | `rv32im_zicsr_zifencei_zicntr_xsimd` |
| `MABI` | ABI | `ilp32` |
| `CFLAGS` | Extra compiler flags | *none* |
| `LOOPS` | Loop count passed as `-DLOOPS=N` | `1` |
| `USER_DEFINES` | Extra `-D` flags | *none* |
| `EXPLICIT_SRCS` | Disable auto source discovery | `0` |
| `APP_REQS` | Additional prerequisites for the `.elf` rule | *none* |
| `TEST_DEFINES` | Per-target define sets for multi-binary tests | *none* |

Available make targets from `Makefile.inc`:

| Target | Action |
|--------|--------|
| `build_common` | Compile common objects only |
| `clean` | Remove build artifacts in the test directory |
| `clean_common` | Remove `./common_build/` |
| `cleanall` | `clean` + `clean_common` |

A minimal per-test `Makefile` looks like this (from `fibonacci/`):

```makefile
N_IN := 18
TARGET := n_$(N_IN)
CFLAGS += -DN_IN=$(N_IN)
GCC_OPTS += -O3 -flto
all: $(TARGET).elf
include ../Makefile.inc
```

# Test organization

Each subdirectory is a self-contained test or test family. Most produce a single named `.elf`; some produce multiple binaries from one source tree via `TEST_DEFINES` (see below).

Test categories:

- **Hand-written ASM**: `asm_rv32i*`, `asm_rv32im`, `asm_rv32ic` - instruction-level correctness
- **C functional**: `fibonacci`, `factorial`, `gcd_lcm`, `prime_numbers`, `rd_pairing`, etc.
- **Memory / cache**: `dcache_*`, `memcpy`, `stream_int`
- **Numeric / SIMD**: `vector_ew_*`, `matmul*`, `dot_product*`, `conv1d`, `sorting_*`
- **Benchmarks**: `dhrystone`, `coremark`, `embench`, `ustress`
- **Peripherals / interrupts**: `uart_*`, `timer_interrupt`
- **ISA compliance**: `imperas-riscv-tests`
- **Random generated**: `aapg/aapg_rv32_*` (see [AAPG flow](#aapg-flow))

## LOOPS

All tests accept a `LOOPS` variable to extend execution time by repeating the workload:

```sh
make LOOPS=5
```

Default is `1`.

## Multi-binary tests

Some tests build many ELFs from a single source by compiling it repeatedly with different preprocessor defines. This is done via the `TEST_DEFINES` variable in the test Makefile, with entries of the form `<target_stem>:<define1>:<define2>:...`.

`vector_ew_basic` is the canonical example: it combines 4 operations (`add`, `sub`, `mul`, `div`) with 10 numeric types (`uint8`, `int8`, `uint16`, `int16`, `uint32`, `int32`, `uint64`, `int64`, `float32`, `float64`), producing 40 ELF targets. Each is built with a unique `-DNF_<TYPE>` and `-DOP_<OP>` pair selected from `TEST_DEFINES`. Building in parallel:

```sh
make -j
```

# Python codegen flow

Several tests generate their input data and reference values offline, before compilation. This avoids large hard-coded arrays in the source and allows parameterization across types and sizes without duplicating C code.

## `codegen_common.py`

[`codegen_common.py`](./codegen_common.py) at the root of `baremetal/` provides shared helpers imported by per-test codegen scripts:

- `NUM` - dict mapping C type strings (e.g. `"uint8_t"`, `"float"`) to NumPy dtypes and per-operation offsets used to avoid overflow when generating random data
- `np2c_1d_arr` / `np2c_2d_arr` - format NumPy arrays as C array initializers
- `finish_gen()` - write generated code to a header file on disk

## Per-test `codegen.py`

Each test that needs generated data has its own `codegen.py`. It imports `codegen_common`, generates random inputs and golden references using NumPy, and writes one or more `#ifdef NF_<TYPE>` / `#ifdef OP_<OP>` guarded headers. The test C source includes the header and uses the preprocessor defines set by `TEST_DEFINES` to select the active path at compile time.

Workflow using `vector_ew_basic` as an example:

```sh
cd vector_ew_basic
make headers        # runs codegen.py -> writes test_arrays_{add,sub,mul,div}.h
make -j             # compiles all 40 targets (runs headers first if missing)
```

Generated headers are not under version control and are recreated on demand. Other tests with their own `codegen.py`: `sorting_num`, `sorting_str`, `matmul`, `memcpy`, `dcache_ds_*`.

# AAPG flow

[AAPG](https://gitlab.com/shaktiproject/tools/aapg) (Automated Assembly Program Generator) generates random RV32 assembly programs from a YAML configuration. The `aapg/` subdirectory manages multiple named variants and their build infrastructure.

`aapg.patch` must be applied to the upstream AAPG source before using it for custom SIMD ISA (see `sim/README.md` for details).

## Directory layout

```
aapg/
  config_meta.yaml      # defines all variants and their AAPG config overrides
  config_gen.py         # generates per-variant directories from config_meta.yaml
  template/             # baseline config.yaml + Makefile copied to each variant
  Makefile.aapg.inc     # shared make rules included by every variant Makefile
  aapg.patch            # patch against upstream AAPG
  aapg_common.h         # common header included by generated assembly
  aapg_rv32_*/          # one directory per variant (generated by config_gen.py)
```

## Variant configuration

`config_meta.yaml` has one top-level entry per variant. Each entry lists only the fields that differ from `template/config.yaml` - typically `total_instructions` and ISA distribution weights such as `rel_rv32i.compute`, `rel_rv32i.data`, `rel_rv32i.ctrl`, `rel_rv32m`, and custom-SIMD groups. The variant name becomes the directory suffix (e.g. `rv32_generic` -> `aapg_rv32_generic/`).

## Generating variant directories

```sh
cd aapg
python3 config_gen.py                   # recreate all aapg_rv32_*/ directories
python3 config_gen.py -f rv32_mem       # filter by name (regex)
python3 config_gen.py -m                # generate directories and build
```

`config_gen.py` copies `template/` to `aapg_<name>/` and merges the overrides into `config.yaml`. Existing directories are removed and recreated.

## Building a variant

```sh
cd aapg/aapg_rv32_generic
make                   # codegen with default SEED=47, then build -> test_s47.elf/.hex
make SEED=123          # different seed -> test_s123.elf/.hex
make codegen           # only generate main_s<SEED>.S, skip compilation
make build             # compile a previously generated main_s<SEED>.S
```

`Makefile.aapg.inc` runs `aapg setup` + `aapg gen`, moves the output `.S` from `work/asm/` to `main_s<SEED>.S`, removes the `work/` tree, and then compiles via `../../Makefile.inc`. Each seed produces a distinct random program from the same configuration profile. `prepare_riscv_tests.py` accepts `--aapg_seeds` to build multiple seeds per variant.

# Building all tests

From the `sim/` root:

```sh
cd test
make prepare
```

`testlist.json` maps each test directory to its make targets and optional extra make variables. `prepare_riscv_tests.py` compiles the common objects once (`make build_common`), then builds every listed test. For tests with a codegen step (`aapg`, `vector_ew_basic`, `sorting_*`, `matmul`, `memcpy`) it runs `make clean_codegen` before regenerating, so artifacts from a previous run do not persist.
