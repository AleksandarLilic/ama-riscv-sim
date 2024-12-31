#!/usr/bin/env python3

import os
import subprocess
import json
import argparse
import re
import concurrent.futures
import numpy as np
import matplotlib.pyplot as plt
from itertools import product
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
FIG_SIZE = (12, 10)

def get_test_name(bin: str) -> str:
    test_name = bin.split("/")
    return "_".join(test_name[-2:]).replace(".bin", "")

def get_out_dir(test_name: str) -> str:
    return f"out_{test_name}"

def gen_sweep_log_name(sweep: str, test_name: str) -> str:
    return f"sweep_{sweep}_{test_name}.log"

def within_size(num, size_lim: List[int]) -> bool:
    return size_lim[0] <= num <= size_lim[1]

def reformat_json(json_data, indent=4, max_depth=2):
    dumped_json = json.dumps(json_data, indent=indent)
    deeper_indent = " " * (indent * (max_depth + 1))
    parent_indent = " " * (indent * max_depth)
    out_json = re.sub(rf"\n{deeper_indent}", " ", dumped_json)
    out_json = re.sub(rf"\n{parent_indent}}},", " },", out_json)
    out_json = re.sub(rf"\n{parent_indent}}}", " }", out_json)
    return out_json

@dataclass
class sim_args:
    bin: str
    args: List[str]
    msg: str
    sweep_log: str
    save_sim: bool
    parallel: bool = False
    tag: str = ""
    ret_list: List = None # for parallel, anything needed to be returned

def run_sim(sa: sim_args) -> Dict[str, Any]:
    tag_arg = []
    if sa.tag:
        tag_arg = ["--out_dir_tag", sa.tag]
    cmd = [SIM] + [sa.bin] + sa.args + tag_arg
    res = subprocess.run(cmd, capture_output=True, text=True)

    if PASS_STRING not in res.stdout:
        raise RuntimeError("Simulation failed. Check the logs. Sim output: \n\n"
                           f"{cmd}\n"
                           f"{res.stderr}"
                           f"{res.stdout}")

    if not sa.parallel:
        with open(sa.sweep_log, "a") as f:
            f.write(sa.msg + "\n") # always write the msg
            if sa.save_sim:
                f.write(res.stdout + "\n\n")

    out_dir = get_out_dir(get_test_name(args.bin))
    if sa.tag:
        out_dir += f"_{sa.tag}"

    if not os.path.exists(out_dir):
        raise FileNotFoundError(f"Output directory not found: {out_dir}")

    json_hw_stats = os.path.join(out_dir, "hw_stats.json")
    if not os.path.exists(json_hw_stats):
        raise FileNotFoundError(f"hw_stats.json not found: {json_hw_stats}")

    with open(json_hw_stats, "r") as f:
        hw_stats = json.load(f)

    subprocess.run(["rm", "-r", out_dir])

    if sa.parallel:
        msg_out = f"{sa.msg}\n"
        if sa.save_sim:
            msg_out += f"{res.stdout}\n\n"
        return hw_stats, sa.ret_list, msg_out

    return hw_stats

def run_cache_sweep(
    args: argparse.Namespace,
    sweep_params: Dict[str, Any],
    log_arg: List[str]) \
    -> None:

    test_name = get_test_name(args.bin)
    sweep_log = gen_sweep_log_name(args.sweep, test_name)
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
                    bin=args.bin,
                    args=arg_sets + arg_ways + log_arg + bp_act,
                    msg=msg,
                    sweep_log=sweep_log,
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

    sweep_results = sweep_log.replace(".log", "_best.json")
    if args.load_stats:
        with open(sweep_results, "r") as f:
            sr = json.load(f)
    elif args.save_stats:
        with open(sweep_results, "w") as f:
            json.dump(sr, f)

    axs_hr = []
    # plot HR wrt num of ways
    fig, ax = plt.subplots(figsize=FIG_SIZE)
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
    fig, ax = plt.subplots(figsize=FIG_SIZE)
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
        fig, ax = plt.subplots(figsize=FIG_SIZE)
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
        fig, ax = plt.subplots(figsize=FIG_SIZE)
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
        a.set_title(f"{ck} Sweep: {test_name} - Hit Rate")
        a.legend(loc="lower right")
        ymin = a.get_ylim()[0]
        if args.plot_hr_thr:
            ymin = max(args.plot_hr_thr, ymin)
        a.set_ylim(ymin, 100.1) # make room for 100% markers to be fully visible

    # common for CT
    for a in axs_ct:
        a.set_title(f"{ck} Sweep: {test_name} - Cache to Memory Traffic")
        a.legend(loc="upper right")
        if args.plot_ct_thr:
            ymax = min(args.plot_ct_thr, a.get_ylim()[1])
            a.set_ylim(0, ymax)

    # common for all plots
    for a in axs_hr + axs_ct:
        a.grid(True)
        a.margins(x=0.02)

def get_bp_size(bp: str, params: Dict[str, Any]) -> int:
    bp = bp.replace("bp_", "", 1) # in case it's passed with the prefix

    if bp == "bimodal" or bp == "combined":
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

def gen_bp_sweep_params(bp, bp_sweep, size_lim) \
-> List[List[Tuple[str, Any]]]:
    sk = bp_sweep.keys()
    sk = [k for k in sk if "bits" in k] # only keys with 'bits' in the name

    bp_args = list(sk)
    # cartesian product of all the params
    bp_combs = product(*(bp_sweep[bp_arg] for bp_arg in bp_args))

    all = []
    for comb in bp_combs:
        if not within_size(get_bp_size(bp, dict(zip(bp_args, comb))), size_lim):
            if bp != "bp_static": # static is always included
                continue
        command = []
        for k, v in zip(bp_args, comb):
            command.append(f"--{bp}_{k}")
            command.append(str(v))
        all.append(command)

    return all

def run_bp_sweep(
    args: argparse.Namespace,
    sweep_params: Dict[str, Any],
    log_arg: List[str]) \
    -> None:

    test_name = get_test_name(args.bin)
    sweep_log = gen_sweep_log_name(args.sweep, test_name)
    if os.path.exists(sweep_log):
        os.remove(sweep_log)

    bpk = args.sweep.capitalize() # bp key in the hw_stats dict
    sr_bin = {}
    sr_best = {}

    SIZE_LIM = [int(sweep_params["common_settings"]["min_size"]),
                int(sweep_params["common_settings"]["max_size"])]
    if "size_bins" in sweep_params["common_settings"]:
        bins = sweep_params["common_settings"]["size_bins"]
    else:
        # create bins as powers of 2 within the size limits
        bins = [2**i for i
                in range(SIZE_LIM[0].bit_length(), SIZE_LIM[1].bit_length()+1)]

    save_bpp_for_combined = {}
    for bp in sweep_params.keys():
        if args.load_stats:
            break # skip the sweep if loading previous stats
        if bp == "common_settings":
            continue

        best = {}
        bp_act =  ["--bp_active", bp.replace("bp_", "", 1)]
        if bp == "bp_combined":
            bpc = sweep_params[bp]
            bpc_params = gen_bp_sweep_params(bp, bpc, SIZE_LIM)
            bp1 = sweep_params[bp]['predictors'][0]
            bp2 = sweep_params[bp]['predictors'][1]
            bp1_params = save_bpp_for_combined[bp1]
            bp2_params = save_bpp_for_combined[bp2]

            bp_params_list = [
                bp1_params[bp1_k] + bp2_params[bp2_k]
                for bp1_k, bp2_k
                in product(bp1_params, bp2_params)
                if within_size(bp1_k + bp2_k, SIZE_LIM) # keys are sizes
            ]
            merged = [cmd1 + cmd2
                      for cmd1, cmd2 in product(bpc_params, bp_params_list)]

            bp1_arg = [f"--bp_combined_p1", bp1.replace("bp_", "", 1)]
            bp2_arg = [f"--bp_combined_p2", bp2.replace("bp_", "", 1)]
            for m in merged:
                # add the combined predictor args into each command
                m.extend(bp1_arg)
                m.extend(bp2_arg)
            bp_params = merged
        else:
            save_bpp_for_combined[bp] = {}
            bp_params = gen_bp_sweep_params(bp, sweep_params[bp], SIZE_LIM)

        if args.track:
            print(INDENT, f"BP: {bp}, configs: {len(bp_params)}")

        with concurrent.futures.ProcessPoolExecutor() as executor:
            #print(f"Running {bp} with {executor._max_workers} processes")
            # run in parallel
            futures = {executor.submit(
                run_sim,
                sim_args(
                    bin=args.bin,
                    args=bpp + log_arg + bp_act,
                    msg=f"==> SWEEP: {bp} {bpp} <==",
                    sweep_log=sweep_log,
                    save_sim=args.save_sim,
                    parallel=True,
                    tag="".join(bpp[1::2]), # every even index is a number
                    ret_list=bpp
                    )
                ): bpp for bpp in bp_params
            }

        # get results as they come in
        for future in concurrent.futures.as_completed(futures):
            hw_stats, bpp, msg = future.result()
            # check results
            bp_size = hw_stats[bpk]['size']

            # FIXME: should be resolved before running sim instead
            if bp == "bp_combined" and not within_size(bp_size, SIZE_LIM):
                continue

            with open(sweep_log, "a") as f:
                f.write(msg)

            d = {}
            for i in range(0,len(bpp),2):
                d[bpp[i].replace("--", "", 1)] = bpp[i+1]

            b_pred = hw_stats[bpk]['predicted']
            b_tot = hw_stats[bpk]['branches']
            acc = round(b_pred/b_tot*100, 2)
            if bp_size not in best or acc > best[bp_size]["acc"]:
                best[bp_size] = {}
                for k, v in d.items():
                    best[bp_size][k] = v
                best[bp_size]["acc"] = acc
                if bp != "bp_combined":
                    save_bpp_for_combined[bp][bp_size] = bpp

        # sort the best dict by accuracy
        sr_best[bp] = dict(
            sorted(best.items(), key=lambda x: x[1]["acc"], reverse=True))
        # get the top N configs
        sr_best[bp] = dict(list(sr_best[bp].items())[:args.bp_top_num])
        # sort the best dict by size
        sr_best[bp] = dict(sorted(sr_best[bp].items(), key=lambda x: x[0]))

        # for each bin, find the best config for accuracy
        prev_bin = 0
        best_binned = {}
        for bin in bins:
            for bp_size, d in best.items():
                if bp_size < bin and bp_size >= prev_bin:
                    if (bin not in best_binned or \
                        d["acc"] > best_binned[bin]["acc"]):
                        best_binned[bin] = d.copy() # so size is not added to og
                        best_binned[bin]["size"] = bp_size
            prev_bin = bin

        sr_bin[bp] = best_binned

    sr_bin_out = sweep_log.replace(".log", "_binned.json")
    sr_best_out = sweep_log.replace(".log", "_best.json")
    if args.load_stats:
        with open(sr_bin_out, "r") as f:
            sr_bin = json.load(f)
        with open(sr_best_out, "r") as f:
            sr_best = json.load(f)
    elif args.save_stats:
        with open(sr_bin_out, "w") as f:
            f.write(reformat_json(sr_bin))
        with open(sr_best_out, "w") as f:
            f.write(reformat_json(sr_best))

    # plot accuracy wrt size
    axs = []
    for sr in [sr_bin, sr_best]:
        fig, ax = plt.subplots(figsize=FIG_SIZE)
        for bp, entry in sr.items():
            if "static" in bp:
                fk = list(entry.keys())[0] # first key
                ax.axhline(y=entry[fk]["acc"], color="r", linestyle="--",
                           label=f"{bp}, accuracy: {entry[fk]['acc']}%")
                continue

            accs = [entry[s]["acc"] for s in entry.keys()]
            if sr == sr_bin: # bins are keys, sizes stored separately
                sizes = [entry[s]["size"] for s in entry.keys()]
                title_add = "best per size bin"
                ax.plot(sizes, accs, label=f"{bp}", marker="o", lw=1)
            else: # sizes are keys
                sizes = [int(s) for s in entry.keys()] # chars when load_stats
                title_add = f"top {args.bp_top_num} for accuracy"
                ax.scatter(sizes, accs, label=f"{bp}", marker="o", s=40)

        ax.set_title(f"{bpk} Sweep: {test_name} - {title_add}")
        axs.append(ax)

    for a in axs:
        a.set_xlim(SIZE_LIM)
        a.set_xlabel("Size (B)")
        a.set_xticks(bins)
        a.set_xticklabels(a.get_xticks(), rotation=45)
        a.set_ylabel("Accuracy [%]")
        a.legend(loc="lower right")
        a.grid(True)
        a.margins(x=0.02)
        ymin = a.get_ylim()[0]
        if args.plot_acc_thr:
            ymin = max(args.plot_acc_thr, ymin)
        a.set_ylim(ymin, 100.1)

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
    parser.add_argument("--bp_top_num", type=int, default=16, help="Number of top branch predictor configs to keep based only on accuracy")

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

if __name__ == "__main__":
    if not os.path.exists(SIM):
        raise FileNotFoundError(f"Simulator not found at: {SIM}")
    args = parse_args()
    run_main(args)
