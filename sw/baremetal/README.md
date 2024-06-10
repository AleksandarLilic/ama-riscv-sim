# Test layout

All tests are self-checking: a reference value is provided, and the end result is checked against it. If test passes, `0x1` is written to `tohost` CSR. If test fails, index of the failed check, shifted left by 1, is written to `tohost` which is then `or`ed with `0x1`. Finally, `ecall` ends the test.

## Common files

Common files are put under [common](./common/) directory, and include:
1. `crt0.S` containing entry point which executes first
2. `link.ld` linker script
3. `common.h` and `common.c` for logic shared among tests
4. `mem_map.h` which describes memory map of the processor
5. `newlib_defs.c` containing stubs for `newlib`, if used

## Tests

Most tests follow the approach outliened above: a test input and reference value are provided, which then CPU executes and compares against, respectively.

All tests support increasing the execution duration through repeatedly looping over a single instance. Default is 1 and can be changed during make with to e.g. 5 with

``` bash
make LOOPS=5
```

Some tests also support parametrization with respect to input.

### Codegen
Two tests use provided code generation scripts to further parametrize test to use different operantion and/or number format

#### Test `vector_basic`

- Supports 4 vector operations: `add`, `sub`, `mul`, `div`  
- And 10 number formats for each: `uint8`, `int8`, `uint16`, `int16`, `uint32`, `int32`, `uint64`, `int64`, `float32`, `float64`  

Inputs and reference vectors are provided via 4 headers files (one for each operation) and are prepared with 
```
make headers
```

To make all 40 tests at once (which also prepares headers if needed)
```
make -j
```

#### Test `sorting`

- Supports 6 different sorting algorithms `bubble`, `insertion`, `selection`, `merge`, `quick`, `heap`  
- Total of 10 number formats for each: `uint8`, `int8`, `uint16`, `int16`, `uint32`, `int32`, `uint64`, `int64`, `float32`, `float64`  
- Across two predefined lengths: `small` and `large`

Inputs and reference vectors are provided via two header files (one for each length) and are prepared with 
```
make headers
```

To make all 120 tests at once (which also prepares header if needed)
```
make -j
```

Array lengths are defined in [codegen.py](./sorting/codegen.py) script (with two extra lengths if needed)

```python
LEN_MAP = {"tiny": 10, "small": 30, "medium": 100, "large": 400}
```

## Build all
All tests specified in `../../test/testlist.json` can be built with (which by default also builds ISA tests)

``` bash
cd ../../test
make prepare
```
## Toolchain

`gcc` settings might change from version to version. Makefiles are set up for `gcc-13`  

Set `RV_GNU_LATEST` to (prefix) of the binary name  
e.g. when binary is already in the PATH
```bash
export RV_GNU_LATEST=riscv64-unknown-elf
```

Or when built from source, and not in the PATH, e.g.
```bash
export RV_GNU_LATEST=/home/tools/rv_gcc_2024-05-15/bin/riscv32-unknown-elf
```
