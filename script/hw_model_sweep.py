#!/usr/bin/env python3

import os
import subprocess
import json
import argparse
import numpy as np
import matplotlib.pyplot as plt
from dataclasses import dataclass
from typing import Dict, Any, Tuple, List

def get_reporoot():
    try:
        repo_root = subprocess.check_output(
            ["git", "rev-parse", "--show-toplevel"],
            stderr=subprocess.STDOUT
        ).strip().decode('utf-8')
        return repo_root
    except subprocess.CalledProcessError as e:
        print(f"Error: {e.output.decode('utf-8')}")
        return None

reporoot = get_reporoot()
SIM = os.path.join(reporoot, "src", "ama-riscv-sim")
PARAMS_DEF = os.path.join(reporoot, "script", "hw_model_sweep_params.json")
PASS_STRING = "    0x051e tohost   : 0x00000001"
INDENT = "  "

@dataclass
class sim_args:
    args: List[str]
    msg: str
    sweep_log: str
    out_dir: str
    save_sim: bool

def run_sim(sa: sim_args) -> Dict[str, Any]:
    res = subprocess.run([SIM] + sa.args, capture_output=True, text=True)

    if PASS_STRING not in res.stdout:
        raise RuntimeError("Simulation failed. Check the logs. Sim output: \n\n"
                           f"{res.stderr}"
                           f"{res.stdout}")

    with open(sa.sweep_log, "a") as f:
        f.write("\n\n" + sa.msg + "\n")
        if sa.save_sim:
            f.write(res.stdout)

    if not os.path.exists(sa.out_dir):
        raise FileNotFoundError(f"Output directory not found: {sa.out_dir}")

    json_hw_stats = os.path.join(sa.out_dir, "hw_stats.json")
    if not os.path.exists(json_hw_stats):
        raise FileNotFoundError(f"hw_stats.json not found: {json_hw_stats}")

    with open(json_hw_stats, "r") as f:
        hw_stats = json.load(f)

    return hw_stats

def run_cache_sweep(
    args: argparse.Namespace,
    sweep_params: Dict[str, Any],
    log_arg: List[str]) \
    -> None:

    if args.save_sim:
        test_name = args.bin.split("/")
        test_name = "_".join(test_name[-2:]).replace(".bin", "")
        sweep_log = f"hw_model_sweep_{args.sweep}_{test_name}.log"
        out_dir = f"out_{test_name}"
        if os.path.exists(sweep_log):
            os.remove(sweep_log)

    print(f"Sweep: {args.sweep}")
    cache = args.sweep
    ck = cache.capitalize() # cache key in the hw_stats dict
    sr = {} # sweep results dict
    for policy in sweep_params["policies"]:
        if args.track:
            print(INDENT, f"Policy: {policy}")
        # only LRU is supported atm
        sr[policy] = {}

        for set in sweep_params["sets"]:
            if args.track:
                print(INDENT * 2, f"Sets: {set}")
            arg_sets = [f"--{cache}_sets", str(set)]
            sr[policy][set] = {}

            for way in sweep_params["ways"]:
                if "max_size" in sweep_params:
                    max_size = int(sweep_params["max_size"])
                    if way * set * 64 > max_size:
                        #print("skipping: ", way, set)
                        continue

                arg_ways = [f"--{cache}_ways", str(way)]
                bp_act =  ["--bp_active", "none"]
                msg = f"==> SWEEP: policy: {policy}, set: {set}, way: {way} <=="
                sa = sim_args(
                    args=[args.bin] + arg_sets + arg_ways + log_arg + bp_act,
                    msg=msg,
                    sweep_log=sweep_log,
                    out_dir=out_dir,
                    save_sim=args.save_sim)
                hw_stats = run_sim(sa)
                sr[policy][set][way] = hw_stats
                if hw_stats[ck]["accesses"] == 0:
                    print(f"Warning: 0 accesses for {cache} with {way} ways "
                          f"and {set} sets. Logging might be disabled.")

                if args.track:
                    stats = hw_stats[ck]
                    hr = round(stats["hits"]/stats["accesses"]*100,2)
                    size = stats["size"]["data"]
                    print(INDENT * 3,
                          f"Ways: {way}, HR: {hr:.2f}%, Size: {size} B")

    if args.save_stats:
        sweep_results = sweep_log.replace(".log", "_results.json")
        with open(sweep_results, "w") as f:
            json.dump(sr, f)

    axs_hr = []
    # plot HR wrt num of ways
    fig, ax = plt.subplots(figsize=(12, 10))
    for policy, pe in sr.items():
        for set, se in pe.items():
            hit_rate = np.array(
                [se[way][ck]["hits"]/se[way][ck]["accesses"]*100
                 for way in se.keys()])
            ax.plot(se.keys(), hit_rate,
                    label=f"{policy} sets: {set}", marker="o", lw=1)
    ax.set_xlabel("Ways")
    ax.set_xticks(sweep_params["ways"])
    axs_hr.append(ax)

    # plot HR wrt cache size (excl tag & metadata)
    fig, ax = plt.subplots(figsize=(12, 10))
    for policy, pe in sr.items():
        for set, se in pe.items():
            hit_rate = np.array(
                [se[way][ck]["hits"]/se[way][ck]["accesses"]*100
                 for way in se.keys()])
            sizes = np.array(
                [se[way][ck]["size"]["data"]
                 for way in se.keys()])
            ax.plot(sizes, hit_rate,
                    label=f"{policy} sets: {set}", marker="o", lw=1)
    ax.set_xlabel("Size (B)")
    ax.set_xticks([2**i for i in range(6, int(np.log2(max_size)))])
    ax.set_xticklabels(ax.get_xticks(), rotation=45)
    axs_hr.append(ax)

    axs_ct = []
    for direction in ["reads", "writes"]:
        if cache == "icache" and direction == "writes":
            continue # icache has no writes
        # plot CT mem wrt num of ways
        fig, ax = plt.subplots(figsize=(12, 10))
        for policy, pe in sr.items():
            for set, se in pe.items():
                ct_mem_read = np.array(
                    [se[way][ck]["ct_mem"][direction]
                    for way in se.keys()])
                ax.plot(se.keys(), ct_mem_read,
                        label=f"{policy} sets: {set}", marker="o", lw=1)
        ax.set_xlabel("Ways")
        ax.set_xticks(sweep_params["ways"])
        ax.set_ylabel(f"Cache to Memory Traffic - {direction.capitalize()} (B)")
        ct_core = hw_stats[ck]["ct_core"][direction]
        ax.axhline(y=ct_core,color="r", linestyle="--",
                   label=f"Core to Cache Traffic = {ct_core} B")
        axs_ct.append(ax)

        # plot CT mem wrt cache size (excl tag & metadata)
        fig, ax = plt.subplots(figsize=(12, 10))
        for policy, pe in sr.items():
            for set, se in pe.items():
                ct_mem_read = np.array(
                    [se[way][ck]["ct_mem"][direction]
                    for way in se.keys()])
                sizes = np.array(
                    [se[way][ck]["size"]["data"]
                    for way in se.keys()])
                ax.plot(sizes, ct_mem_read,
                        label=f"{policy} sets: {set}", marker="o", lw=1)
        ax.set_xlabel("Size (B)")
        ax.set_xticks([2**i for i in range(6, int(np.log2(max_size)))])
        ax.set_xticklabels(ax.get_xticks(), rotation=45)
        ax.set_ylabel(f"Cache to Memory Traffic - {direction.capitalize()} (B)")
        ax.axhline(y=ct_core,color="r", linestyle="--",
                   label=f"Core to Cache Traffic = {ct_core} B")
        axs_ct.append(ax)

    # common for HR
    for a in axs_hr:
        a.set_ylabel("Hit Rate [%]")
        a.set_title(f"{ck} Sweep: {test_name}")
        a.legend(loc="lower right")
        ymin = a.get_ylim()[0]
        if args.plot_hr_thr:
            ymin = max(args.plot_hr_thr, ymin)
        a.set_ylim(ymin, 100.1) # make room for 100% markers to be fully visible

    # common for CT
    for a in axs_ct:
        a.set_title(f"{ck} Sweep: {test_name}")
        a.legend(loc="upper right")
        if args.plot_ct_thr:
            ymax = min(args.plot_ct_thr, a.get_ylim()[1])
            a.set_ylim(0, ymax)

    # common for all plots
    for a in axs_hr + axs_ct:
        a.grid(True)
        a.margins(x=0.02)

    # remove the out dir (has only the last run anyway)
    subprocess.run(["rm", "-r", out_dir])

# def run_bp_sweep():
#    pass

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Sweep through specified hardware models for a given app")
    parser.add_argument("bin", type=str, help="App binary to use for the sweep")
    parser.add_argument("--sweep", choices=["icache", "dcache"], help="Select the hardware model to sweep", required=True)
    parser.add_argument("--params", type=str, default=PARAMS_DEF, help="Path to the hardware model sweep params file")
    parser.add_argument("--sim_log_all", action="store_true", help="Log simulation from the boot")
    parser.add_argument("--save_sim", action="store_true", help="Save simulation stdout in a log file")
    parser.add_argument("--save_stats", action="store_true", help="Save combined simulation stats as json")
    parser.add_argument("--track", action="store_true", help="Print the sweep progress")
    parser.add_argument("--plot_hr_thr", type=int, default=None, help="Set the lower limit for the plot y-axis for hit rate")
    parser.add_argument("--plot_ct_thr", type=int, default=None, help="Set the upper limit for the plot y-axis for cache traffic")

    return parser.parse_args()

def run_main(args: argparse.Namespace) -> None:
    log_arg = []
    if args.sim_log_all:
        log_arg = ["--log_pc_start", "10000"]

    if not os.path.exists(args.bin):
        raise FileNotFoundError(f"Binary not found at: {args.bin}")

    if not os.path.exists(args.params):
        raise FileNotFoundError(f"Params file not found at: {args.params}")
    with open(args.params, "r") as f:
        sweep_dict = json.load(f)

    sweep_params = sweep_dict[args.sweep]
    if args.sweep == "icache" or args.sweep == "dcache":
        run_cache_sweep(args, sweep_params, log_arg)
    # TODO:
    # elif args.sweep == "bp":
    #     run_bp_sweep()

if __name__ == "__main__":
    if not os.path.exists(SIM):
        raise FileNotFoundError(f"Simulator no found at: {SIM}")
    args = parse_args()
    run_main(args)
