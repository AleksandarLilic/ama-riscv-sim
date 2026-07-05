#!/usr/bin/env python3
"""
sweep over ISA sim build flag combinations

catches improperly guarded code by building all common configs
uses `make obj` (no link) by default; pass --ld to also link a binary per config

each config gets its own BDIR=build_matrix/<name>
re-running the script only rebuilds configs that changed (or never built)

DPI is out of scope for now, too many dependencies for this repo
"""

import argparse
import multiprocessing
import os
import subprocess
import sys
import time

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SIM_SRC_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "src"))
BDIR_ROOT = "build_matrix" # relative to sim/src, matches gitignored 'build*/'

# make variable -> short name used in BDIR, default value per sim/src/Makefile
FLAGS = [
    ("PROFILERS", "P", 1),
    ("HW_MODELS", "H", 1),
    ("DASM", "D", 0),
    ("RV32C", "C", 0),
    ("SIMD", "S", 1),
    ("UART", "U", 1),
    ("UART_IN", "UI", 0),
    ("DEBUG", "DBG", 0),
    ("CACHE_MODE", "CM", 0),
    ("CACHE_VERIFY", "CV", 0),
]
DEFAULTS = {name: default for name, _, default in FLAGS}
ABBR = {name: abbr for name, abbr, _ in FLAGS}

def cfg(**overrides):
    # use defaults, then override provided entries
    c = dict(DEFAULTS)
    c.update(overrides)
    return c

def name_of(c):
    return "_".join(f"{ABBR[name]}{c[name]}" for name, _, _ in FLAGS)

def gen_configs():
    configs = []

    # full cross, everything else at defaults.
    for p in (0, 1):
        for h in (0, 1):
            for d in (0, 1):
                for c in (0, 1):
                    for s in (0, 1):
                        configs.append(cfg(
                            PROFILERS=p, HW_MODELS=h, DASM=d, RV32C=c, SIMD=s
                        ))

    low = cfg(PROFILERS=0, HW_MODELS=0, DASM=0, RV32C=0, SIMD=0)
    high = cfg(PROFILERS=1, HW_MODELS=1, DASM=1, RV32C=1, SIMD=1)

    # dependent pairs (UART_IN needs UART=1, CACHE_* needs HW_MODELS=1):
    # tack one flip at a time onto the two corners rather than a full cross
    # take existing high/low config, then override specific flags
    configs.append(dict(low, DEBUG=1))
    configs.append(dict(high, DEBUG=1))
    configs.append(dict(high, UART=0, UART_IN=0))
    configs.append(dict(high, UART_IN=1))
    configs.append(dict(high, CACHE_VERIFY=1))
    configs.append(dict(high, CACHE_MODE=1))

    # all off/on
    configs.append(dict(low, UART=0))
    configs.append(dict(high, UART_IN=1, DEBUG=1, CACHE_MODE=1, CACHE_VERIFY=1))

    # de-dup while preserving order (corner tack-ons can coincide with core)
    seen = set()
    unique = []
    for c in configs:
        key = tuple(sorted(c.items()))
        if key not in seen:
            seen.add(key)
            unique.append(c)
    return unique

def run_config(c, jobs, log_dir, ld):
    name = name_of(c)
    bdir = f"{BDIR_ROOT}/{name}"
    log_path = os.path.join(log_dir, f"{name}.log")

    target = "all" if ld else "obj"
    make_args = [f"{k}={v}" for k, v in c.items()]
    cmd = [
        "make", "-C", SIM_SRC_DIR, target, f"BDIR={bdir}", f"-j{jobs}"
    ] + make_args

    t0 = time.time()
    with open(log_path, "w") as log_f:
        log_f.write(f"$ {' '.join(cmd)}\n\n")
        log_f.flush()
        proc = subprocess.run(cmd, stdout=log_f, stderr=subprocess.STDOUT)
    rt = time.time() - t0

    return name, (proc.returncode == 0), rt, log_path

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="ISA sim build config sweep")
    parser.add_argument("--filter", help="only run configs whose name contains this substring")
    parser.add_argument("--jobs", type=int, default=multiprocessing.cpu_count(), help="make -j per config (default: nproc)")
    parser.add_argument("--dry-run", action="store_true", help="list configs and exit")
    parser.add_argument("--ld", action="store_true",
                         help="also link each config into a binary (make all instead of make obj)")
    return parser.parse_args()

def main():
    args = parse_args()
    configs = gen_configs()
    if args.filter:
        configs = [c for c in configs if args.filter in name_of(c)]

    if args.dry_run:
        for c in configs:
            print(name_of(c))
        print(f"\n{len(configs)} configs")
        return 0

    log_dir = os.path.join(SIM_SRC_DIR, BDIR_ROOT)
    os.makedirs(log_dir, exist_ok=True)

    st_all = time.time()
    results = []
    for i, c in enumerate(configs, 1):
        name, ok, rt, log_path = run_config(c, args.jobs, log_dir, args.ld)
        status = "PASS" if ok else "FAIL"
        print(
            f"[{i:2}/{len(configs)}]"
            f"  {status:4s}  {name:45s} {rt:3.1f}s"
        , end='')
        if not ok:
            print(f"    (Log under '{log_path}')", end='')
        print()
        results.append((name, ok, log_path))
    rt_all = time.time() - st_all

    failed = [r for r in results if not r[1]]
    print(
        f"\n{len(results)} configs, "
        f"{len(results) - len(failed)} passed, {len(failed)} failed"
    )
    print(f"Runtime: {rt_all:4.0f}s")
    if failed:
        print("\nFailed configs (see logs):")
        for name, _, log_path in failed:
            print(f"  {name}: {log_path}")
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main())
