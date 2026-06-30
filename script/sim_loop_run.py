#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

from hw_model_sweep import get_test_name
from sim_run_utils import (default_isa_sim, is_pass, load_config,
                           parse_inst_counts, resolve_workloads)
from utils import INDENT

# globals
MAX_WORKERS = int(os.cpu_count())

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser( description="Run a batch of workloads through the ISA sim in parallel")
    parser.add_argument("-c", "--config", required=True, help="Path to the YAML config (workloads + isa_sim_args)")
    parser.add_argument("--isa_sim", default=default_isa_sim(), help="Path to the ISA sim binary (defaults to the build/ one)")
    parser.add_argument("-f", "--filter", type=str, nargs='*', help="Filter (whitespace separated list of regex patterns) to only run workloads whose path matches any of the patterns matches any of the patterns")
    parser.add_argument("--max_workers", type=int, default=MAX_WORKERS, help="Maximum number of parallel workers")
    parser.add_argument("--work_dir", default=os.getcwd(), help="Directory to run in and store sim outputs")
    parser.add_argument("--save_log", action="store_true", help="Save each workload's stdout to <name>.log")
    return parser.parse_args()

def run_one(sim: str, app: str, sim_args, work_dir: str, save_log: bool):
    name = get_test_name(app)
    cmd = [sim, app] + sim_args
    t_start = time.time()
    res = subprocess.run(cmd, cwd=work_dir, capture_output=True, text=True)
    runtime = time.time() - t_start

    passed = is_pass(res.stdout)

    executed, profiled = parse_inst_counts(res.stdout)
    insts = ""
    if executed is not None:
        if profiled is not None:
            insts = f" - executed: {executed:,}, profiled: {profiled:,}"
            if executed == profiled:
                insts += " (all)"
            else:
                insts += f", diff: {profiled - executed:,}"
        else:
            insts = f" - executed: {executed:,}"

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
    args.isa_sim = os.path.abspath(args.isa_sim)

    if not os.path.exists(args.config):
        raise FileNotFoundError(f"Config not found at: {args.config}")
    if not os.path.exists(args.isa_sim):
        raise FileNotFoundError(f"Simulator not found at: {args.isa_sim}")
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
                run_one,
                args.isa_sim, app, isa_sim_args, args.work_dir, args.save_log
            ): app for app in workloads
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
