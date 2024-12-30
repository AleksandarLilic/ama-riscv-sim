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

def get_test_name(bin: str) -> str:
    test_name = bin.split("/")
    return "_".join(test_name[-2:]).replace(".bin", "")

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
        f.write("\n\n" + sa.msg + "\n") # always write the msg
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

    test_name = get_test_name(args.bin)
    sweep_log = f"hw_model_sweep_{args.sweep}_{test_name}.log"
    out_dir = f"out_{test_name}"
    if os.path.exists(sweep_log):
        os.remove(sweep_log)

    CACHE_LINE_BYTES = 64
    MAX_SIZE = int(sweep_params["max_size"])
    XTICKS = [2**i for i in range(int(np.log2(CACHE_LINE_BYTES)),
                                  int(np.log2(MAX_SIZE))+1)]
    cache = args.sweep
    ck = cache.capitalize() # cache key in the hw_stats dict
    sr = {} # sweep results dict
    for cpolicy in sweep_params["policies"]:
        if args.load_stats:
            break # skip the sweep if loading previous stats
        if args.track:
            print(INDENT, f"Policy: {cpolicy}")
        # only LRU is supported atm
        sr[cpolicy] = {}

        for cset in sweep_params["sets"]:
            if args.track:
                print(INDENT * 2, f"Sets: {cset}")
            arg_sets = [f"--{cache}_sets", str(cset)]
            sr[cpolicy][cset] = {}

            for cway in sweep_params["ways"]:
                if cway * cset * CACHE_LINE_BYTES > MAX_SIZE:
                    continue

                arg_ways = [f"--{cache}_ways", str(cway)]
                bp_act =  ["--bp_active", "none"]
                msg = f"==> SWEEP: " \
                      f"policy: {cpolicy}, set: {cset}, way: {cway} <=="
                sa = sim_args(
                    args=[args.bin] + arg_sets + arg_ways + log_arg + bp_act,
                    msg=msg,
                    sweep_log=sweep_log,
                    out_dir=out_dir,
                    save_sim=args.save_sim)
                hw_stats = run_sim(sa)
                sr[cpolicy][cset][cway] = hw_stats
                if hw_stats[ck]["accesses"] == 0:
                    print(f"Warning: 0 accesses for {cache} with {cway} ways "
                          f"and {cset} sets. Logging might be disabled.")

                if args.track:
                    stats = hw_stats[ck]
                    hr = round(stats["hits"]/stats["accesses"]*100,2)
                    size = stats["size"]["data"]
                    print(INDENT * 3,
                          f"Ways: {cway}, HR: {hr:.2f}%, Size: {size} B")

    sweep_results = sweep_log.replace(".log", "_results.json")
    if args.load_stats:
        with open(sweep_results, "r") as f:
            sr = json.load(f)
    elif args.save_stats:
        with open(sweep_results, "w") as f:
            json.dump(sr, f)

    axs_hr = []
    # plot HR wrt num of ways
    fig, ax = plt.subplots(figsize=(12, 10))
    for cpolicy, pe in sr.items():
        for cset, se in pe.items():
            hit_rate = np.array(
                [se[way][ck]["hits"]/se[way][ck]["accesses"]*100
                 for way in se.keys()])
            ax.plot(se.keys(), hit_rate,
                    label=f"{cpolicy} sets: {cset}", marker="o", lw=1)
    ax.set_xlabel("Ways")
    ax.set_xticks(sweep_params["ways"])
    axs_hr.append(ax)

    # plot HR wrt cache size (excl tag & metadata)
    fig, ax = plt.subplots(figsize=(12, 10))
    for cpolicy, pe in sr.items():
        for cset, se in pe.items():
            hit_rate = np.array(
                [se[way][ck]["hits"]/se[way][ck]["accesses"]*100
                 for way in se.keys()])
            sizes = np.array(
                [se[way][ck]["size"]["data"]
                 for way in se.keys()])
            ax.plot(sizes, hit_rate,
                    label=f"{cpolicy} sets: {cset}", marker="o", lw=1)
    ax.set_xlabel("Size (B)")
    ax.set_xticks(XTICKS)
    ax.set_xticklabels(ax.get_xticks(), rotation=45)
    axs_hr.append(ax)

    axs_ct = []
    for direction in ["reads", "writes"]:
        if cache == "icache" and direction == "writes":
            continue # icache has no writes
        # plot CT mem wrt num of ways
        fig, ax = plt.subplots(figsize=(12, 10))
        for cpolicy, pe in sr.items():
            for cset, se in pe.items():
                ct_mem_read = np.array(
                    [se[way][ck]["ct_mem"][direction]
                    for way in se.keys()])
                ax.plot(se.keys(), ct_mem_read,
                        label=f"{cpolicy} sets: {cset}", marker="o", lw=1)
        ax.set_xlabel("Ways")
        ax.set_xticks(sweep_params["ways"])
        ax.set_ylabel(f"Cache to Memory Traffic - {direction.capitalize()} (B)")
        fk = list(se.keys())[0] # first key
        ct_core = se[fk][ck]["ct_core"][direction]
        ax.axhline(y=ct_core,color="r", linestyle="--",
                   label=f"Core to Cache Traffic = {ct_core} B")
        axs_ct.append(ax)

        # plot CT mem wrt cache size (excl tag & metadata)
        fig, ax = plt.subplots(figsize=(12, 10))
        for cpolicy, pe in sr.items():
            for cset, se in pe.items():
                ct_mem_read = np.array(
                    [se[way][ck]["ct_mem"][direction]
                    for way in se.keys()])
                sizes = np.array(
                    [se[way][ck]["size"]["data"]
                    for way in se.keys()])
                ax.plot(sizes, ct_mem_read,
                        label=f"{cpolicy} sets: {cset}", marker="o", lw=1)
        ax.set_xlabel("Size (B)")
        ax.set_xticks(XTICKS)
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

def get_bp_size(bp: str, params: Dict[str, Any]) -> int:
    bp = bp.replace("bp_", "") # in case it's passed with the prefix

    if bp == "bimodal":
        cnt_entries = 1 << (params["pc_bits"])
        return (cnt_entries*params["cnt_bits"] + 8) >> 3
    if bp == "local":
        cnt_entries = 1 << (params["hist_bits"])
        hist_entries = 1 << (params["pc_bits"])
        return (hist_entries*params["hist_bits"] + \
                cnt_entries*params["cnt_bits"] + 8) >> 3
    if bp == "global":
        cnt_entries = 1 << (params["gr_bits"])
        idx_bits = params["gr_bits"]
        return (cnt_entries*params["cnt_bits"] + idx_bits + 8) >> 3
    if bp == "gselect":
        cnt_entries = 1 << (params["gr_bits"] + params["pc_bits"])
        return (cnt_entries*params["cnt_bits"] + params["gr_bits"] + 8) >> 3
    if bp == "gshare":
        idx_bits = max(params["pc_bits"], params["gr_bits"])
        cnt_entries = 1 << idx_bits
        return (cnt_entries*params["cnt_bits"] + params["gr_bits"] + 8) >> 3

    return 1 # can't help you

def run_bp_sweep(
    args: argparse.Namespace,
    sweep_params: Dict[str, Any],
    log_arg: List[str]) \
    -> None:

    test_name = get_test_name(args.bin)
    sweep_log = f"hw_model_sweep_{args.sweep}_{test_name}.log"
    out_dir = f"out_{test_name}"
    if os.path.exists(sweep_log):
        os.remove(sweep_log)

    bpk = args.sweep.capitalize() # bp key in the hw_stats dict
    sr = {} # sweep results dict

    if "size_bins" in sweep_params["common_settings"]:
        bins = sweep_params["common_settings"]["size_bins"]
    else:
        # create bins as powers of 2 from 1 to MAX_SIZE
        bins = [2**i for i in range(0, int(np.log2(MAX_SIZE))+1)]

    MAX_SIZE = int(sweep_params["common_settings"]["max_size"])
    # TODO: each predictor sweep can be run in parallel, sync before writing res
    for bp in sweep_params.keys():
        if args.load_stats:
            break # skip the sweep if loading previous stats
        if bp == "common_settings":
            continue
        if args.track:
            print(INDENT, f"BP: {bp}")
        bp_sweep = sweep_params[bp]
        sk = bp_sweep.keys()
        # drop any key that doesn't have 'bits' in the name
        sk = [k for k in sk if "bits" in k]

        best = {}
        bp_act =  ["--bp_active", bp.replace("bp_", "")]
        d = {}
        for s0 in bp_sweep[sk[0]]:
            arg_s0 = [f"--{bp}_{sk[0]}", str(s0)] if s0 else []
            if s0: d[sk[0]] = s0
            for s1 in bp_sweep[sk[1]]:
                arg_s1 = [f"--{bp}_{sk[1]}", str(s1)] if s1 else []
                if s1: d[sk[1]] = s1
                for s2 in bp_sweep[sk[2]]:
                    arg_s2 = [f"--{bp}_{sk[2]}", str(s2)] if s2 else []
                    if s2: d[sk[2]] = s2

                    if get_bp_size(bp, d) > int(MAX_SIZE):
                        continue

                    msg = f"==> SWEEP: {bp} {d} <=="
                    bp_param_args = arg_s0 + arg_s1 + arg_s2
                    sa = sim_args(
                        args=[args.bin] + bp_param_args + log_arg + bp_act,
                        msg=msg,
                        sweep_log=sweep_log,
                        out_dir=out_dir,
                        save_sim=args.save_sim)
                    hw_stats = run_sim(sa)
                    size = hw_stats[bpk]['size']
                    b_pred = hw_stats[bpk]['predicted']
                    b_tot = hw_stats[bpk]['branches']
                    acc = round(b_pred/b_tot*100, 2)
                    #if args.track: # too much noise
                    #    print(INDENT * 2,
                    #          f"{bp} {d}, Size: {size}, Acc: {acc}%")

                    if size not in best or acc > best[size]["acc"]:
                        best[size] = {}
                        for k, v in d.items():
                            if "no_bits" not in k:
                                best[size][k] = v
                        best[size]["acc"] = acc

        # for each bin, find the best config for accuracy
        prev_bin = 0
        best_binned = {}
        for bin in bins:
            for size, d in best.items():
                if size < bin and size >= prev_bin:
                    if (bin not in best_binned or \
                        d["acc"] > best_binned[bin]["acc"]):
                        best_binned[bin] = d
                        best_binned[bin]["size"] = size
            prev_bin = bin

        # sort the best dict by accuracy
        #sr[bp] = dict(sorted(best.items(), key=lambda x: x[1]["acc"]))
        sr[bp] = best_binned

    sweep_results = sweep_log.replace(".log", "_results.json")
    if args.load_stats:
        with open(sweep_results, "r") as f:
            sr = json.load(f)
    elif args.save_stats:
        with open(sweep_results, "w") as f:
            json.dump(sr, f)

    # plot accuracy wrt size
    fig, ax = plt.subplots(figsize=(12, 10))
    for bp, entry in sr.items():
        # if bp is static, use hline instead
        if "static" in bp:
            fk = list(entry.keys())[0] # first key
            ax.axhline(y=entry[fk]["acc"], color="r", linestyle="--",
                        label=f"{bp}, accuracy: {entry[fk]['acc']}%")
            continue
        sizes = [entry[s]["size"] for s in entry.keys()]
        accs = [entry[s]["acc"] for s in entry.keys()]
        ax.plot(sizes, accs, label=f"{bp}", marker="o", lw=1)

    ax.set_xlim(0, MAX_SIZE)
    ax.set_xlabel("Size (B)")
    ax.set_xticks(bins)
    ax.set_xticklabels(ax.get_xticks(), rotation=45)
    ax.set_ylabel("Accuracy [%]")
    ax.set_title(f"{bpk} Sweep: {test_name}")
    ax.legend(loc="lower right")
    ax.grid(True)
    ax.margins(x=0.02)
    ymin = ax.get_ylim()[0]
    if args.plot_acc_thr:
        ymin = max(args.plot_acc_thr, ymin)
    ax.set_ylim(ymin, 100.1) # make room for 100% markers to be fully visible

def parse_args() -> argparse.Namespace:
    SWEEP_CHOICES = ["icache", "dcache", "bpred"]
    parser = argparse.ArgumentParser(description="Sweep through specified hardware models for a given app")
    parser.add_argument("bin", type=str, help="App binary to use for the sweep")
    parser.add_argument("--sweep", choices=SWEEP_CHOICES, help="Select the hardware model to sweep", required=True)
    parser.add_argument("--params", type=str, default=PARAMS_DEF, help="Path to the hardware model sweep params file")
    parser.add_argument("--sim_log_all", action="store_true", help="Log simulation from the boot")
    parser.add_argument("--save_sim", action="store_true", help="Save simulation stdout in a log file")
    parser.add_argument("--save_stats", action="store_true", help="Save combined simulation stats as json")
    parser.add_argument("--load_stats", action="store_true", default=None, help="Load the previously saved stats from a json file instead of running the sweep. Ignores --save_stats")
    parser.add_argument("--track", action="store_true", help="Print the sweep progress")
    parser.add_argument("--plot_hr_thr", type=int, default=None, help="Set the lower limit for the plot y-axis for cache hit rate")
    parser.add_argument("--plot_ct_thr", type=int, default=None, help="Set the upper limit for the plot y-axis for cache traffic")
    parser.add_argument("--plot_acc_thr", type=int, default=None, help="Set the lower limit for the plot y-axis for branch predictor accuracy")

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
    print(f"Sweep: {args.sweep}")
    if "cache" in args.sweep:
        run_cache_sweep(args, sweep_params, log_arg)
    elif "bp" in args.sweep:
         run_bp_sweep(args, sweep_params, log_arg)

    out_dir = f"out_{get_test_name(args.bin)}"
    if os.path.exists(out_dir):
        # remove the out dir (has only the last run anyway)
        subprocess.run(["rm", "-r", out_dir])

if __name__ == "__main__":
    if not os.path.exists(SIM):
        raise FileNotFoundError(f"Simulator no found at: {SIM}")
    args = parse_args()
    run_main(args)
