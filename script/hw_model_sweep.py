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
from run_analysis import is_notebook

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
APPS_DIR = os.path.join(reporoot, "sw", "baremetal")
#PASS_STRING = "    0x051e tohost   : 0x00000001"
PASS_STRING = "    0x051e (tohost): 0x1"
INDENT = "  "
FIG_SIZE = (12, 10)

def get_test_name(app: str) -> str:
    test_name = app.split("/")
    return "_".join(test_name[-2:]).replace(".bin", "")

def get_out_dir(test_name: str) -> str:
    return f"out_{test_name}"

def gen_sweep_log_name(sweep: str, workloads: List) -> str:
    if len(workloads) > 1:
        sweep_name = "workload_sweep"
    else:
        sweep_name = get_test_name(workloads[0]['app'])
    return sweep_name, f"sweep_{sweep}_{sweep_name}.log"

def within_size(num, size_lim: List[int]) -> bool:
    return size_lim[0] <= num <= size_lim[1]

def reformat_json(json_data, max_depth=2, indent=4):
    dumped_json = json.dumps(json_data, indent=indent)
    deeper_indent = " " * (indent * (max_depth + 1))
    parent_indent = " " * (indent * max_depth)
    out_json = re.sub(rf"\n{deeper_indent}", " ", dumped_json)
    out_json = re.sub(rf"\n{parent_indent}}},", " },", out_json)
    out_json = re.sub(rf"\n{parent_indent}}}", " }", out_json)
    return out_json

def convert_keys_to_int(obj):
    if isinstance(obj, dict):
        return {int(k) if k.isdigit() else k: convert_keys_to_int(v)
                for k, v in obj.items()}
    elif isinstance(obj, list):
        return [convert_keys_to_int(i) for i in obj]
    else:
        return obj

def run_sim(cmd: List) -> Dict[str, Any]:
    res = subprocess.run(cmd, capture_output=True, text=True)
    if PASS_STRING not in res.stdout:
        raise RuntimeError("Simulation failed. Check the logs. Sim output: \n\n"
                           f"{cmd}\n {res.stderr}\n {res.stdout}")
    return res

@dataclass
class workload_params:
    workloads: List[Dict[str, Any]]
    sweep: str
    args: List[str]
    msg: str
    save_sim: bool
    ignore_thr: bool = False
    tag: str = ""
    ret_list: List = None # for parallel, anything needed to be returned

def run_workloads(wp: workload_params):
    tag_arg = []
    if wp.tag:
        tag_arg = ["--out_dir_tag", wp.tag]

    bp_sweep = (wp.sweep == "bpred")
    cache_sweep = (wp.sweep == "icache" or wp.sweep == "dcache")
    if not bp_sweep and not cache_sweep:
        raise ValueError("Unknown sweep type")

    if bp_sweep:
        agg_acc = [] # aggregate accuracy across all workloads
        bp_size = None
    elif cache_sweep:
        agg_hr = [] # aggregate hit rate across all workloads
        cache_size = None
        cache_ct = []

    msg_out = f"{wp.msg}\n"
    for workload in wp.workloads:
        app = workload["app"]
        wl_args = workload["args"]
        cmd = [SIM] + [app] + wp.args + tag_arg + wl_args
        res = run_sim(cmd)

        if wp.save_sim:
            msg_out += f"{res.stdout}\n\n"

        out_dir = get_out_dir(get_test_name(app))
        if wp.tag:
            out_dir += f"_{wp.tag}"

        if not os.path.exists(out_dir):
            raise FileNotFoundError(f"Output directory not found: {out_dir}")

        json_hw_stats = os.path.join(out_dir, "hw_stats.json")
        if not os.path.exists(json_hw_stats):
            raise FileNotFoundError(f"hw_stats.json not found: {json_hw_stats}")

        with open(json_hw_stats, "r") as f:
            hw_stats = json.load(f)

        subprocess.run(["rm", "-r", out_dir])

        if bp_sweep:
            if bp_size is None: # bp, and therefore size, is constant
                bp_size = hw_stats[wp.sweep]['size']
            app_thr_acc = 0 # per app threshold, if any
            if "thr" in workload:
                app_thr_acc = workload["thr"]["thr_bpred_acc"]
            b_pred = hw_stats[wp.sweep]['predicted']
            b_tot = hw_stats[wp.sweep]['branches']
            acc = round(b_pred/b_tot*100, 2)
            if acc < app_thr_acc and not wp.ignore_thr:
                agg_acc = []
                break # skip the rest of the workloads if acc is too low on any
            agg_acc.append(acc)

        elif cache_sweep:
            if cache_size is None:
                cache_size = hw_stats[wp.sweep]['size']['data']
            app_thr_hr = 0
            if "thr" in workload:
                app_thr_hr = workload["thr"][f"thr_{wp.sweep}_hr"]
            hits = hw_stats[wp.sweep]['hits']
            accesses = hw_stats[wp.sweep]['accesses']
            hr = round(hits/accesses*100, 2)
            if hr < app_thr_hr and not wp.ignore_thr:
                agg_hr = []
                break
            agg_hr.append(hr)
            if len(wp.workloads) == 1:
                cache_ct.append(hw_stats[wp.sweep]['ct_core']['reads'])
                cache_ct.append(hw_stats[wp.sweep]['ct_core']['writes'])
                cache_ct.append(hw_stats[wp.sweep]['ct_mem']['reads'])
                cache_ct.append(hw_stats[wp.sweep]['ct_mem']['writes'])

    if bp_sweep:
        avg_acc = None
        if len(agg_acc) > 0:
            avg_acc = round(sum(agg_acc)/len(agg_acc), 2)
        return bp_size, avg_acc, wp.ret_list, msg_out

    elif cache_sweep:
        avg_hr = None
        if len(agg_hr) > 0:
            avg_hr = round(sum(agg_hr)/len(agg_hr), 2)
        return cache_size, avg_hr, cache_ct, wp.ret_list, msg_out

def run_cache_sweep(
    args: argparse.Namespace,
    workloads: List[str],
    sweep_params: Dict[str, Any]) \
    -> None:

    sweep_name, sweep_log = gen_sweep_log_name(args.sweep, workloads)
    if os.path.exists(sweep_log):
        os.remove(sweep_log)

    CACHE_LINE_BYTES = 64
    MAX_SIZE = int(sweep_params["max_size"])
    XTICKS = [2**i for i in range(int(np.log2(CACHE_LINE_BYTES)),
                                  int(np.log2(MAX_SIZE))+1)]

    ck = args.sweep # cache key in the hw_stats dict
    sr = {} # sweep results dict
    bp_act =  ["--bp_active", "none"] # so there is no interference on caches
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
            arg_sets = [f"--{ck}_sets", str(cset)]
            sr[cpolicy][cset] = {}

            for cway in sweep_params["ways"]:
                if cway * cset * CACHE_LINE_BYTES > MAX_SIZE:
                    continue

                arg_ways = [f"--{ck}_ways", str(cway)]
                msg = f"==> SWEEP: " \
                      f"policy: {cpolicy}, set: {cset}, way: {cway} <=="
                wp = workload_params(
                    workloads=workloads,
                    sweep=ck,
                    args=arg_sets + arg_ways + bp_act,
                    msg=msg,
                    save_sim=args.save_sim,
                    ignore_thr=False,
                    tag=f"{cpolicy}_{cset}_{cway}",
                    ret_list=[cpolicy, cset, cway]
                )
                size, hr, ct, _, msg = run_workloads(wp)
                if hr == None:
                    continue
                with open(sweep_log, "a") as f:
                    f.write(msg)
                sr[cpolicy][cset][cway] = {
                    "hr": hr,
                    "size": size,
                    "ct_core": {"reads": ct[0], "writes": ct[1]},
                    "ct_mem": {"reads": ct[2], "writes": ct[3]}
                }
                if args.track:
                    print(INDENT * 3,
                          f"Ways: {cway}, HR: {hr:.2f}%, Size: {size} B")

    sweep_results = sweep_log.replace(".log", "_best.json")
    if args.load_stats:
        with open(sweep_results, "r") as f:
            sr = convert_keys_to_int(json.load(f))
    elif args.save_stats:
        with open(sweep_results, "w") as f:
            f.write(reformat_json(sr, max_depth=3))

    axs_hr = []
    # plot HR wrt num of ways
    fig, ax = plt.subplots(figsize=FIG_SIZE)
    for cpolicy, pe in sr.items():
        for cset, se in pe.items():
            hit_rate = np.array([se[way]["hr"] for way in se.keys()])
            ax.plot(se.keys(), hit_rate,
                    label=f"{cpolicy} sets: {cset}", marker="o", lw=1)
    ax.set_xlabel("Ways")
    ax.set_xticks(sweep_params["ways"])
    axs_hr.append(ax)

    # plot HR wrt cache size (excl tag & metadata)
    fig, ax = plt.subplots(figsize=FIG_SIZE)
    for cpolicy, pe in sr.items():
        for cset, se in pe.items():
            hit_rate = np.array([se[way]["hr"] for way in se.keys()])
            sizes = np.array([se[way]["size"] for way in se.keys()])
            ax.plot(sizes, hit_rate,
                    label=f"{cpolicy} sets: {cset}", marker="o", lw=1)
    ax.set_xlabel("Size (B)")
    ax.set_xticks(XTICKS)
    ax.set_xticklabels(ax.get_xticks(), rotation=45)
    axs_hr.append(ax)

    axs_ct = []
    # ct meaningless for workload sweeps
    if len(workloads) == 1:
        for direction in ["reads", "writes"]:
            if ck == "icache" and direction == "writes":
                continue # icache has no writes
            # plot CT mem wrt num of ways
            fig, ax = plt.subplots(figsize=FIG_SIZE)
            for cpolicy, pe in sr.items():
                for cset, se in pe.items():
                    ct_mem_read = np.array(
                        [se[way]["ct_mem"][direction]for way in se.keys()])
                    ax.plot(se.keys(), ct_mem_read,
                            label=f"{cpolicy} sets: {cset}", marker="o", lw=1)
            ax.set_xlabel("Ways")
            ax.set_xticks(sweep_params["ways"])
            ax.set_ylabel(
                f"Cache to Memory Traffic - {direction.capitalize()} (B)")
            fk = list(se.keys())[0] # first key
            ct_core = se[fk]["ct_core"][direction]
            ax.axhline(y=ct_core,color="r", linestyle="--",
                    label=f"Core to Cache Traffic = {ct_core} B")
            axs_ct.append(ax)

            # plot CT mem wrt cache size (excl tag & metadata)
            fig, ax = plt.subplots(figsize=FIG_SIZE)
            for cpolicy, pe in sr.items():
                for cset, se in pe.items():
                    ct_mem_read = np.array(
                        [se[way]["ct_mem"][direction]for way in se.keys()])
                    sizes = np.array(
                        [se[way]["size"]for way in se.keys()])
                    ax.plot(sizes, ct_mem_read,
                            label=f"{cpolicy} sets: {cset}", marker="o", lw=1)
            ax.set_xlabel("Size (B)")
            ax.set_xticks(XTICKS)
            ax.set_xticklabels(ax.get_xticks(), rotation=45)
            ax.set_ylabel(
                f"Cache to Memory Traffic - {direction.capitalize()} (B)")
            ax.axhline(y=ct_core,color="r", linestyle="--",
                    label=f"Core to Cache Traffic = {ct_core} B")
            axs_ct.append(ax)

    # common for HR
    for a in axs_hr:
        a.set_ylabel("Hit Rate [%]")
        a.set_title(f"{ck} sweep: {sweep_name} - Hit Rate")
        a.legend(loc="lower right")
        ymin = a.get_ylim()[0]
        if args.plot_hr_thr:
            ymin = max(args.plot_hr_thr, ymin)
        a.set_ylim(ymin, 100.1) # make room for 100% markers to be fully visible

    # common for CT
    for a in axs_ct:
        a.set_title(f"{ck} sweep: {sweep_name} - Cache to Memory Traffic")
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

    cmds = []
    sizes = []
    for comb in bp_combs:
        bp_size = get_bp_size(bp, dict(zip(bp_args, comb)))
        if not within_size(bp_size, size_lim):
            if bp != "bp_static": # static is always included
                continue
        command = []
        for k, v in zip(bp_args, comb):
            command.append(f"--{bp}_{k}")
            command.append(str(v))

        cmds.append(command)
        sizes.append(bp_size)

    return sizes, cmds

def run_bp_sweep(
    args: argparse.Namespace,
    workloads: List[str],
    sweep_params: Dict[str, Any]) \
    -> None:

    sweep_name, sweep_log = gen_sweep_log_name(args.sweep, workloads)
    if os.path.exists(sweep_log) and not args.load_stats:
        os.remove(sweep_log)

    bpk = args.sweep # bp key in the hw_stats dict
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
            bpc_sizes, bpc_params = gen_bp_sweep_params(bp, bpc, SIZE_LIM)
            bp1 = sweep_params[bp]['predictors'][0]
            bp2 = sweep_params[bp]['predictors'][1]
            bp1_params = save_bpp_for_combined[bp1]
            bp2_params = save_bpp_for_combined[bp2]

            bp_params_list = [
                bp1_params[bp1_k] + bp2_params[bp2_k] + bpc_p
                for bp1_k, bp2_k, (bpc_s, bpc_p)
                in product(bp1_params, bp2_params, zip(bpc_sizes, bpc_params))
                if within_size(bp1_k + bp2_k + bpc_s, SIZE_LIM) # keys are sizes
            ]

            bp1_arg = [f"--bp_combined_p1", bp1.replace("bp_", "", 1)]
            bp2_arg = [f"--bp_combined_p2", bp2.replace("bp_", "", 1)]
            for m in bp_params_list:
                # add the combined predictor args into each command
                m.extend(bp1_arg)
                m.extend(bp2_arg)
            bp_params = bp_params_list
        else:
            save_bpp_for_combined[bp] = {}
            _, bp_params = gen_bp_sweep_params(bp, sweep_params[bp], SIZE_LIM)

        if args.track:
            print(INDENT, f"BP: {bp}, configs: {len(bp_params)}")

        with concurrent.futures.ProcessPoolExecutor() as executor:
            futures = {
                executor.submit(
                    run_workloads,
                    workload_params(
                        workloads=workloads,
                        sweep=bpk,
                        args=bpp + bp_act,
                        msg=f"==> SWEEP: {bp} {bpp} <==",
                        save_sim=args.save_sim,
                        ignore_thr=(bp == "bp_static"),
                        tag="".join(bpp[1::2]), # every even index is a number
                        ret_list=bpp
                    )
                ): bpp for bpp in bp_params
            }

        # get results as they come in
        for future in concurrent.futures.as_completed(futures):
            bp_size, acc, bpp, msg = future.result()
            with open(sweep_log, "a") as f:
                f.write(msg)
            if acc == None:
                continue
            d = {}
            for i in range(0,len(bpp),2):
                d[bpp[i].replace("--", "", 1)] = bpp[i+1]
            if (bp_size not in best or acc > best[bp_size]["acc"]) or \
                bp == "bp_static":
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
        # sort again in ascending order (more intuitive)
        sr_best[bp] = dict(
            sorted(sr_best[bp].items(), key=lambda x: x[1]['acc']))
        # sort the best dict by size
        #sr_best[bp] = dict(sorted(sr_best[bp].items(), key=lambda x: x[0]))

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
            sr_bin = convert_keys_to_int(json.load(f))
        with open(sr_best_out, "r") as f:
            sr_best = convert_keys_to_int(json.load(f))
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
            if len(entry.keys()) == 0: # below thr limit for all available sizes
                continue

            fmt = lambda x: x.replace("bp_", "", 1)
            label = fmt(bp)
            if bp == "bp_combined":
                bp1 = fmt(sweep_params[bp]['predictors'][0])
                bp2 = fmt(sweep_params[bp]['predictors'][1])
                label = f"{label}\n{bp1} & {bp2}"

            if "static" in bp:
                fk = list(entry.keys())[0] # first key
                ax.axhline(y=entry[fk]["acc"], color="r", linestyle="--",
                           label=f"{label} ({entry[fk]['acc']:.0f}%)")
                continue

            accs = [entry[s]["acc"] for s in entry.keys()]
            if sr == sr_bin: # bins are keys, sizes stored separately
                sizes = [entry[s]["size"] for s in entry.keys()]
                title_add = "best per size bin"
                ax.plot(sizes, accs, label=label, marker="o", lw=1)
            else: # sizes are keys
                sizes = entry.keys()
                title_add = f"top {args.bp_top_num} for accuracy"
                ax.scatter(sizes, accs, label=label, marker="o", s=40)

        ax.set_title(f"{bpk} sweep: {sweep_name} - {title_add}")
        axs.append(ax)

    for a in axs:
        a.set_xlim(SIZE_LIM)
        a.set_xlabel("Size (B)")
        a.set_xticks(bins)
        a.set_xticklabels(a.get_xticks(), rotation=45)
        a.set_ylabel("Accuracy [%]")
        a.legend(loc="upper left")
        a.grid(True)
        a.margins(x=0.02)
        ymin = a.get_ylim()[0]
        if args.plot_acc_thr:
            ymin = max(args.plot_acc_thr, ymin)
        a.set_ylim(ymin, 100.1)

    plt.show(block=False)
    if not is_notebook():
        input("Press Enter to close all plots...")
        plt.close('all')

def parse_args() -> argparse.Namespace:
    SWEEP_CHOICES = ["icache", "dcache", "bpred"]
    parser = argparse.ArgumentParser(description="Sweep through specified hardware models for a given app")
    parser.add_argument("--sweep", choices=SWEEP_CHOICES, help="Select the hardware model to sweep", required=True)
    parser.add_argument("--params", type=str, default=PARAMS_DEF, help="Path to the hardware model sweep params file")
    parser.add_argument("--save_sim", action="store_true", help="Save simulation stdout in a log file")
    parser.add_argument("--save_stats", action="store_true", help="Save combined simulation stats as json")
    parser.add_argument("--load_stats", action="store_true", default=None, help="Load the previously saved stats from a json file instead of running the sweep. Ignores --save_stats")
    parser.add_argument("--track", action="store_true", help="Print the sweep progress")
    parser.add_argument("--plot_hr_thr", type=int, default=None, help="Set the lower limit for the plot y-axis for cache hit rate")
    parser.add_argument("--plot_ct_thr", type=int, default=None, help="Set the upper limit for the plot y-axis for cache traffic")
    parser.add_argument("--plot_acc_thr", type=int, default=None, help="Set the lower limit for the plot y-axis for branch predictor accuracy")
    parser.add_argument("--bp_top_num", type=int, default=32, help="Number of top branch predictor configs to keep based only on accuracy")

    return parser.parse_args()

def run_main(args: argparse.Namespace) -> None:
    if not os.path.exists(args.params):
        raise FileNotFoundError(f"Params file not found at: {args.params}")

    with open(args.params, "r") as flavor:
        sweep_dict = json.load(flavor)

    workloads = []
    workloads_dict = sweep_dict["workloads"]
    for wl,flavors in workloads_dict.items():
        for flavor in flavors[0]:
            binary = os.path.join(APPS_DIR, wl, f"{flavor}.bin")
            if not os.path.exists(binary):
                raise FileNotFoundError(f"Binary not found at: {binary}")
            else:
                wl_args = flavors[2]["sim_args"].split(" ") # args as list
                workloads.append({
                    "app": binary,
                    "thr": flavors[1],
                    "args": wl_args
                })

    if len(workloads) == 0:
        raise FileNotFoundError("No workloads found")

    sweep_params = sweep_dict["hw_params"][args.sweep]
    print(f"Sweep: {args.sweep} with {len(workloads)} workloads")
    if "cache" in args.sweep:
        run_cache_sweep(args, workloads, sweep_params)
    elif "bp" in args.sweep:
        run_bp_sweep(args, workloads, sweep_params)

if __name__ == "__main__":
    if not os.path.exists(SIM):
        raise FileNotFoundError(f"Simulator not found at: {SIM}")
    args = parse_args()
    run_main(args)
