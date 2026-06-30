#!/usr/bin/env python3

# sampling profiler for the ISA sim host machine
# wraps one dedicated `perf record` run per workload and
# converts it to the folded-stack format
# needs symbols (build with PERF=1)
# and a usable perf (kernel.perf_event_paranoid)

import argparse
import os
import subprocess
import sys

from hw_model_sweep import get_test_name
from sim_run_utils import (add_common_args, get_build_info, host_meta, is_pass,
                           perf_available, prepare, sim_cmd, write_json)
from utils import INDENT

SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))
STACKCOLLAPSE = os.path.join(SCRIPT_PATH, "FlameGraph", "stackcollapse-perf.pl")

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Sample-profile the host execution of the ISA sim (perf record -> folded stacks)")
    add_common_args(p)
    p.add_argument("-F", "--freq", type=int, default=999, help="perf sampling frequency in Hz")
    p.add_argument("--call-graph", dest="call_graph", choices=["fp", "dwarf"], default="fp", help="perf stack unwind method (fp needs PERF=1 frame pointers)")
    p.add_argument("--top", type=int, default=15, help="Top-N hot symbols to print to stdout")
    return p.parse_args()

def record(cmd, data, freq, cg_mode, cwd):
    rec = ([
        "perf", "record",
        "-F", str(freq),
        "--call-graph", cg_mode,
        "-o", data, "--"
        ] + cmd
    )
    return subprocess.run(rec, cwd=cwd, capture_output=True, text=True)

def collapse(data, folded, cwd):
    # perf script -i <data> | stackcollapse-perf.pl > <folded>
    script = subprocess.run(
        ["perf", "script", "-i", data], cwd=cwd, capture_output=True, text=True
    )
    if script.returncode != 0:
        return False, script.stderr
    col = subprocess.run(
        [STACKCOLLAPSE], input=script.stdout, capture_output=True, text=True
    )
    if col.returncode != 0:
        return False, col.stderr
    with open(folded, "w") as f:
        f.write(col.stdout)
    return True, ""

def parse_folded(path):
    # leaf frame gets the self-weight; sum is total weight;
    # perf samples on `cycles` are a per-sample PERIOD,
    # so the count is period-weighted (~cycles),
    # not a raw sample count, percentages are important
    self_counts = {}
    total = 0
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                stack, cnt = line.rsplit(None, 1)
                cnt = int(cnt)
            except ValueError:
                continue
            frames = [x for x in stack.split(";") if x]
            if not frames:
                continue
            self_counts[frames[-1]] = self_counts.get(frames[-1], 0) + cnt
            total += cnt
    return self_counts, total

def top_symbols(self_counts, total, n):
    ranked = sorted(self_counts.items(), key=lambda kv: kv[1], reverse=True)
    return [{
        "symbol": s,
        "self_weight": c,
        "self_pct": round(c / total * 100, 2) if total else 0.0
        } for s, c in ranked[:n]
    ]

def profile_one(sim, app, sim_args, args):
    name = get_test_name(app)
    d = os.path.abspath(os.path.join(args.work_dir, name))
    os.makedirs(d, exist_ok=True)
    data = os.path.join(d, f"{name}.data")
    folded = os.path.join(d, f"{name}_folded_host.txt")

    cmd = sim_cmd(sim, app, sim_args, args.pin)

    rec = record(cmd, data, args.freq, args.call_graph, d)
    passed = is_pass(rec.stdout)
    if rec.returncode != 0:
        print(
            f"{INDENT}[ERROR] {name}: perf record failed (rc={rec.returncode})"
        )
        print("\n".join(rec.stderr.splitlines()[-10:]))
        return None

    ok, err = collapse(data, folded, d)
    if not ok:
        print(f"{INDENT}[ERROR] {name}: folding failed\n{err}")
        return None

    # stdout glance only; render folded with get_flamegraph.py / prof_stats.py
    self_counts, total = parse_folded(folded)
    status = "PASS" if passed else "FAIL"
    print(f"{INDENT}[{status}] {name}  ({total:,} weight, "
          f"{len(self_counts)} symbols)")
    for s in top_symbols(self_counts, total, args.top):
        print(f"{INDENT * 2}{s['self_pct']:>6.2f}%  {s['symbol']}")

    return {
        "name": name,
        "passed": passed,
        "perf_data": data,
        "folded": folded,
    }

def main():
    args = parse_args()
    isa_sim_args, workloads = prepare(args)

    if not perf_available():
        sys.exit(
            "ERROR: `perf` unavailable. Sampling needs it - lower "
            "kernel.perf_event_paranoid (e.g. sudo sysctl -w "
            "kernel.perf_event_paranoid=1) or run with privileges."
        )

    if not os.path.exists(STACKCOLLAPSE):
        sys.exit(
            f"ERROR: {STACKCOLLAPSE} missing (init the FlameGraph submodule)"
        )

    build_info = get_build_info(args.isa_sim)
    cxxflags = build_info.get("CXXFLAGS", "")
    if args.call_graph == "fp" and "fno-omit-frame-pointer" not in cxxflags:
        print(
            "WARNING: sim not built with PERF=1 (no frame pointers); "
            "fp call graphs may be shallow. "
            "Rebuild with PERF=1 or use --call-graph dwarf."
        )

    print(
        f"Profiling {len(workloads)} workload(s),"
        f"freq={args.freq}Hz, "
        f"call-graph={args.call_graph}, "
        f"pin={'none' if args.pin is None else args.pin} in "
        f"'{args.work_dir}'"
    )
    print(f"sim: {args.isa_sim}")
    if isa_sim_args:
        print(f"sim args: {' '.join(isa_sim_args)}")

    results = []
    for app in workloads:
        entry = profile_one(args.isa_sim, app, isa_sim_args, args)
        if entry is not None:
            results.append(entry)

    out = {
        "meta": host_meta(
            args.isa_sim, isa_sim_args, args.pin,
            freq=args.freq, call_graph=args.call_graph
        ),
        "results": results,
    }

    if args.json_out:
        write_json(args.json_out, out)
        print(f"\nWrote {args.json_out}")

    sys.exit(0 if results and all(r["passed"] for r in results) else 1)

if __name__ == "__main__":
    main()
