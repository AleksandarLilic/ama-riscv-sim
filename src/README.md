# Building

```bash
make -j
```

For temporary/experimental build, options can be added before `make`ing through `USER_DEFINES` Makefile variable (optionally also setting `DEFINES` to empty string) or changed in the Makefile itself  

Options are passed to `gcc` so `make clean` might have to be run first (or force with `make -B`) recompile if no source code was changed

```bash
make clean # also deletes all logs
make -j USER_DEFINES=-DLOG_EXEC_ALL
```

Or
```bash
make -j USER_DEFINES=-DLOG_EXEC_ALL -B
```

## All build options

`-DLOG_EXEC` 
- Create a log during execution, saved as `<test_prefix>_exec.log`
- Each executed instruction is logged as `PC: INST` e.g. `8000001c: 00050023`

`-DENABLE_DASM` 
- Also disassemble insutrction on the fly and add to the log
- Requires `-DLOG_EXEC`
- Log entry becomes `8000001c: 00050023 sb zero,0(a0)`

`-DLOG_EXEC_ALL`
- Requires `-DLOG_EXEC`
- Logs entire state of the CPU
- Also logs result of the DMEM access for load/store instructions
- Log entry becomes:

```
8000001c: 00050023 sb zero,0(a0)
    zero (0x0) -> mem[80000310]
    PC: 8000001c
    zero: 0x00000000  ra  : 0x00000000  sp  : 0x80040000  gp  : 0x80000af0
    tp  : 0x00000000  t0  : 0x00000000  t1  : 0x00000000  t2  : 0x00000000
    s0  : 0x00000000  s1  : 0x00000000  a0  : 0x80000310  a1  : 0x80000350
    a2  : 0x00000000  a3  : 0x00000000  a4  : 0x00000000  a5  : 0x00000000
    a6  : 0x00000000  a7  : 0x00000000  s2  : 0x00000000  s3  : 0x00000000
    s4  : 0x00000000  s5  : 0x00000000  s6  : 0x00000000  s7  : 0x00000000
    s8  : 0x00000000  s9  : 0x00000000  s10 : 0x00000000  s11 : 0x00000000
    t3  : 0x00000000  t4  : 0x00000000  t5  : 0x00000000  t6  : 0x00000000
    0x0340 (mscratch): 0x0
    0x051e (tohost): 0x0
```

`-DUSE_ABI_NAMES`
- Use ABI names for disassembly and the end report (e.g. `zero` instead of `x0`, `sp` instead of `x2`, etc.)

`-DENABLE_PROF`
- Enable profiling of the execution
- Creates an instruction profile log from each executed instruction as `<test_prefix>_inst_profiler.json`, as shown in the [example below](#example-instruction-profile)
- Creates a binary log of each executed instruction with it's program counter and stack usage as `<test_prefix>_trace.bin`

`-DUART_ENABLE`
- Enable UART peripheral (only output to the `stdout`)

`-DUART_INPUT_ENABLE`
- Enable listening for input from the user
- Requires `-DUART_ENABLE`

`-DCHECK_LOG`
- Generate log used to compare against same program executed on Spike simulator

### Example instruction profile
Saved as `vector_mac_vm_uint8_inst_profiler.json`

``` json
{
"add": {"count": 5376},
"sub": {"count": 0},
"sll": {"count": 0},
"srl": {"count": 0},
"sra": {"count": 0},
"slt": {"count": 0},
"sltu": {"count": 0},
"xor": {"count": 0},
"or": {"count": 0},
"and": {"count": 0},
"nop": {"count": 5},
"addi": {"count": 5538},
"slli": {"count": 7168},
"srli": {"count": 7168},
"srai": {"count": 0},
"slti": {"count": 0},
"sltiu": {"count": 0},
"xori": {"count": 0},
"ori": {"count": 0},
"andi": {"count": 7248},
"lb": {"count": 0},
"lh": {"count": 0},
"lw": {"count": 1065},
"lbu": {"count": 2048},
"lhu": {"count": 0},
"sb": {"count": 64},
"sh": {"count": 0},
"sw": {"count": 1051},
"fence.i": {"count": 0},
"lui": {"count": 0},
"auipc": {"count": 67},
"ecall": {"count": 1},
"ebreak": {"count": 0},
"csrrw": {"count": 1},
"csrrs": {"count": 0},
"csrrc": {"count": 0},
"csrrwi": {"count": 0},
"csrrsi": {"count": 0},
"csrrci": {"count": 0},
"beq": {"count": 7184, "breakdown": {"taken": 2832, "taken_fwd": 2832, "taken_bwd": 0, "not_taken": 4352, "not_taken_fwd": 4352, "not_taken_bwd": 0}},
"bne": {"count": 8288, "breakdown": {"taken": 7197, "taken_fwd": 0, "taken_bwd": 7197, "not_taken": 1091, "not_taken_fwd": 0, "not_taken_bwd": 1091}},
"blt": {"count": 0, "breakdown": {"taken": 0, "taken_fwd": 0, "taken_bwd": 0, "not_taken": 0, "not_taken_fwd": 0, "not_taken_bwd": 0}},
"bge": {"count": 0, "breakdown": {"taken": 0, "taken_fwd": 0, "taken_bwd": 0, "not_taken": 0, "not_taken_fwd": 0, "not_taken_bwd": 0}},
"bltu": {"count": 0, "breakdown": {"taken": 0, "taken_fwd": 0, "taken_bwd": 0, "not_taken": 0, "not_taken_fwd": 0, "not_taken_bwd": 0}},
"bgeu": {"count": 65, "breakdown": {"taken": 1, "taken_fwd": 1, "taken_bwd": 0, "not_taken": 64, "not_taken_fwd": 64, "not_taken_bwd": 0}},
"jalr": {"count": 1025, "breakdown": {"taken": 1025, "taken_fwd": 1025, "taken_bwd": 0, "not_taken": 0, "not_taken_fwd": 0, "not_taken_bwd": 0}},
"jal": {"count": 1092, "breakdown": {"taken": 1092, "taken_fwd": 1, "taken_bwd": 1091, "not_taken": 0, "not_taken_fwd": 0, "not_taken_bwd": 0}},
"_max_sp_usage": 32,
"_profiled_instructions": 54454
}
```
