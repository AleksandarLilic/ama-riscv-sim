#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

import regex
import yaml
from hw_model_sweep import get_test_name
from utils import INDENT, SIM_EARLY_EXIT_STRING, SIM_PASS_STRING, get_reporoot

# globals
reporoot = get_reporoot()
SIM = os.path.join(reporoot, "src", "build", "ama-riscv-sim")
MAX_WORKERS = int(os.cpu_count())

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser( description="Run a batch of workloads through the ISA sim in parallel")
    parser.add_argument("-c", "--config", required=True, help="Path to the YAML config (workloads + isa_sim_args)")
    parser.add_argument("-f", "--filter", type=str, nargs='*', help="Filter (whitespace separated list of regex patterns) to only run workloads whose path matches any of the patterns matches any of the patterns")
    parser.add_argument("--max_workers", type=int, default=MAX_WORKERS, help="Maximum number of parallel workers")
    parser.add_argument("--work_dir", default=os.getcwd(), help="Directory to run in and store sim outputs")
    parser.add_argument("--save_log", action="store_true", help="Save each workload's stdout to <name>.log")
    return parser.parse_args()

def load_config(cfg_path: str):
    with open(cfg_path, "r") as f:
        cfg = yaml.safe_load(f) or {}

    isa_sim_args = cfg.get("isa_sim_args") or []
    if not isinstance(isa_sim_args, list):
        raise ValueError("'isa_sim_args' must be a list of CLI tokens")
    # split on whitespace so "--arg value" pairs can share a line; numbers
    # (e.g. addresses) are coerced to str
    isa_sim_args = [tok for a in isa_sim_args for tok in str(a).split()]

    workloads = cfg.get("workloads")
    if not workloads:
        raise ValueError("'workloads' is required and must be non-empty")
    if not isinstance(workloads, list):
        raise ValueError("'workloads' must be a list of paths")

    return isa_sim_args, workloads

def resolve_workloads(workloads, filters):
    # resolve relative paths against the repo root, keep absolute as-is
    resolved = [p if os.path.isabs(p) else os.path.join(reporoot, p)
                for p in workloads]

    if filters:
        resolved = [p for p in resolved
                    if any(regex.search(f, p) for f in filters)]
        print(f"Filter enabled. Patterns: {filters}. "
              f"Number of workloads after filter: {len(resolved)}")

    if len(resolved) == 0:
        raise FileNotFoundError("No workloads to run (check filter/config)")

    missing = [p for p in resolved if not os.path.exists(p)]
    if missing:
        raise FileNotFoundError(
            "Workload elf(s) not found:\n" +
            "\n".join(f"{INDENT}{p}" for p in missing))

    return resolved

def run_one(app: str, sim_args, work_dir: str, save_log: bool):
    name = get_test_name(app)
    cmd = [SIM, app] + sim_args
    t_start = time.time()
    res = subprocess.run(cmd, cwd=work_dir, capture_output=True, text=True)
    runtime = time.time() - t_start

    passed = (SIM_PASS_STRING in res.stdout or
              SIM_EARLY_EXIT_STRING in res.stdout)

    insts = ""
    m = regex.search(
        r"Instruction Counters: executed: (\d+), profiled: (\d+)", res.stdout)
    if m:
        executed, profiled = int(m.group(1)), int(m.group(2))
        insts = f" - executed: {executed:,}, profiled: {profiled:,}"
        if executed == profiled:
            insts += " (all)"

    if save_log:
        with open(os.path.join(work_dir, f"{name}.log"), "w") as f:
            f.write(res.stdout)

    fail_out = ""
    if not passed:
        fail_out = (f"{INDENT}cmd: {' '.join(cmd)}\n"
                    f"{INDENT}stderr:\n{res.stderr}\n"
                    f"{INDENT}stdout (tail):\n"
                    + "\n".join(res.stdout.splitlines()[-20:]))

    return {
        "name": name,
        "passed": passed,
        "runtime": runtime,
        "insts": insts,
        "returncode": res.returncode,
        "fail_out": fail_out,
    }

def main():
    args = parse_args()

    if not os.path.exists(args.config):
        raise FileNotFoundError(f"Config not found at: {args.config}")
    if not os.path.exists(SIM):
        raise FileNotFoundError(f"Simulator not found at: {SIM}")
    if not os.path.exists(args.work_dir):
        os.makedirs(args.work_dir)

    isa_sim_args, workloads = load_config(args.config)
    workloads = resolve_workloads(workloads, args.filter)

    max_workers = min(MAX_WORKERS, args.max_workers)
    print(f"Running {len(workloads)} workload(s) with {max_workers} workers "
          f"in '{args.work_dir}'")
    if isa_sim_args:
        print(f"sim args: {' '.join(isa_sim_args)}")

    results = []
    t_start = time.time()
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = {
            executor.submit(
                run_one, app, isa_sim_args, args.work_dir, args.save_log): app
            for app in workloads
        }
        for future in as_completed(futures):
            r = future.result()
            results.append(r)
            status = "PASS" if r["passed"] else "FAIL"
            print(f"{INDENT}[{status}] {r['name']} "
                  f"({r['runtime']:.2f}s){r['insts']}")

    total = time.time() - t_start

    passed = [r for r in results if r["passed"]]
    failed = [r for r in results if not r["passed"]]

    if failed:
        print("\nFailures:")
        for r in failed:
            print(f"{INDENT}{r['name']} (rc={r['returncode']}):")
            print(r["fail_out"])

    print(f"\n{len(passed)} passed / {len(failed)} failed. "
          f"Total: {total:.2f}s")

    sys.exit(1 if failed else 0)

if __name__ == "__main__":
    main()
