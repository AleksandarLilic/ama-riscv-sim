#!/usr/bin/env python3

import os
import sys
import subprocess

SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))
FLAMEGRAPH_PL = os.path.join(SCRIPT_PATH, "FlameGraph/flamegraph.pl")

if len(sys.argv) < 2:
    raise ValueError("Usage: ./get_flamegraph.py <folded_stack>")

folded_stack = sys.argv[1]
if not os.path.exists(folded_stack):
    raise FileNotFoundError(f"File not found: {folded_stack}")

# e.g. out_dhrystone_dhrystone/callstack_folded_exec.txt
stack_name = os.path.splitext(folded_stack)[0]
bench_name = os.path.dirname(stack_name).split("out_")[1]
stack_type = os.path.basename(stack_name).split("folded_")[1]
svg_out = os.path.join(os.path.dirname(stack_name),
                       f"flamegraph_{stack_type}.svg")

with open(svg_out, "w") as f:
    cmd = [
        FLAMEGRAPH_PL,
        folded_stack,
        "--title", bench_name,
        "--subtitle", stack_type,
        "--width", "1920",
        "--height", "24",
        "--color", "aqua"
    ]
    result = subprocess.run(cmd, stdout=f)
    if result.returncode != 0:
        raise RuntimeError(f"Error: {result.stderr.decode('utf-8')}")
