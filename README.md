# RISC-V ISA simulator

C++ Instruction Set Simulator for RISC-V RV32IMC & custom SIMD instructions with cache and branch predictor models, C/ASM workloads, and Python analysis tools

- [RISC-V ISA simulator](#risc-v-isa-simulator)
- [Getting the project](#getting-the-project)
  - [Quick start](#quick-start)
- [Overview](#overview)
  - [Usage](#usage)
- [Example use-case: Dhrystone](#example-use-case-dhrystone)
  - [Running Dhrystone](#running-dhrystone)
    - [Notes on profiling](#notes-on-profiling)
  - [Execution log](#execution-log)
  - [Callstack](#callstack)
  - [Profiled instructions](#profiled-instructions)
  - [Execution trace and register file usage](#execution-trace-and-register-file-usage)
  - [Hardware models outputs](#hardware-models-outputs)
  - [Analysis scripts](#analysis-scripts)
    - [Flat profile](#flat-profile)
    - [FlameGraph](#flamegraph)
    - [Call Graph](#call-graph)
    - [Execution visualization](#execution-visualization)
    - [Hardware performance estimates](#hardware-performance-estimates)
- [Hardware model sweeps](#hardware-model-sweeps)
  - [Caches](#caches)
  - [Branch predictors](#branch-predictors)
- [Building RISC-V programs](#building-risc-v-programs)
  - [Baremetal](#baremetal)
    - [Benchmarks](#benchmarks)
    - [Quantized Neural Network](#quantized-neural-network)
  - [RISC-V ISA tests](#risc-v-isa-tests)
- [GTest](#gtest)
- [Building the Simulator](#building-the-simulator)
- [Custom packed SIMD ISA](#custom-packed-simd-isa)
  - [Instruction list](#instruction-list)
  - [Instruction details](#instruction-details)
  - [Patching `binutils` to add support for custom instructions](#patching-binutils-to-add-support-for-custom-instructions)
  - [Patching `GCC` to add support for custom instructions](#patching-gcc-to-add-support-for-custom-instructions)


# Getting the project
Project relies on a few external libraries and tools. Clone recursively with
```sh
git clone --recurse-submodules git@github.com:AleksandarLilic/ama-riscv-sim.git
```

## Quick start
To check that everything is available and working as expected:
1. build a single test
2. build the simulator
3. run the test and check the logs

``` sh
# build test
cd sw/baremetal/dhrystone
make DHRY_ITERS=1000 # override number of iterations for testing only
cd -
# build ISA sim
cd src
make -j8
# optionally, use non-default gcc
# make CXX=g++-12 -j8
# create a separate workdir
cd .. && mkdir workdir && cd workdir
# run test, profile from the beginning, log executed instructions
../src/build/ama-riscv-sim ../sw/baremetal/dhrystone/dhrystone.elf --prof_pc_start 0x40000 -tl
# check the execution log
vim dhrystone_dhrystone_out/exec.log
```

Simulator `stdout`
```
Running: ../sw/baremetal/dhrystone/dhrystone.elf
Profiling start PC: 0x40000
Logging enabled

SIMULATION STARTED
=== UART START ===
...
=== UART END ===
SIMULATION FINISHED

Instruction Counters: executed: 521118, profiled: 521118
    0x051e tohost       : 0x00000001
Profiler - Inst:
    All: 521118
    Control: B: 63985(12.28%), JAL: 21746(4.17%), JALR: 22705(4.36%)
    Memory: MEM: 166065(31.87%) - L/S: 87869/78196(16.86%/15.01%)
    Compute: ALU: 245609(47.13%), MUL: 1000(0.19%), DIV: 0(0.00%)
    Bitmanip: Zbb: 0(0.00%)
    SIMD arith: ALU: 0(0.00%), WMUL: 0(0.00%), DOT: 0(0.00%)
    SIMD data fmt: WIDEN: 0(0.00%), NARROW: 0(0.00%), TXP: 0(0.00%)
    SIMD vector-scalar: DUP: 0(0.00%), VINS: 0(0.00%), VEXT: 0(0.00%)
    Hint: SCP: 0(0.00%)
    Other: NOP: 0(0.00%), Misc: 8(0.00%)
Profiler - Sparsity:
    ANY: 355615/20719(5.83%)
    ALU: 238689/16132(6.76%)
    MEM_L: 87869/4583(5.22%)
    MEM_S: 78196/0(0.00%)
    SIMD_DOT: 0/0(0.00%)
    SIMD_ALU: 0/0(0.00%)
    SIMD_DATA_FMT: 0/0(0.00%)
Profiler - Stack:
    Peak usage: 368 B
    Accesses: 80424(48.43%) - L/S: 40458/39966(24.36%/24.07%)
Profiler - Perf:
    Event: inst, Samples: 521118
Profiler - Fusion:
    LEA opportunities: 1000
Branch stats:
    Unique branches: 110
bpred
    bimodal (12 B): P: 55284, M: 8701, ACC: 86.40%, MPKI: 16.70
icache (S/W: 32/2, D/T/M: 4096/41/33 B): 
    Ref: 529819, H: 526687(526687/0), M: 3132(3132/0), R: 3068, HR: 99.41%; CT (R/W): core 2.0/0.0 MB, mem 195.8/0.0 KB
dcache (S/W: 16/4, D/T/M: 4096/49/41 B): 
    Ref: 162717, H: 162511(86166/76345), M: 206(29/177), R: 142, WB: 142, HR: 99.87%; CT (R/W): core 297/279 KB, mem 12.9/8.9 KB

Simulation performance: 0.42 MIPS (521118 instructions in 1.24s)
```

Outputs:
``` sh
ls dhrystone_dhrystone_out
callstack_folded_inst.txt  exec.log  hw_stats.json  inst_profile.json  trace.bin  uart.log
```

# Overview

Some simulator options depend on the build switches. This doc assumes default build switches which is the standalone mode with all available features included

Core functionalities:
1. Standalone ISA simulator capable of running applications targeting `RV32IMC_zicsr_zifencei_zicntr_zihpm_xsimd` ([rv drom](https://rv.drom.io/?RV32IMC_zicsr_zifencei_zicntr_zihpm_xsimd)) 
2. API for single step execution, aimed at DPI verification environments

Profilers:
1. Records the folded callstack based on the user-specified event source (`callstack_folded_inst.txt`)
2. Records breakdown of executed instruction (`inst_profile.json`)
3. Provides stack usage stats (`inst_profile.json` and `stdout`)
4. Records execution trace (`trace.bin`)
5. Records register file usage (`rf_usage.bin`)
6. Provides sparsity check for the custom SIMD extension (`stdout`)
7. Can be run in the timed environment (e.g. DPI) and use its time source for instruction cycles and callstack profiler source

Logging:
1. Records each executed instruction, the current callstack, and registers & memory locations changed by the instruction (`exec.log`)
2. Optionally records entire architectural state after each executed instruction
3. Optionally records the entire execution (from reset to the end of simulation), instead of just the profiling range

Hardware models:
1. Provides L1I and L1D caches as both statistical (metadata only) or functional (with data storage) models.
    1. number of sets - parametrizable from the CLI
    2. number of ways - parametrizable from the CLI
    3. replacement policy - only LRU is available
2. Records separate cache stats for the user defined region of interest (ROI)
3. Provides various branch predictor models
    1. The user specified one is completely configurable from the CLI
    2. The rest are hardcoded (provides variety), and can optionally be run (in parallel)
    3. Any number of branch predictors can be run in parallel, but only the user specified one will drive the L1I cache - the 'active' predictor
4. Provides branch stats for the number of unique branches (`stdout`) and performance of each of the predictors for the given unique branch (`branches.csv`)
5. Records runtime hardware statistics in the same region as the profilers (`hw_stats.json` and `stdout`)

Analysis scripts:
1. FlameGraphs
2. Detailed execution visualization
3. Performance estimates
4. Hardware models parameters sweep

Emulator is used through DPI for verification of the SystemVerilog implementation: [ama-riscv](https://github.com/AleksandarLilic/ama-riscv)

## Usage
The only required user argument is a path to the RISC-V executable, every other argument can use its default value

Full usage available in [examples/ama-riscv-sim.help](examples/ama-riscv-sim.help)

# Example use-case: Dhrystone
Example use-case which includes all generated log files from the simulator and the applicable analysis outputs are available under [examples/dhrystone_dhrystone_out](./examples/dhrystone_dhrystone_out). The `stdout` redirected to a file is also available

The following paragraphs will go into detail about each of the logs, analysis, and visualization

ISA sim and Dhrystone are assumed to have been built as described in the [Quick start](#quick-start)

## Running Dhrystone
To generate all available outputs, run
```sh
../src/build/ama-riscv-sim ../sw/baremetal/dhrystone/dhrystone.elf --prof_pc_start 0x40000 -tl --rf_usage --bp_dump_csv
```

Outputs:
``` sh
ls dhrystone_dhrystone_out
branches.csv  callstack_folded_inst.txt  exec.log  hw_stats.json  inst_profile.json  rf_usage.bin  trace.bin  uart.log
```
### Notes on profiling
A more common way of profiling is to only focus on one part of the workload at the time. In case of Dhrystone, the following will profile only a single loop, 500th iteration

``` sh
../src/build/ama-riscv-sim ../sw/baremetal/dhrystone/dhrystone.elf --prof_pc_start 41570 --prof_pc_stop 41644 --prof_pc_single_match 500
```
Dropping `--prof_pc_single_match 500` would profile all iterations

Cache and branch predictor models are still running while profiling is inactive, but don't record any stats

Another useful option when analyzing cache behavior is to profile a specific memory region. For example, `mlp` benefits from having high hit rate for the input image in the first layer, and its hit rate can be analyzed with

```sh
../src/build/ama-riscv-sim ../sw/baremetal/mlp/w8a8.elf --roi_start 0x00017900 --roi_size 256
```
After the simulations finishes, the `stdout` has one more line with ROI stats:
```
...
ROI: (0x17900 - 0x17a00): R: 4100, H: 4096, M: 4, E: 4, WB: 0, HR: 99.90%; CT (R/W): core 16.0/0.0 KB, mem 256.0/0.0 B
...
```

Memory location of the image might not always be at this address. Search the `*.dasm` for `input_img_0` symbol start.

Perf events can also be changed, which can help when analyzing e.g. tricky branch prediction section (`--perf_event bp_mispredict`), or heavy traffic and cache misses (`--perf_event dcache_miss`). Complete list of events is listed in the [Usage](#usage) chapter.

## Execution log
Execution log, saved as `exec.log`, logs instructions as they are executed. Compact version (default options) is saved and it  includes:
- the current callstack
- instruction count during profiling (omitted when profiling is inactive)
- PC
- Instruction and its disassembly
- modified register(s), memory locations, CSRs (instruction dependent)


Snippets taken from the [examples/dhrystone_dhrystone_out/exec.log](./examples/dhrystone_dhrystone_out/exec.log)
```
_start;
         1: 40000: 00000093 addi x1,x0,0                  x1 : 0x00000000  
         2: 40004: 00000113 addi x2,x0,0                  x2 : 0x00000000  
         3: 40008: 00000193 addi x3,x0,0                  x3 : 0x00000000  
...
        33: 40080: 07028293 addi x5,x5,112                x5 : 0x000400ec  
        34: 40084: 30529073 csrrw x0,mtvec,x5             0x0305 mtvec        : 0x000400ec
        35: 40088: 00005197 auipc x3, 0x5                 x3 : 0x00045088
...
clear_bss_w_loop;
        47: 400b8: 00052023 sw x0,0(x10)                  mem[4412c] <- x0 (0x00000000)
        48: 400bc: 00450513 addi x10,x10,4                x10: 0x00044130  
        49: 400c0: fff68693 addi x13,x13,-1               x13: 0x00000a5d  
        50: 400c4: fe069ae3 bne x13,x0,400b8
...
call_main;
     10664: 400dc: 5a1020ef jal x1,42e7c                  x1 : 0x000400e0
call_main;__libc_init_array;
     10665: 42e7c: ff010113 addi x2,x2,-16                x2 : 0x0004fff0
     10666: 42e80: 00812423 sw x8,8(x2)                   mem[4fff8] <- x8 (0x00000000)
     10667: 42e84: 01212023 sw x18,0(x2)                  mem[4fff0] <- x18 (0x00000000)
     10668: 42e88: 00000793 addi x15,x0,0                 x15: 0x00000000
     10669: 42e8c: 00000713 addi x14,x0,0                 x14: 0x00000000
     10670: 42e90: 00112623 sw x1,12(x2)                  mem[4fffc] <- x1 (0x000400e0)
     10671: 42e94: 00912223 sw x9,4(x2)                   mem[4fff4] <- x9 (0x00000000)
     10672: 42e98: 40e78933 sub x18,x15,x14               x18: 0x00000000
     10673: 42e9c: 02e78263 beq x15,x14,42ec0
     10674: 42ec0: 00000793 addi x15,x0,0                 x15: 0x00000000
     10675: 42ec4: 00000713 addi x14,x0,0                 x14: 0x00000000
     10676: 42ec8: 40e78933 sub x18,x15,x14               x18: 0x00000000
     10677: 42ecc: 40295913 srai x18,x18,0x2              x18: 0x00000000
     10678: 42ed0: 02e78063 beq x15,x14,42ef0
     10679: 42ef0: 00c12083 lw x1,12(x2)                  x1 : 0x000400e0 <- mem[4fffc]
     10680: 42ef4: 00812403 lw x8,8(x2)                   x8 : 0x00000000 <- mem[4fff8]
     10681: 42ef8: 00412483 lw x9,4(x2)                   x9 : 0x00000000 <- mem[4fff4]
     10682: 42efc: 00012903 lw x18,0(x2)                  x18: 0x00000000 <- mem[4fff0]
     10683: 42f00: 01010113 addi x2,x2,16                 x2 : 0x00050000
     10684: 42f04: 00008067 jalr x0,0(x1) # ret
call_main;
     10685: 400e0: 398010ef jal x1,41478                  x1 : 0x000400e4
call_main;main;
     10686: 41478: f9010113 addi x2,x2,-112               x2 : 0x0004ff90
     10687: 4147c: 03000513 addi x10,x0,48                x10: 0x00000030
     10688: 41480: 06112623 sw x1,108(x2)                 mem[4fffc] <- x1 (0x000400e4)
     10689: 41484: 06812423 sw x8,104(x2)                 mem[4fff8] <- x8 (0x00000000)
     10690: 41488: 06912223 sw x9,100(x2)                 mem[4fff4] <- x9 (0x00000000)
     10691: 4148c: 07212023 sw x18,96(x2)                 mem[4fff0] <- x18 (0x00000000)
     10692: 41490: 05312e23 sw x19,92(x2)                 mem[4ffec] <- x19 (0x00000000)
     10693: 41494: 05412c23 sw x20,88(x2)                 mem[4ffe8] <- x20 (0x00000000)
     10694: 41498: 05512a23 sw x21,84(x2)                 mem[4ffe4] <- x21 (0x00000000)
     10695: 4149c: 05612823 sw x22,80(x2)                 mem[4ffe0] <- x22 (0x00000000)
     10696: 414a0: 15c010ef jal x1,425fc                  x1 : 0x000414a4  
call_main;main;malloc;
     10697: 425fc: 00050593 addi x11,x10,0                x11: 0x00000030  
     10698: 42600: 8101a503 lw x10,-2032(x3)              x10: 0x00043ff8 <- mem[44128]
     10699: 42604: 0040006f jal x0,42608
call_main;main;_malloc_r;
     10700: 42608: fd010113 addi x2,x2,-48                x2 : 0x0004ff60
     10701: 4260c: 01312e23 sw x19,28(x2)                 mem[4ff7c] <- x19 (0x00000000)
...
```

A detailed version can be generated by adding `--log_state`. This will include the entire architectural state after each executed instruction, and all CSRs when any one is modified. It's indented such that it can be completely folded inside the code editor. When folded, it's as compact as the version without state log. Note that this will significantly increase runtime and log size.  

```
_start;  
         1: 40000: 00000093 addi x1,x0,0                  x1 : 0x00000000  
            PC: 40000
            x0 : 0x00000000   x1 : 0x00000000   x2 : 0x00c0ffee   x3 : 0x00c0ffee   
            x4 : 0x00c0ffee   x5 : 0x00c0ffee   x6 : 0x00c0ffee   x7 : 0x00c0ffee   
            x8 : 0x00c0ffee   x9 : 0x00c0ffee   x10: 0x00c0ffee   x11: 0x00c0ffee   
            x12: 0x00c0ffee   x13: 0x00c0ffee   x14: 0x00c0ffee   x15: 0x00c0ffee   
            x16: 0x00c0ffee   x17: 0x00c0ffee   x18: 0x00c0ffee   x19: 0x00c0ffee   
            x20: 0x00c0ffee   x21: 0x00c0ffee   x22: 0x00c0ffee   x23: 0x00c0ffee   
            x24: 0x00c0ffee   x25: 0x00c0ffee   x26: 0x00c0ffee   x27: 0x00c0ffee   
            x28: 0x00c0ffee   x29: 0x00c0ffee   x30: 0x00c0ffee   x31: 0x00c0ffee
...
```

## Callstack
Folded callstack is saved as `callstack_folded_inst.txt` and is ready to be used with [script/FlameGraph/flamegraph.pl](./script/FlameGraph/flamegraph.pl). If other perf events are used, the output will be saved under the appropriate name, e.g. `callstack_folded_bp_mispredict.txt`

Example snippet from the folded callstack
```
...
call_main;main;__udivsi3; 22000       
call_main;main;__divsi3; 2000
call_main;main;Proc_1;Proc_6; 20000
call_main;main;Proc_1;Proc_7; 4000
call_main;main;Proc_8; 25000
call_main;main;Proc_7; 8000
...
```

## Profiled instructions
Profiled instructions summary is saved as `inst_profile.json`. Execution of each supported instruction is counted. Control flow instructions are further broken down based on the direction, and if taken or not. Finally, stack usage is logged together with number of profiled instructions

``` json
{
    "add": {"count": 26500},
    "sub": {"count": 4274},
    "sll": {"count": 8}, 
...
    "beq": {"count": 25642, "breakdown": {"taken": 9070, "taken_fwd": 3056, "taken_bwd": 6014, "not_taken": 16572, "not_taken_fwd": 11857, "not_taken_bwd": 4715}},
    "bne": {"count": 24472, "breakdown": {"taken": 6610, "taken_fwd": 0, "taken_bwd": 6610, "not_taken": 17862, "not_taken_fwd": 15105, "not_taken_bwd": 2757}},
...
    "_max_sp_usage": 368,
    "_profiled_instructions": 521118
}
```

## Execution trace and register file usage
Execution trace, saved as `trace.bin`, contains `trace_entry` struct for each executed instruction, and it's needed as an input for the analysis scripts (below)

Similarly, register file usage is saved as `rf_usage.bin`. As each instruction is executed, appropriate counters are incremented based on the used register(s), and the type of usage (destination or source)

## Hardware models outputs
Stats for three hardware models are available as `hw_stats.json`
```json
{
"icache": {
    "references": 529819,
    "hits": {"reads": 526687, "writes": 0},
    "misses": {"reads": 3132, "writes": 0},
    "replacements": 3068,
    "writebacks": 0,
    "ct_core": {"reads": 2119276, "writes": 0},
    "ct_mem": {"reads": 200448, "writes": 0},
    "size": {"data": 4096, "tags": 41, "metadata": 33, "sets": 32, "ways": 2, "line_size": 64}
},
"dcache": {
    "references": 162717,
    "hits": {"reads": 86166, "writes": 76345},
    "misses": {"reads": 29, "writes": 177},
    "replacements": 142,
    "writebacks": 142,
    "ct_core": {"reads": 304181, "writes": 285631},
    "ct_mem": {"reads": 13184, "writes": 9088},
    "size": {"data": 4096, "tags": 49, "metadata": 41, "sets": 16, "ways": 4, "line_size": 64}
},
"bpred": {
    "type": "bimodal",
    "branches": 63985,
    "predicted": 55284,
    "predicted_fwd": 36173,
    "predicted_bwd": 19111,
    "mispredicted": 8701,
    "mispredicted_fwd": 4412,
    "mispredicted_bwd": 4289,
    "accuracy": 86.40,
    "mpki": 16.70,
    "size": 12
},
"profiled_inst": 521118,
"_done": true
} 
```

Additionally, detailed breakdowns of all branches and predictor stats for each are available in `branches.csv`

```csv
PC,Direction,Funct3,Funct3_mn,Taken,Not_Taken,All,Taken%,P_bimodal,P_bimodal%,Best,P_best,P_best%,Pattern                                                   
4020c,F,1,bne,0,1000,1000,0.0,1000,100.0,bimodal,1000,100.0,1000N
40230,F,1,bne,0,1000,1000,0.0,999,99.9,bimodal,999,99.9,1000N
40234,F,1,bne,0,1000,1000,0.0,1000,100.0,bimodal,1000,100.0,1000N
40250,F,1,bne,0,1000,1000,0.0,999,99.9,bimodal,999,99.9,1000N
...
411d4,B,6,bltu,0,1000,1000,0.0,999,99.9,bimodal,999,99.9,1000N        
411dc,F,6,bltu,1000,1000,2000,50.0,1000,50.0,bimodal,1000,50.0,1T 1N 1T 1N 1T 1N 1T 1N 1T ...
411f0,B,1,bne,1000,1000,2000,50.0,1000,50.0,bimodal,1000,50.0,1T 1N 1T 1N 1T 1N 1T 1N 1T ...
...
```

## Analysis scripts
Collection of custom and open source tools are provided for profiling, analysis, and visualization

### Flat profile
Similar to the GNU Profiler `gprof`, flat profile script provides samples/time spent in all executed functions, and prints it to the `stdout`

```sh
./script/prof_stats.py -t examples/dhrystone_dhrystone_out/callstack_folded_inst.txt -e inst --plot
```

Output (also available under [examples/dhrystone_dhrystone_out/prof_stats_exec.txt](examples/dhrystone_dhrystone_out/prof_stats_exec.txt)):  
```
Profile - Inst
     %      cumulative       self      total   symbol
 16.54           16.54      86172      86172   strcpy
 12.15           28.68      63291     510433   main
 10.94           39.62      57000      57000   strcmp
 10.36           49.98      54000      90000   Proc_1
  6.00           55.98      31272      41316   _write
  5.37           61.36      28000      90000   Func_2
  4.88           66.24      25428      25428   __udivsi3
  4.80           71.03      25000      25000   Proc_8
  4.12           75.15      21475      90087   mini_vpprintf
  3.84           78.99      20000      23000   Proc_6
  3.83           82.82      19968      61284   __puts_uart
  2.88           85.70      15000      15000   Func_1
  2.30           88.00      12000      12000   Proc_7
  2.04           90.04      10616      10616   clear_bss_w_loop
  1.93           91.97      10044      10044   send_byte_uart0
  1.73           93.70       9000       9000   Proc_2
  1.73           95.42       9000       9000   Proc_3
  1.73           97.15       9000       9000   Proc_4
total_samples : 521118

(Showing 18 of 40 entries after filtering: Threshold: 1%)
```
![](examples/dhrystone_dhrystone_out/prof_stats_plot.png)

### FlameGraph
A lightweight wrapper is provided for chart formatting and annotation around the open source [FlameGraph scripts](https://github.com/brendangregg/FlameGraph)
``` sh
./script/get_flamegraph.py examples/dhrystone_dhrystone_out/callstack_folded_inst.txt
```
Open the generated interactive `flamegraph_inst.svg` in the web browser

![](examples/dhrystone_dhrystone_out/flamegraph_inst.svg)

### Call Graph  
Similarly, a lightweight wrapper around the open source [gprof2dot tool](https://github.com/jrfonseca/gprof2dot) is provided for generating Call Graphs from the same folded callstack 

Generate DOT and PNG with
```sh
./script/get_call_graph.py examples/dhrystone_dhrystone_out/callstack_folded_inst.txt --png
```

Wrapper additionally provides a few other options for saving the output call graph, other than the mandatory `.dot` format. Full usage available in [examples/get_call_graph.help](examples/get_call_graph.help)

The `.dot` format can be opened directly with
```
xdot callstack_folded_inst.dot
```

![](examples/dhrystone_dhrystone_out/call_graph_inst.png)

### Execution visualization

Main analysis script. Full usage available in [examples/run_analysis.help](examples/run_analysis.help)

Register file usage. Full usage available in [examples/rf_usage.help](examples/rf_usage.help)

> [!NOTE]
> Running any of the below commands with `-b` will open (or host with `--host`) interactive session in the browser

Get timeline plot
```sh
./script/run_analysis.py -t examples/dhrystone_dhrystone_out/trace.bin --dasm sw/baremetal/dhrystone/dhrystone.dasm --timeline
```

Get stats trace (adjust window sizes as needed)
```sh
./script/run_analysis.py -t examples/dhrystone_dhrystone_out/trace.bin --dasm sw/baremetal/dhrystone/dhrystone.dasm --stats_trace --win_size_stats 512 --win_size_hw 64 --save_decoded_trace
```

Get execution breakdown
```sh
./script/run_analysis.py -i examples/dhrystone_dhrystone_out/inst_profile.json
```

Get execution histograms
```sh
./script/run_analysis.py -t examples/dhrystone_dhrystone_out/trace.bin --dasm sw/baremetal/dhrystone/dhrystone.dasm --pc_hist --add_cache_lines
./script/run_analysis.py -t examples/dhrystone_dhrystone_out/trace.bin --dasm sw/baremetal/dhrystone/dhrystone.dasm --dmem_hist --add_cache_lines
```

Get execution trace
```sh
./script/run_analysis.py -t examples/dhrystone_dhrystone_out/trace.bin --dasm sw/baremetal/dhrystone/dhrystone.dasm --pc_trace --add_cache_lines
./script/run_analysis.py -t examples/dhrystone_dhrystone_out/trace.bin --dasm sw/baremetal/dhrystone/dhrystone.dasm --dmem_trace --add_cache_lines
```

Optionally, save symbols found in `dasm` with `--save_symbols`

> [!NOTE]
> Any time `./script/run_analysis.py` is invoked with `--dasm` arg, backannotated dasm will be saved, e.g `dhrystone.prof.dasm`

Generate register file usage plots and save csv
```sh
./script/rf_usage.py examples/dhrystone_dhrystone_out/rf_usage.bin --save_csv
```

***Timeline***  

![](examples/dhrystone_dhrystone_out/trace_timeline.png)

***Stats Trace***  

![](examples/dhrystone_dhrystone_out/trace_hw_exec.png)

***Execution breakdown***

![](examples/dhrystone_dhrystone_out/inst_profile.png)

***Execution histograms***

![](examples/dhrystone_dhrystone_out/trace_pc_hist.png)
![](examples/dhrystone_dhrystone_out/trace_dmem_hist.png)

***Execution trace***

![](examples/dhrystone_dhrystone_out/trace_pc_exec.png)
![](examples/dhrystone_dhrystone_out/trace_dmem_exec.png)

***Register file usage***

![](examples/dhrystone_dhrystone_out/rf_usage_rd_rdp_rs1_rs2_rs3.png)

***Backannotation of disassembly***

Adding `--print_symbols` to any of the commands using `-t` will print all found symbols to `stdout`
```
Symbols found in ../sw/baremetal/dhrystone/dhrystone.dasm in 'text' section:
0x430EC - 0x433D8: _free_r (0)
0x42FAC - 0x430E8: _malloc_trim_r (0)
0x42F08 - 0x42FA8: strcpy (86.2k)
0x42E7C - 0x42F04: __libc_init_array (20)
0x42E20 - 0x42E78: _sbrk_r (30)
0x42E1C - 0x42E1C: __malloc_unlock (2)
0x42E18 - 0x42E18: __malloc_lock (2)
0x42608 - 0x42E14: _malloc_r (198)
0x425FC - 0x42604: malloc (6)
0x425D4 - 0x425F8: _sbrk (17)
0x42298 - 0x425D0: mini_vpprintf (21.5k)
0x42224 - 0x42294: _puts (0)
0x42018 - 0x42220: mini_pad (819)
0x41EB0 - 0x42014: mini_itoa (2.46k)
0x41E88 - 0x41EAC: mini_strlen (500)
0x41E2C - 0x41E84: trap_handler (0)
0x41E00 - 0x41E28: timer_interrupt_handler (0)
0x41DEC - 0x41DFC: get_cpu_time (10)
0x41D98 - 0x41DE8: mini_printf (1.26k)
0x41D64 - 0x41D94: __puts_uart (20.0k)
0x41D14 - 0x41D60: _write (31.3k)
0x41CFC - 0x41D10: send_byte_uart0 (10.0k)
0x41CD4 - 0x41CF8: time_s (20)
0x41C14 - 0x41CD0: Proc_6 (20.0k)
0x41C08 - 0x41C10: Func_3 (3.00k)
0x41B8C - 0x41C04: Func_2 (28.0k)
0x41B6C - 0x41B88: Func_1 (15.0k)
0x41B08 - 0x41B68: Proc_8 (25.0k)
0x41AF8 - 0x41B04: Proc_7 (12.0k)
0x41478 - 0x41AF4: main (63.3k)
0x41468 - 0x41474: Proc_5 (4.00k)
0x41444 - 0x41464: Proc_4 (9.00k)
0x412F4 - 0x41440: Proc_1 (54.0k)
0x412D0 - 0x412F0: Proc_3 (9.00k)
0x412A8 - 0x412CC: Proc_2 (9.00k)
0x4125C - 0x412A4: __clzsi2 (0)
0x4122C - 0x41258: __modsi3 (0)
0x411F8 - 0x41228: __umodsi3 (228)
0x411B0 - 0x411F4: __hidden___udivsi3 (25.4k)
0x411A8 - 0x411AC: __divsi3 (2.00k)
0x41184 - 0x411A4: __mulsi3 (32)
0x41074 - 0x41180: __floatsisf (0)
0x41004 - 0x41070: __fixsfsi (0)
0x40CB8 - 0x41000: __mulsf3 (0)
0x40944 - 0x40CB4: __divsf3 (0)
0x4037C - 0x40940: __udivdi3 (194)
0x40200 - 0x40378: strcmp (57.0k)
0x400EC - 0x401FC: trap_entry (0)
0x400E8 - 0x400E8: forever (0)
0x400DC - 0x400E4: call_main (2)
0x400CC - 0x400D8: clear_bss_b_loop (0)
0x400C8 - 0x400C8: clear_bss_b_check (1)
0x400B8 - 0x400C4: clear_bss_w_loop (10.6k)
0x40000 - 0x400B4: _start (46)
```

It also backannotates the disassembly and saves it as `dhrystone.prof.dasm`
```
00040000 <_start>:
    1    40000:	00000093          	addi	x1,x0,0
    1    40004:	00000113          	addi	x2,x0,0
    1    40008:	00000193          	addi	x3,x0,0
    1    4000c:	00000213          	addi	x4,x0,0
    1    40010:	00000293          	addi	x5,x0,0
    1    40014:	00000313          	addi	x6,x0,0
...
000400b8 <clear_bss_w_loop>:
 2654    400b8:	00052023          	sw	x0,0(x10)
 2654    400bc:	00450513          	addi	x10,x10,4
 2654    400c0:	fff68693          	addi	x13,x13,-1
 2654    400c4:	fe069ae3          	bne	x13,x0,400b8 <clear_bss_w_loop>
...
00041468 <Proc_5>:
 1000    41468:	04100693          	addi	x13,x0,65
 1000    4146c:	82d186a3          	sb	x13,-2003(x3) # 44145 <Ch_1_Glob>
 1000    41470:	8201a823          	sw	x0,-2000(x3) # 44148 <Bool_Glob>
 1000    41474:	00008067          	jalr	x0,0(x1)
...
```

### Hardware performance estimates

By default, it uses `script/hw_perf_metrics_v2.yaml`
This microarchitecture description is based on the [ama-riscv](https://github.com/AleksandarLilic/ama-riscv) implementation

Entires other than `*_mhz` and `*_name` specify the number of cycles/stages it takes for instruction or hardware block to produce the result. Value of 1 implies a single pipeline stage, while 0 would be equivalent of a fully combinational path
``` yaml
  cpu_frequency_mhz: 100
  pipeline: 5
  # bp and caches
  bp_hit: 1
  bp_miss: 3
  icache_hit: 1
  icache_miss: 5
  dcache_hit: 1
  dcache_miss: 5
  dcache_writeback: 3
  # pipeline latencies
  jump_direct: 1
  jump_indirect: 3 # latency to resolution
  mem: 5
  mul: 2
  div: 6
  dot: 2
  dcache_load: 2 # on load-to-use, otherwise irrelevant
  # names
  icache_name: icache
  dcache_name: dcache
  bpred_name: bpred
```

Run with positional arguments as
```sh
./script/perf_est_v2.py examples/dhrystone_dhrystone_out/inst_profile.json examples/dhrystone_dhrystone_out/hw_stats.json -e examples/dhrystone_dhrystone_out/exec.log
```

Since the hardware stats are collected from the ISA model, and simple microarchitecure description, there is some uncertainty on how much time exactly it would take to execute the workload. Estimates are therefore provided as a range between best and worst case.

```
Performance estimate breakdown for: 
    examples/dhrystone_dhrystone_out/inst_profile.json
    examples/dhrystone_dhrystone_out/hw_stats.json
    <home_path>/sim/script/hw_perf_metrics_v2.yaml

Peak Stack usage: 368 bytes
Instructions executed: 521.1k
    icache (32 sets, 2 ways, 4096B data): References: 529.8k, Hits: 526.7k, Misses: 3.13k, Hit Rate: 99.41%, MPKI: 6.01
DMEM inst: 166.1k - L/S: 87.9k/78.2k (31.87% instructions)
    dcache (16 sets, 4 ways, 4096B data): References: 162.7k, Hits: 162.5k, Misses: 206, Writebacks: 142, Hit Rate: 99.87%, MPKI: 0.40
Branches: 63985 (12.28% instructions)
    Branch Predictor (bimodal): Predicted: 55.3k, Mispredicted: 8.70k, Accuracy: 86.40%, MPKI: 16.7

Pipeline stalls (max): 
    Bad spec: 17.4k
    FE bound: 61.1k - ICache: 15.7k (AMAT: 1.02), Core: 45.4k
    BE bound: 6.30k - DCache: 1.46k (AMAT: 1.01), Core: 4.84k

Estimated HW performance at 100MHz:
    Best:  Cycles: 599.6k, CPI: 1.151 (IPC: 0.869), Time: 6.00ms, MIPS: 86.9
    Worst: Cycles: 605.9k, CPI: 1.163 (IPC: 0.860), Time: 6.06ms, MIPS: 86.0
    Estimated Cycles range: 6.30k cycles, midpoint: 602.7k, ratio: 1.04%
```

# Hardware model sweeps
ISA simulator provides two types of parametrizable hardware models - caches and branch predictors. It's useful to sweep across various configurations with chosen workloads and compare their performances, as well as size and hardware complexity tradeoffs. These results then drive the decision on which configuration to implement in hardware

Full usage available in [examples/hw_model_sweep.help](examples/hw_model_sweep.help)

## Caches
Icache and Dcache share the config file for workloads, but have separate hardware parameters: [script/hw_model_sweep_params_caches.json](script/hw_model_sweep_params_caches.json)

Sweep for Icache can be run with
```sh
./script/hw_model_sweep.py -p ./script/hw_model_sweep_params_caches.json --save_stats --sweep icache --track
```
Dcache only needs `--sweep` parameter change
```sh
./script/hw_model_sweep.py -p ./script/hw_model_sweep_params_caches.json --save_stats --sweep dcache --track
```
Add `--load_stats` if the sweep has already been run and only charts need to be regenerated

With `--save_stats`, output stats of each workload, and a combined average, are saved as `.json` files, e.g.
- All workloads average: [examples/hw_sweep_icache/sweep_icache_workloads_searched_best.json](examples/hw_sweep_icache/sweep_icache_workloads_searched_best.json)
- Workload specific - Dhrystone: [examples/hw_sweep_icache/sweep_icache_dhrystone_dhrystone_best.json](examples/hw_sweep_icache/sweep_icache_dhrystone_dhrystone_best.json)

Icache average stats across all workloads
![](examples/hw_sweep_icache/sweep_icache_workloads_searched.png)

Icache stats for Dhrystone
![](examples/hw_sweep_icache/sweep_icache_dhrystone_dhrystone.png)

Dcache average stats across all workloads
![](examples/hw_sweep_dcache/sweep_dcache_workloads_searched.png)

Dcache stats for Dhrystone
![](examples/hw_sweep_dcache/sweep_dcache_dhrystone_dhrystone.png)

## Branch predictors
Similarly to caches, branch predictor sweeps are also specified through a config file with appropriate parameters: [script/hw_model_sweep_params_bp.json](script/hw_model_sweep_params_bp.json)

Sweep for branch predictors can be run with
```sh
./script/hw_model_sweep.py -p ./script/hw_model_sweep_params_bp.json --save_stats --sweep bpred --track --bp_top_acc_thr 70 --save_png
```
This also ignores all predictors with accuracy below 70%

With `--save_stats`, best and binned output stats of each workload, and a combined average, are saved as `.json` files, e.g.
- All workloads average, best predictors: [examples/hw_sweep_bpred/sweep_bpred_workloads_all_best.json](examples/hw_sweep_bpred/sweep_bpred_workloads_all_best.json)
- All workloads average, predictors binned for size: [examples/hw_sweep_bpred/sweep_bpred_workloads_all_binned.json](examples/hw_sweep_bpred/sweep_bpred_workloads_all_binned.json)
- Workload specific, best - Dhrystone: [examples/hw_sweep_bpred/sweep_bpred_dhrystone_dhrystone_best.json](examples/hw_sweep_bpred/sweep_bpred_dhrystone_dhrystone_best.json)
- Workload specific, binned - Dhrystone: [examples/hw_sweep_bpred/sweep_bpred_dhrystone_dhrystone_binned.json](examples/hw_sweep_bpred/sweep_bpred_dhrystone_dhrystone_binned.json)

If some workloads are skipped during search, an additional set of logs is available as `sweep_*_workloads_searched_*.json` that includes only stats of those predictors used for sweep

Branch predictor stats across searched workloads only
![](examples/hw_sweep_bpred/sweep_bpred_workloads_searched.png)

Branch predictor stats across all workloads
![](examples/hw_sweep_bpred/sweep_bpred_workloads_all.png)

Branch predictor stats for Dhrystone
![](examples/hw_sweep_bpred/sweep_bpred_dhrystone_dhrystone.png)

# Building RISC-V programs
Each program is in its own directory with the provided `Makefile`  
Simulation ends when `tohost` LSB is set

## Baremetal
The exact build command is test dependent, but all tests follow the same general structure with the Makefile recipes. A detailed description is available in the [baremetal README](./sw/baremetal/README.md)  

All tests and workloads are under [sw/baremetal](./sw/baremetal)

Each workload can be built individually. A more common, and easier approach is to use the provided python build script, described in [Gtest](#gtest) chapter below

### Benchmarks
Some of the standard benchmarks are available:
- [Dhrystone](./sw/baremetal/dhrystone)
- [CoreMark](./sw/baremetal/coremark)
- [Embench](./sw/baremetal/embench)
- [STREAM-INT](./sw/baremetal/stream_int) (STREAM benchmark modified to use `int32_t` instead of `float`/`double`)

### Quantized Neural Network
Quantized MLP NN is available under [mlp](./sw/baremetal/mlp) in three flavors:
- w2a8.elf
- w4a8.elf
- w8a8.elf

NN is used as a showcase of [Custom packed SIMD ISA](#custom-simd-isa) capabilities and speed-up

## RISC-V ISA tests
[Official RISC-V ISA tests](https://github.com/riscv-software-src/riscv-tests) are also provided

``` sh
cd sw/riscv-isa-tests
```

Build all tests
```sh
# uses default DIR=riscv-tests/isa/rv32ui
make -j8
```

To add CSR test
``` sh
make -j8 DIR=modified_riscv-tests/isa/rv32mi/
```

# GTest

GTest harness is used to run all available tests, while keeping the PASS/FAIL status and sim output of each test

``` sh
cd test
make
```

Prepare RISC-V tests with
```sh
./prepare_riscv_tests.py --testlist testlist.json --isa_tests
```
This also produces `gtest_testlist.txt` which lists complete paths on disk for each generated `.elf` file to be used by GTest

`Makefile` can also be used to prepare all tests specified under `test/testlist.json` and `--isa_tests` (convenience wrapper around the above python script)
``` sh
make prepare_tests
```

GTest can then be run with
```sh
make -j8
```

Test outputs are stored under `*_out` directory, while `stdout` is stored under `*_dump.log`, e.g. for Dhrystone: `dhrystone_dhrystone_out` and `dhrystone_dhrystone_dump.log`

# Building the Simulator

By default, use
```sh
make -j8
```

Options can be passed to make by using `USER_DEFINES=` argument, e.g.
```
make -j8 USER_DEFINES=-DRV32C
```

Optional build switches  
`-DDASM_EN` - allows recording of the execution log, enabled by default with `DEFINES` make variable  
`-DRV32C`- enables support for compressed RISC-V instructions (C ext.), disabled by default  
`-DUART_INPUT_EN` - enables user interaction through UART, disabled by default  
`-DDPI` - build targetting DPI environment, disabled by default

# Custom packed SIMD ISA

32-bit packed SIMD ISA with vector elements 16, 8, 4, and 2-bit wide  
Includes instructions for using Dcache as scratchpad  

## Instruction list

<details>
<summary>Arithmetic & Logic</summary>

| Name | Mnemonic |
|------|----------|
| Addition 16-bit | `add16 rd, rs1, rs2` |
| Addition 8-bit | `add8 rd, rs1, rs2` |
| Subtraction 16-bit | `sub16 rd, rs1, rs2` |
| Subtraction 8-bit | `sub8 rd, rs1, rs2` |
| Addition Signed Saturated 16-bit | `qadd16 rd, rs1, rs2` |
| Addition Unsigned Saturated 16-bit | `qadd16u rd, rs1, rs2` |
| Addition Signed Saturated 8-bit | `qadd8 rd, rs1, rs2` |
| Addition Unsigned Saturated 8-bit | `qadd8u rd, rs1, rs2` |
| Subtraction Signed Saturated 16-bit | `qsub16 rd, rs1, rs2` |
| Subtraction Unsigned Saturated 16-bit | `qsub16u rd, rs1, rs2` |
| Subtraction Signed Saturated 8-bit | `qsub8 rd, rs1, rs2` |
| Subtraction Unsigned Saturated 8-bit | `qsub8u rd, rs1, rs2` |
| Widening Multiply Signed 16-bit | `wmul16 rd, rs1, rs2` |
| Widening Multiply Unsigned 16-bit | `wmul16u rd, rs1, rs2` |
| Widening Multiply Signed 8-bit | `wmul8 rd, rs1, rs2` |
| Widening Multiply Unsigned 8-bit | `wmul8u rd, rs1, rs2` |
| Dot Product Signed 16-bit | `dot16 rd, rs1, rs2` |
| Dot Product Unsigned 16-bit | `dot16u rd, rs1, rs2` |
| Dot Product Signed 8-bit | `dot8 rd, rs1, rs2` |
| Dot Product Unsigned 8-bit | `dot8u rd, rs1, rs2` |
| Dot Product Signed 4-bit | `dot4 rd, rs1, rs2` |
| Dot Product Unsigned 4-bit | `dot4u rd, rs1, rs2` |
| Dot Product Signed 2-bit | `dot2 rd, rs1, rs2` |
| Dot Product Unsigned 2-bit | `dot2u rd, rs1, rs2` |
| Min Signed 16-bit | `min16 rd, rs1, rs2` |
| Min Unsigned 16-bit | `min16u rd, rs1, rs2` |
| Min Signed 8-bit | `min8 rd, rs1, rs2` |
| Min Unsigned 8-bit | `min8u rd, rs1, rs2` |
| Max Signed 16-bit | `max16 rd, rs1, rs2` |
| Max Unsigned 16-bit | `max16u rd, rs1, rs2` |
| Max Signed 8-bit | `max8 rd, rs1, rs2` |
| Max Unsigned 8-bit | `max8u rd, rs1, rs2` |
| Shift Left Logical Imm 16-bit | `slli16 rd, rs1, imm` |
| Shift Left Logical Imm 8-bit | `slli8 rd, rs1, imm` |
| Shift Right Logical Imm 16-bit | `srli16 rd, rs1, imm` |
| Shift Right Logical Imm 8-bit | `srli8 rd, rs1, imm` |
| Shift Right Arithmetic Imm 16-bit | `srai16 rd, rs1, imm` |
| Shift Right Arithmetic Imm 8-bit | `srai8 rd, rs1, imm` |

</details>

<details>
<summary>Data Format</summary>

| Name | Mnemonic |
|------|----------|
| Widen from Signed 16-bit | `widen16 rd, rs1` |
| Widen from Unsigned 16-bit | `widen16u rd, rs1` |
| Widen from Signed 8-bit | `widen8 rd, rs1` |
| Widen from Unsigned 8-bit | `widen8u rd, rs1` |
| Widen from Signed 4-bit | `widen4 rd, rs1` |
| Widen from Unsigned 4-bit | `widen4u rd, rs1` |
| Widen from Signed 2-bit | `widen2 rd, rs1` |
| Widen from Unsigned 2-bit | `widen2u rd, rs1` |
| Narrow from 32-bit | `narrow32 rd, rs1, rs2` |
| Narrow from 16-bit | `narrow16 rd, rs1, rs2` |
| Narrow from 8-bit | `narrow8 rd, rs1, rs2` |
| Narrow from 4-bit | `narrow4 rd, rs1, rs2` |
| Saturating Narrow from Signed 32-bit | `qnarrow16 rd, rs1, rs2` |
| Saturating Narrow from Unsigned 32-bit | `qnarrow16u rd, rs1, rs2` |
| Saturating Narrow from Signed 16-bit | `qnarrow8 rd, rs1, rs2` |
| Saturating Narrow from Unsigned 16-bit | `qnarrow8u rd, rs1, rs2` |
| Saturating Narrow from Signed 8-bit | `qnarrow4 rd, rs1, rs2` |
| Saturating Narrow from Unsigned 8-bit | `qnarrow4u rd, rs1, rs2` |
| Saturating Narrow from Signed 4-bit | `qnarrow2 rd, rs1, rs2` |
| Saturating Narrow from Unsigned 4-bit | `qnarrow2u rd, rs1, rs2` |
| Transpose across pair 16-bit | `txp16 rd, rs1, rs2` |
| Transpose across pair 8-bit | `txp8 rd, rs1, rs2` |
| Transpose across pair 4-bit | `txp4 rd, rs1, rs2` |
| Transpose across pair 2-bit | `txp2 rd, rs1, rs2` |

</details>

<details>
<summary>Vector-Scalar</summary>

| Name | Mnemonic |
|------|----------|
| Duplicate Scalar Register 16-bit | `dup16 rd, rs1` |
| Duplicate Scalar Register 8-bit | `dup8 rd, rs1` |
| Duplicate Scalar Register 4-bit | `dup4 rd, rs1` |
| Duplicate Scalar Register 2-bit | `dup2 rd, rs1` |
| Insert Scalar into Vector 16-bit | `vins16 rd, rs1, idx` |
| Insert Scalar into Vector 8-bit | `vins8 rd, rs1, idx` |
| Insert Scalar into Vector 4-bit | `vins4 rd, rs1, idx` |
| Insert Scalar into Vector 2-bit | `vins2 rd, rs1, idx` |
| Extract Scalar from Vector Signed 16-bit | `vext16 rd, rs1, idx` |
| Extract Scalar from Vector Unsigned 16-bit | `vext16u rd, rs1, idx` |
| Extract Scalar from Vector Signed 8-bit | `vext8 rd, rs1, idx` |
| Extract Scalar from Vector Unsigned 8-bit | `vext8u rd, rs1, idx` |
| Extract Scalar from Vector Signed 4-bit | `vext4 rd, rs1, idx` |
| Extract Scalar from Vector Unsigned 4-bit | `vext4u rd, rs1, idx` |
| Extract Scalar from Vector Signed 2-bit | `vext2 rd, rs1, idx` |
| Extract Scalar from Vector Unsigned 2-bit | `vext2u rd, rs1, idx` |

</details>

<details>
<summary>Cache Ops</summary>

| Name | Mnemonic |
|------|----------|
| SCP Load cache line | scp.ld rd, rs1 |
| Release SCP cache line | scp.rel rd, rs1 |

</details>

## Instruction details
Coming soon  

## Patching `binutils` to add support for custom instructions

Apply [provided patch file](./binutils.patch). Adds assembler and disassembler support for the custom `xsimd` packed SIMD extension - registers the vendor extension and defines encodings/opcodes for all custom SIMD instructions

```sh
# assuming the already compiled toolchain
# go to binutils dir
cd <toolchain_workdir>/riscv-gnu-toolchain/binutils

# optionally check patch
git apply --stat <workdir>/ama-riscv-sim/binutils.patch
git apply --check <workdir>/ama-riscv-sim/binutils.patch

# apply patch
git apply <workdir>/ama-riscv-sim/binutils.patch

# rebuild just binutils
cd <toolchain_workdir>/riscv-gnu-toolchain/build-binutils-newlib
make -j8 && make install -j8
```

Patch taken from  
RISC-V GNU toolchain repo: https://github.com/riscv/riscv-gnu-toolchain  
Pulls binutils from: https://sourceware.org/git/binutils-gdb.git

```
commit c7f28aad0c99d1d2fec4e52ebfa3735d90ceb8e9 (HEAD, tag: binutils-2_42)
Author: Nick Clifton <nickc@redhat.com>
Date:   Mon Jan 29 14:51:43 2024 +0000

    Update version number to 2.42
```

## Patching `GCC` to add support for custom instructions

Apply [provided patch file](./gcc-14.patch). Adds GCC compiler recognition of the `xsimd` extension, allowing it to be specified in `-march` strings (e.g. `-march=rv32imc_xsimd1p0`)

```sh
# assuming the already compiled toolchain
# go to gcc dir
cd <toolchain_workdir>/riscv-gnu-toolchain/gcc

# optionally check patch
git apply --stat <workdir>/ama-riscv-sim/gcc-14.patch
git apply --check <workdir>/ama-riscv-sim/gcc-14.patch

# apply patch
git apply <workdir>/ama-riscv-sim/gcc-14.patch

# rebuild gcc
cd <toolchain_workdir>/riscv-gnu-toolchain/build-gcc-newlib-stage2
make -j8 && make install -j8
```

Patch taken from  
RISC-V GNU toolchain repo: https://github.com/riscv/riscv-gnu-toolchain  
Pulls GCC from: https://github.com/gcc-mirror/gcc  
Patch targets GCC 14

```
commit 897cd794d341a3bdd3195e90ebeea054ac80bf65 (grafted, HEAD -> releases/gcc-14, origin/releases/gcc
-14)
Author: GCC Administrator <gccadmin@gcc.gnu.org>
Date:   Fri Aug 9 00:22:36 2024 +0000

    Daily bump
```
