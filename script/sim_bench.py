#!/usr/bin/env python3

import argparse
import json
import statistics
import subprocess
import sys
import time

import regex
from hw_model_sweep import get_test_name
from sim_run_utils import (add_common_args, host_meta, is_pass,
                           parse_inst_counts, perf_available, prepare, sim_cmd,
                           write_json)
from utils import INDENT

PERF_EVENTS = [
    "instructions", "cycles",
    "branches", "branch-misses",
    "cache-references", "cache-misses"
]

def parse_sim_self_mips(stdout: str):
    # "Simulation performance: 39.16 MIPS (1643650 instructions in 0.04s)"
    m = regex.search(r"Simulation performance: ([\d.]+) MIPS", stdout)
    return float(m.group(1)) if m else None

def run_perf_stat(cmd, work_dir):
    pcmd = ["perf", "stat", "-x,", "-e", ",".join(PERF_EVENTS)] + cmd
    res = subprocess.run(pcmd, cwd=work_dir, capture_output=True, text=True)
    counters = {}
    for line in res.stderr.splitlines():
        parts = line.split(",")
        if len(parts) >= 3:
            try:
                counters[parts[2]] = int(parts[0])  # raw counts are integers
            except ValueError:
                pass  # "<not supported>" / "<not counted>"
    if "instructions" not in counters or "cycles" not in counters:
        return None
    return counters

def perf_metrics(c: dict) -> dict:
    m = {}
    avg = lambda x, y, p: round((x / y) * p, 3)
    if c.get("cycles"):
        m["ipc"] = avg(c["instructions"], c["cycles"], 1)
    if c.get("branches"):
        m["branch_miss_pct"] = \
            avg(c.get("branch-misses", 0), c["branches"], 100)
    if c.get("cache-references"):
        m["cache_miss_pct"] = \
            avg(c.get("cache-misses", 0), c["cache-references"], 100)
    return m

def bench_one(sim, app, sim_args, args, perf_ok):
    name = get_test_name(app)
    cmd = sim_cmd(sim, app, sim_args, args.pin)

    for _ in range(max(0, args.warmup)):
        subprocess.run(cmd, cwd=args.work_dir, capture_output=True, text=True)

    times = []
    last = None
    for _ in range(args.repeats):
        t0 = time.perf_counter()
        last = subprocess.run(
            cmd, cwd=args.work_dir, capture_output=True, text=True
        )
        times.append(time.perf_counter() - t0)

    passed = is_pass(last.stdout)
    executed, _ = parse_inst_counts(last.stdout)
    median = statistics.median(times)
    mips = round(executed / median / 1e6, 2) if executed and median else None
    res = {
        "name": name,
        "passed": passed,
        "executed": executed,
        "runtime_s_min": round(min(times), 6),
        "runtime_s_median": round(median, 6),
        "mips": mips,
        "sim_self_mips": parse_sim_self_mips(last.stdout),
    }

    if perf_ok:
        counters = run_perf_stat(cmd, args.work_dir)
        if counters:
            # raw counters first, derived stats after
            res["perf"] = {**counters, **perf_metrics(counters)}

    return res

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Benchmark host throughput (MIPS) of the ISA sim")
    add_common_args(p)
    p.add_argument("--repeats", type=int, default=5, help="Timed runs per workload (median/min reported)")
    p.add_argument("--warmup", type=int, default=1, help="Warmup runs per workload (discarded)")
    p.add_argument("--perf", action="store_true", help="Wrap each workload in `perf stat` for IPC/branch/cache counters")
    p.add_argument("--baseline", default=None, help="Prior results JSON to diff MIPS against")
    return p.parse_args()

def main():
    args = parse_args()
    isa_sim_args, workloads = prepare(args)

    perf_ok = args.perf
    if args.perf and not perf_available():
        print(
            "WARNING: `perf stat` unavailable; skipping counters. "
            "Check kernel.perf_event_paranoid.")
        perf_ok = False

    print(f"Benchmarking {len(workloads)} workload(s), "
          f"repeats={args.repeats}, "
          f"warmup={args.warmup}, "
          f"pin={'none' if args.pin is None else args.pin} "
          f"in '{args.work_dir}'"
    )
    print(f"sim: {args.isa_sim}")
    if isa_sim_args:
        print(f"sim args: {' '.join(isa_sim_args)}")

    results = []
    for app in workloads:
        r = bench_one(args.isa_sim, app, isa_sim_args, args, perf_ok)
        results.append(r)
        status = "PASS" if r["passed"] else "FAIL"
        extra = ""
        p = r.get("perf")
        if p and p.get("ipc") is not None:
            extra = (
                f"IPC: {p['ipc']:<5}, "
                f"bp miss [%]: {p.get('branch_miss_pct'):<5}, "
                f"cache miss [%]: {p.get('cache_miss_pct'):<6}"
            )
        print(
            f"{INDENT}[{status}] {r['name']:<24} "
            f"{str(r['mips']):<5} MIPS "
            f"(median {r['runtime_s_median']:.4f}s, "
            f"self {r['sim_self_mips']:<5})\t"
            f"{extra}"
        )

    out = {
        "meta": host_meta(
            args.isa_sim, isa_sim_args, args.pin, repeats=args.repeats
        ),
        "results": results,
    }

    if args.json_out:
        write_json(args.json_out, out)
        print(f"\nWrote {args.json_out}")

    if args.baseline:
        with open(args.baseline) as f:
            base = {r["name"]: r for r in json.load(f).get("results", [])}
        print(f"\nDelta vs baseline ({args.baseline}):")
        for r in results:
            b = base.get(r["name"])
            if not b or not b.get("mips") or not r.get("mips"):
                print(f"{INDENT}{r['name']:<24} (no baseline)")
                continue
            d = (r["mips"] - b["mips"]) / b["mips"] * 100
            print(f"{INDENT}"
                f"{r['name']:<24} {b['mips']:>8} -> "
                f"{r['mips']:<8} MIPS  ({d:+.1f}%)"
            )

    sys.exit(0 if all(r["passed"] for r in results) else 1)

if __name__ == "__main__":
    main()
