#!/usr/bin/env python3

import sys
import os
import glob
from analyze_profiling_log import run_main
from perf_est import perf

class l_args:
    def reset(self):
        self.inst_log = ""
        self.inst_dir = ""
        self.trace = ""
        self.trace_dir = ""

        self.exclude = None
        self.exclude_type = None
        self.top = None
        self.allow_zero = False
        self.estimate_perf = 'hw_perf_metrics.json'

        self.dasm = ""
        self.pc_begin = ""
        self.pc_end = ""
        self.save_symbols = None
        self.pc_time_series_limit = 50000

        self.highlight = None
        self.silent = True
        self.symbols_only = True
        self.save_png = False
        self.save_svg = False
        self.save_pdf = None
        self.save_csv = False
        self.combined_only = False
        self.output = os.getcwd()

    def __init__(self):
        self.reset()

bench = sys.argv[1]
flavor = sys.argv[2] if len(sys.argv) > 2 else ""
inst_logs = sorted(glob.glob(f'../src/{bench}*{flavor}*_inst_profiler.json'))
traces = sorted(glob.glob(f'../src/{bench}*{flavor}*_trace.bin'))
dasms = sorted(glob.glob(f'../sw/baremetal/{bench}*/*{flavor}*.dump'))
# remove elements with .prof. in the name from dasms list
dasms = [x for x in dasms if ".prof." not in x]

mispredict_penalty = 1
prediction_resolution = 0
argsc = l_args()
for trace, dasm, inst_l in zip(traces, dasms, inst_logs):
    test_name = os.path.splitext(os.path.basename(dasm))[0]
    if test_name not in os.path.basename(trace) or test_name not in os.path.basename(inst_l):
        print(f"Test name mismatch: {test_name}")
        print(f"inst_l: {inst_l}")
        print(f"trace: {trace}")
        print(f"dasm: {dasm}")
        raise ValueError("Test name mismatch")
    argsc.trace = [trace]
    argsc.dasm = dasm
    # prints symbols
    run_main(args=argsc)
    # runs analysis
    est = perf(inst_l, argsc.estimate_perf)
    est.save_as_df()
    print(est)
    print()
