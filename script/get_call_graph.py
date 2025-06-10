#!/usr/bin/env python3

import os
import subprocess
import argparse
from pathlib import Path

SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))
DOT_PY = os.path.join(SCRIPT_PATH, "gprof2dot/gprof2dot.py")

def run_cmd(cmd):
    result = subprocess.run(cmd)
    if result.returncode != 0:
        raise RuntimeError(f"Error: {result.stderr.decode('utf-8')}")

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Convert collapsed callstack to call graph")
    parser.add_argument('input', help="Collapsed stack trace")
    parser.add_argument('--svg', action="store_true", help="Also save output as SVG")
    parser.add_argument('--png', action="store_true", help="Also save output as PNG")
    parser.add_argument('--pdf', action="store_true", help="Also save output as PDF")

    return parser.parse_args()

args = parse_args()
folded_stack = args.input
if not os.path.exists(folded_stack):
    raise FileNotFoundError(f"File not found: {folded_stack}")

# e.g. out_dhrystone_dhrystone/callstack_folded_exec.txt
stack_name = os.path.splitext(folded_stack)[0]
try:
    bench_name = os.path.dirname(stack_name).split("out_")[1]
except:
    bench_name = 'none'
stack_type = os.path.basename(stack_name).split("folded_")[1]
dot_out_name = os.path.join(os.path.dirname(stack_name),
                       f"call_graph_{stack_type}.dot")

cmd = [DOT_PY, folded_stack, "-f", "collapse", "-o", dot_out_name]
run_cmd(cmd)

for fmt in ("svg", "png", "pdf"):
    if getattr(args, fmt):
        out_name = Path(dot_out_name).with_suffix(f".{fmt}")
        cmd = ["dot", f"-T{fmt}", dot_out_name, "-o", str(out_name)]
        run_cmd(cmd)
