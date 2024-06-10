# Scripts

> Check [build README](../src/README.md) on how to generate logs and add profiling

## Analysis of instrucion log and trace

Script: [`./run_analysis.py`](./run_analysis.py)

***Barebones approach***  
Run instruction log analysis
```bash
python3 run_analysis.py -i ../src/vector_mac_vm_uint8_inst_profiler.json
../sw/baremetal/vector_mac/vm_uint8.dump
```

Run trace analysis
```bash
python3 run_analysis.py -t ../src/vector_mac_vm_uint8_trace.bin
```

***Most common approach***  
Run instruction log analysis with highlight
```bash
HL="lw,lh,lb,lhu,lbu,sw,sh,sb bne,beq,blt,bge,bgeu,bltu jal,jalr"
python3 run_analysis.py -i ../src/vector_mac_vm_uint8_inst_profiler.json --highlight $HL
```

Run trace analysis with annotated symbols and highlight
```bash
python3 run_analysis.py -t ../src/vector_mac_vm_uint8_trace.bin --dasm ../sw/baremetal/vector_mac/vm_uint8.dump --highlight $HL
```
For long traces, increase limit with `--pc_time_series_limit 120000`  
Optionally, save symbols found in `dasm` for later annotation: `--save_symbols`  
Filter out only part of the program by specifying program counter begin/end with: `--pc_begin 0x80000094 --pc_end 0x800000ec`

With `--dasm`, script reports symbols and number of executed instruction for each symbol (i.e. function)
```
Symbols found in ../sw/baremetal/vector_mac/vm_uint8.dump:
0x01CC - 0x02AC: main (10906)
0x01A8 - 0x01C8: __mulsi3 (43264)
0x0128 - 0x01A4: write_uart0 (0)
0x00F4 - 0x0124: fail (0)
0x00C4 - 0x00F0: pass (7)
0x00A4 - 0x00C0: write_csr_status (8)
0x0060 - 0x00A0: write_mismatch (0)
0x0048 - 0x005C: set_c (0)
0x0040 - 0x0044: asm_add (0)
0x002C - 0x003C: test_end (5)
0x0028 - 0x0028: done_bss (1)
0x0018 - 0x0024: clear_bss (257)
0x0000 - 0x0014: _start (6)
```

Both instruction log and trace analysis can accept:
- `--silent` to suppress window pop-up
- `--save_png` or `--save_svg` to save chart to disk
- `--save_csv` to save source data formatted as csv to disk

For all options:
```bash
python3 run_analysis.py -h
```

## Annotate execution log
Script: [`./annotate_exec_log.py`](./annotate_exec_log.py)  
Requires that symbols have been saved in during trace analysis
``` bash
python3 annotate_exec_log.py -l ../src/vector_mac_vm_uint8_exec.log -s ../sw/baremetal/vector_mac/vm_uint8_symbols.json
```

## HW performance estimates

Script: [`perf_est.py`](./perf_est.py)  
HW config file: [`hw_perf_metrics.json`](./hw_perf_metrics.json)

```bash
python3 perf_est.py ../src/vector_mac_vm_uint8_inst_profiler.json hw_perf_metrics.json
```

Config file, based on the [ama-riscv](https://github.com/AleksandarLilic/ama-riscv) implementation
```json
{
    "cpu_frequency_mhz" : 125,
    "pipeline_latency" : 5,
    "branch_resolution" : 1,
    "jump_resolution" : 1,
    "memory_response" : 0,
    "mispredict_penalty" : 1,
    "prediction_resolution" : 0
}
```

`pipeline_latency` - number of clock cycles between reset being deasserted and the first instruction being committed  
`"branch_resolution" : 1` - branch would cause 1 clock bubble  
`"jump_resolution" : 1` - jump would cause 1 clock bubble  
`"memory_response" : 0` - DMEM responds in the next cycle  
`"mispredict_penalty" : 1` - one instruction is flushed on misprediction (since there is one cycle delay for branch resolution)  
`"prediction_resolution" : 0` - prediction is available in the next cycle  

Output:
```
vector_mac_vm_uint8_inst_profiler.json
Branches total: 15537 out of 54454 total instructions (28.5% branches)
    Taken: 10030, Forwards: 2833, Backwards: 7197
    Not taken: 5507, Forwards: 4416, Backwards: 1091
    Predicted: 11613, Mispredicted: 3924, Accuracy: 74.7%
    Cycles: Original/With prediction: 72113/60500 (11613 cycles saved)
Potential app speedup: 16.1%
Estimated HW performance at 125MHz with 72113 cycles executed: CPI=1.32, exec time=576.9us, MIPS=94.4
Estimated HW performance at 125MHz with 60500 cycles executed: CPI=1.11, exec time=484.0us, MIPS=112.5
```
