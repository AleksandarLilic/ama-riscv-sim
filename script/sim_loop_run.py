#!/usr/bin/env python3

import argparse
import os
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

from hw_model_sweep import get_test_name
from sim_run_utils import (add_common_args, output_tail, parse_inst_counts,
                           prepare, run_cmd, run_status)
from utils import INDENT

# globals
MAX_WORKERS = int(os.cpu_count())

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser( description="Run a batch of workloads through the ISA sim in parallel")
    add_common_args(p)
    p.add_argument("--save_log", action="store_true", help="Save each workload's stdout to <name>.log")
    p.add_argument("--max_workers", type=int, default=MAX_WORKERS, help="Maximum number of parallel workers")
    return p.parse_args()

def run_one(
    sim: str, app: str, sim_args, work_dir: str, save_log: bool, timeout=None
):
    name = get_test_name(app)
    cmd = [sim, app] + sim_args
    res = run_cmd(cmd, work_dir, timeout)
    status = run_status(res)

    executed, profiled = parse_inst_counts(res["stdout"])
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
            f.write(res["stdout"])

    fail_out = ""
    if status != "PASS":
        fail_out = f"{INDENT}cmd: {' '.join(cmd)}\n" + output_tail(res, 20)

    return {
        "name": name,
        "status": status,
        "runtime": res["elapsed_s"],
        "insts": insts,
        "error_msg": res["error_msg"],
        "fail_out": fail_out,
    }

def main():
    args = parse_args()
    isa_sim_args, workloads = prepare(args)

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
                args.isa_sim, app, isa_sim_args, args.work_dir, args.save_log,
                args.timeout
            ): app for app in workloads
        }
        for future in as_completed(futures):
            r = future.result()
            results.append(r)
            print(f"{INDENT}[{r['status']}] {r['name']} "
                  f"({r['runtime']:.2f}s){r['insts']}")

    total = time.time() - t_start

    passed = [r for r in results if r["status"] == "PASS"]
    failed = [r for r in results if r["status"] != "PASS"]

    if failed:
        print("\nFailures:")
        for r in failed:
            why = f": {r['error_msg']}" if r["error_msg"] else ""
            print(f"{INDENT}{r['name']} [{r['status']}]{why}")
            print(r["fail_out"])

    print(f"\n{len(passed)} passed / {len(failed)} failed. "
          f"Total: {total:.2f}s")

    sys.exit(1 if failed else 0)

if __name__ == "__main__":
    main()
