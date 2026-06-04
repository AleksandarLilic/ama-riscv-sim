#!/usr/bin/env python3

import argparse
import os
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed

from hw_model_sweep import get_test_name
from loop_run import load_config, resolve_workloads
from perf_est_v2 import main as pe_main
from perf_est_v2 import parse_args as pe_parse_args
from utils import INDENT


def parse_args():
    parser = argparse.ArgumentParser(description="Run perf_est correlation across a set of workloads")
    parser.add_argument("-c", "--config", required=True, help="Path to the YAML workloads config")
    parser.add_argument("--sim_dir", required=True, help="Directory with ISA sim outputs (<name>_out/)")
    parser.add_argument("--rtl_dir", required=True, help="Directory with RTL cosim outputs (<name>/<name>_out_cosim/)")
    parser.add_argument("-f", "--filter", type=str, nargs='*', help="Regex patterns to filter workloads by path")
    parser.add_argument("--max_workers", type=int, default=os.cpu_count())
    return parser.parse_args()

def get_paths(test_name, sim_dir, rtl_dir):
    sim_test_dir = os.path.join(sim_dir, f"{test_name}_out")
    rtl_test_dir = os.path.join(rtl_dir, test_name, f"{test_name}_out_cosim")
    return {
        "inst_profile": os.path.join(sim_test_dir, "inst_profile.json"),
        "hw_stats":     os.path.join(sim_test_dir, "hw_stats.json"),
        "rf_trace":     os.path.join(sim_test_dir, "rf_trace.bin"),
        "rtl_hw_stats": os.path.join(rtl_test_dir, "hw_stats.json"),
        "sim_test_dir": sim_test_dir,
        "rtl_test_dir": rtl_test_dir,
    }

def validate(test_name, sim_dir, rtl_dir):
    p = get_paths(test_name, sim_dir, rtl_dir)
    for d in [p["sim_test_dir"], p["rtl_test_dir"]]:
        if not os.path.isdir(d):
            raise ValueError(f"directory not found: {d}")

    files = [p["inst_profile"], p["hw_stats"], p["rf_trace"], p["rtl_hw_stats"]]
    for f in files:
        if not os.path.isfile(f):
            raise ValueError(f"file not found: {f}")
    return p

def run_one(wl, sim_dir, rtl_dir):
    test_name = get_test_name(wl)
    p = get_paths(test_name, sim_dir, rtl_dir)
    print(f"Running correlation for {test_name}")

    try:
        pe_main(
            pe_parse_args([
                p["inst_profile"], p["hw_stats"], p["rf_trace"],
                "-c", p["rtl_hw_stats"],
                "--save_corr_csv", "--silent",
            ])
        )
        return test_name, None

    except Exception as e:
        return test_name, str(e)

def main():
    args = parse_args()
    _, workloads = load_config(args.config)
    workloads = resolve_workloads(workloads, args.filter)

    for wl in workloads:
        test_name = get_test_name(wl)
        try:
            validate(test_name, args.sim_dir, args.rtl_dir)
        except ValueError as e:
            print(f"[ERROR] {e}")
            sys.exit(1)

    max_workers = min(os.cpu_count(), args.max_workers)
    print(f"Running {len(workloads)} workload(s) with {max_workers} workers")

    failures = []
    with ProcessPoolExecutor(max_workers=max_workers) as executor:
        futures = {
            executor.submit(run_one, wl, args.sim_dir, args.rtl_dir): wl
            for wl in workloads
        }
        for future in as_completed(futures):
            test_name, err = future.result()
            if err:
                failures.append((test_name, err))
                print(f"[FAIL] {test_name}")

    if failures:
        print("\nFailures:")
        for name, err in failures:
            print(f"{INDENT}{name}: {err}")

    sys.exit(1 if failures else 0)

if __name__ == "__main__":
    main()
