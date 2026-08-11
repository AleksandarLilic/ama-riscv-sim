#!/usr/bin/env python3

import argparse
import os
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

from hw_model_sweep import get_test_name
from sim_run_utils import (add_common_args, init_res, output_tail,
                           parse_inst_counts, prepare, run_cmd, run_status)
from utils import INDENT

# globals
MAX_WORKERS = int(os.cpu_count())
SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))
HW_PERF_EST = os.path.join(SCRIPT_PATH, "hw_perf_est.py")
PERF_EST_INPUTS = ["inst_profile.json", "hw_stats.json", "rf_trace.bin"]
PROF_TRACE_ARGS = ("-t", "--prof_trace")

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser( description="Run a batch of workloads through the ISA sim in parallel")
    add_common_args(p)
    p.add_argument("--save_log", action="store_true", help="Save each workload's stdout to <name>.log")
    p.add_argument("--perf_est", action="store_true", help="Run hw_perf_est.py on each workload's sim outputs (requires '-t' in the config's 'isa_sim_args')")
    p.add_argument("--max_workers", type=int, default=MAX_WORKERS, help="Maximum number of parallel workers")
    return p.parse_args()

def sim_out_dir(work_dir: str, app: str, sim_args) -> str:
    # mirrors gen_out_dir() in src/utils.h: '<test_name>_out[_<tag>]'
    name = f"{get_test_name(app)}_out"
    if "--out_dir_tag" in sim_args:
        name += "_" + sim_args[sim_args.index("--out_dir_tag") + 1]

    return os.path.join(os.path.abspath(work_dir), name)

def run_perf_est(out_dir: str, work_dir: str, timeout=None):
    # estimate HW performance from the sim outputs; returns (stdout, error_msg)

    inputs = [os.path.join(out_dir, f) for f in PERF_EST_INPUTS]
    missing = [f for f, p in zip(PERF_EST_INPUTS, inputs)
               if not os.path.isfile(p)]
    if missing:
        res = init_res()
        res["error_msg"] = f"missing sim output(s): {', '.join(missing)}"
        return res

    res = run_cmd([HW_PERF_EST] + inputs + ["--save_json"], work_dir, timeout)
    # run_status() applies only to isa sim directly
    if res["error_msg"]: # rc alone says nothing, carry the traceback along
        res["error_msg"] += "\n" + output_tail(res, 3)
    return res

def run_one(
    sim: str, app: str, sim_args, work_dir: str, save_log: bool,
    perf_est: bool, timeout=None
):
    name = get_test_name(app)
    cmd = [sim, app] + sim_args
    res = run_cmd(cmd, work_dir, timeout)
    status = run_status(res)

    executed, profiled = parse_inst_counts(res["stdout"])
    insts = ""
    if executed is not None:
        if profiled is not None:
            insts = f"executed: {executed:,},  profiled: {profiled:,}"
            if executed == profiled:
                insts += " (all)"
            else:
                insts += f",  diff: {profiled - executed:,}"
        else:
            insts = f"executed: {executed:,}"

    # only estimate for a passing run
    perf_est_res = init_res()
    if perf_est and status == "PASS":
        perf_est_res = run_perf_est(
            sim_out_dir(work_dir, app, sim_args), work_dir, timeout
        )

    if save_log:
        with open(os.path.join(work_dir, f"{name}.log"), "w") as f:
            f.write(res["stdout"])
            if perf_est_res['stdout']:
                f.write("\n" + perf_est_res['stdout'])

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
        "perf_est_runtime": perf_est_res['elapsed_s'],
        "perf_est_error_msg": perf_est_res['error_msg'],
    }

def main():
    args = parse_args()
    isa_sim_args, workloads = prepare(args)

    if args.perf_est and not any(a in PROF_TRACE_ARGS for a in isa_sim_args):
        sys.exit("--perf_est requires -t/--prof_trace for isa sim run")

    max_workers = min(MAX_WORKERS, args.max_workers)
    print(f"Running {len(workloads)} workload(s) with {max_workers} workers "
          f"in '{args.work_dir}'")
    if isa_sim_args:
        print(f"sim args: {' '.join(isa_sim_args)}")
    if args.perf_est:
        print("perf est: enabled")

    slm = 0 # string length max (for stdout print)
    for w in workloads:
        slm = max(slm, len(get_test_name(w)))

    results = []
    t_start = time.time()
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = {
            executor.submit(
                run_one,
                args.isa_sim, app, isa_sim_args, args.work_dir, args.save_log,
                args.perf_est, args.timeout
            ): app for app in workloads
        }
        for future in as_completed(futures):
            r = future.result()
            results.append(r)
            pe_str = ""
            if args.perf_est:
                pe_str = f";  (perf_est: {r['perf_est_runtime']:.2f}s"
                if r['perf_est_error_msg']:
                    pe_str += ", failed"
                pe_str += ")"
            print(f"{INDENT}[{r['status']}] {r['name']:<{slm}} "
                  f"  ({r['runtime']:.2f}s)  {r['insts']}{pe_str}")

    total = time.time() - t_start

    passed = [r for r in results if r["status"] == "PASS"]
    failed = [r for r in results if r["status"] != "PASS"]
    perf_est_failed = [r for r in results if r["perf_est_error_msg"]]

    if failed:
        print("\nFailures:")
        for r in failed:
            why = f": {r['error_msg']}" if r["error_msg"] else ""
            print(f"{INDENT}{r['name']:<{slm}} [{r['status']}]{why}")
            print(r["fail_out"])

    if perf_est_failed:
        print("\nPerf est failures:")
        for r in perf_est_failed:
            print(f"{INDENT}{r['name']:<{slm}} {r['perf_est_error_msg']}")

    print(f"\n{len(passed)} passed / {len(failed)} failed. "
          f"Total: {total:.2f}s")

    sys.exit(1 if (failed or perf_est_failed) else 0)

if __name__ == "__main__":
    main()
