#!/usr/bin/env python3

import argparse
import glob
import json
import os
import re
import subprocess
import time
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass
from datetime import timedelta
from itertools import chain, combinations, product
from typing import Any, Dict, List, Tuple

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from utils import get_reporoot, is_headless, is_notebook

# globals
reporoot = get_reporoot()
SIM = os.path.join(reporoot, "src", "ama-riscv-sim")
APPS_DIR = os.path.join(reporoot, "sw", "baremetal")
SIM_PASS_STRING = "    0x051e tohost    : 0x00000001"
SIM_EARLY_EXIT_STRING = "    0x051e tohost    : 0xf0000000"
INDENT = "  "
FIG_SIZE = (8, 8)
MK = [["o", 6, 25], ["^", 6, 25]]
LW = .5

BP_COLORS = {
    "static": "k", # not used
    "bimodal": "#4275b1", # blue
    "local": "#ea862a", # orange
    "global": "#579e3a", # green
    "gselect": "#c03a2d", # dark red
    "gshare": "#8c69b9", # purple
}

CACHE_LINE_BYTES = 64

# utility functions
class track_time:
    def __init__(self) -> None:
        self._last = time.time()

    def __call__(self):
        now = time.time()
        now_str = time.strftime("%Y-%m-%d %H-%M-%S")
        diff = now - self._last
        self._last = now
        return now_str, str(timedelta(seconds=diff)).split('.')[0]

def get_test_name(app: str) -> str:
    test_name = app.split("/")
    return "_".join(test_name[-2:]).replace(".elf", "")

def gen_sweep_log_name(
    sweep: str, work_dir: str, workloads: List, only_searched: bool) -> str:
    if len(workloads) > 1:
        if only_searched:
            sweep_name = "workloads_searched"
        else:
            sweep_name = "workloads_all"
    else:
        sweep_name = get_test_name(workloads[0]["app"])
    return sweep_name, os.path.join(work_dir, f"sweep_{sweep}_{sweep_name}.log")

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

def create_plot(
    title: str,
    sweep_name: str,
    ncols=2,
    nrows=1,
    title_2: str = "") \
    -> Tuple:

    fs = [FIG_SIZE[0]*ncols, FIG_SIZE[1]*nrows]
    fig, axs = plt.subplots(
        ncols=ncols, nrows=nrows, figsize=fs, constrained_layout=True,
        num=sweep_name)

    suptitle = f"{title} sweep for {sweep_name}"
    if title_2 != "":
        suptitle = f"{suptitle}: {title_2}"
    fig.suptitle(suptitle)

    if ncols == 1 and nrows == 1:
        axs = [axs]
    return fig, axs

def run_sim(cmd: List) -> Dict[str, Any]:
    res = subprocess.run(cmd, capture_output=True, text=True)
    if (SIM_PASS_STRING not in res.stdout and
        SIM_EARLY_EXIT_STRING not in res.stdout):
        raise RuntimeError("Simulation failed. Check the logs. Sim output: \n\n"
                           f"{cmd}\n {res.stderr}\n {res.stdout}")
    return res

# functions
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
    start_dir = os.getcwd()
    os.chdir(args.work_dir)
    for workload in wp.workloads:
        app = workload["app"]
        wl_args = workload["args"]
        cmd = [SIM] + [app] + wp.args + tag_arg + wl_args
        res = run_sim(cmd)

        if wp.save_sim:
            msg_out += f"{res.stdout}\n\n"

        out_dir = f"out_{get_test_name(app)}"
        if wp.tag:
            out_dir += f"_{wp.tag}"

        if not os.path.exists(out_dir):
            raise FileNotFoundError(f"Output directory not found: {out_dir}")

        json_hw_stats = os.path.join(out_dir, "hw_stats.json")
        if not os.path.exists(json_hw_stats):
            raise FileNotFoundError(f"hw_stats.json not found: {json_hw_stats}")

        with open(json_hw_stats, "r") as f:
            hw_stats = json.load(f)

        # TODO: no longer needed, outputs are required for combined bp prep
        #subprocess.run(["rm", "-r", out_dir])

        if bp_sweep:
            if bp_size is None: # bp, and therefore size, is constant
                bp_size = hw_stats[wp.sweep]["size"]
            app_thr_acc = 0 # per app threshold, if any
            if "thr" in workload:
                app_thr_acc = workload["thr"]["thr_bpred_acc"]
            b_pred = hw_stats[wp.sweep]["predicted"]
            b_tot = hw_stats[wp.sweep]["branches"]
            if b_tot == 0:
                raise RuntimeError(f"No branches logged for {app}")
            acc = round(b_pred/b_tot*100, 2)
            if acc < app_thr_acc and not wp.ignore_thr:
                agg_acc = []
                break # skip the rest of the workloads if acc is too low on any
            agg_acc.append(acc)

        elif cache_sweep:
            if cache_size is None:
                cache_size = hw_stats[wp.sweep]["size"]["data"]
            app_thr_hr = 0
            if "thr" in workload:
                app_thr_hr = workload["thr"][f"thr_{wp.sweep}_hr"]
            hits_d = hw_stats[wp.sweep]["hits"]
            hits = hits_d['reads'] + hits_d['writes']
            references = hw_stats[wp.sweep]["references"]
            if references == 0:
                raise RuntimeError(f"No cache references logged for {app}")
            hr = round(hits/references*100, 2)
            if hr < app_thr_hr and not wp.ignore_thr:
                agg_hr = []
                break
            agg_hr.append(hr)
            if len(wp.workloads) == 1:
                cache_ct.append(hw_stats[wp.sweep]["ct_core"]["reads"])
                cache_ct.append(hw_stats[wp.sweep]["ct_core"]["writes"])
                cache_ct.append(hw_stats[wp.sweep]["ct_mem"]["reads"])
                cache_ct.append(hw_stats[wp.sweep]["ct_mem"]["writes"])

    os.chdir(start_dir)
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
    sweep_params: Dict[str, Any],
    best_params: List[List[Tuple[str, Any]]] = None) \
    -> None:

    running_best = (best_params != None)
    multi_wl = len(workloads) > 1
    running_single_best = running_best and not multi_wl
    sweep_name, sweep_log = gen_sweep_log_name(
        args.sweep, args.work_dir, workloads, not running_best)
    if os.path.exists(sweep_log):
        os.remove(sweep_log)

    MAX_SIZE = int(sweep_params["max_size"])
    SIZE_LIM = [CACHE_LINE_BYTES, MAX_SIZE]
    XTICKS = [2**i for i in range(int(np.log2(CACHE_LINE_BYTES)),
                                  int(np.log2(MAX_SIZE))+1)]
    XLABELS = [f"{x//1024}K" if x >= 1024 else x for x in XTICKS]

    if running_best:
        cache_params = best_params
    else:
        _, cache_params = gen_cache_sweep_params(
            args.sweep, sweep_params, SIZE_LIM)

    ck = args.sweep # cache key in the hw_stats dict
    sr = {} # sweep results dict
    bp_act =  ["--bp", "none"] # so there is no impact on the caches

    if not args.load_stats:
        idx_c = 1
        PROG_BAT = 4 # progress batches
        if args.track and not running_single_best:
            tt = track_time()
            print(INDENT,
                f"Cache: {ck}, configs: {len(cache_params)}, progress: ",
                end="")
            idx_c = (len(cache_params) // PROG_BAT) + 1

        with ProcessPoolExecutor(max_workers=args.max_workers) as executor:
            futures = {
                executor.submit(
                    run_workloads,
                    workload_params(
                        workloads=workloads,
                        sweep=ck,
                        args=cp + bp_act,
                        msg= f"==> SWEEP: {ck} {cp} <==",
                        save_sim=args.save_sim,
                        ignore_thr=False,
                        tag="".join(cp[1::2]), # set (1), way (3), policy (5)
                        ret_list=cp
                    )
                ): cp for cp in cache_params
            }

            # get results as they come in
            cnt = 0
            for future in as_completed(futures):
                size, hr, ct, cp, msg = future.result()

                cnt += 1
                if args.track and cnt % idx_c == 0 and not running_single_best:
                    print(f"{cnt//idx_c}/{PROG_BAT}", end=", ", flush=True)

                if hr == None:
                    continue
                with open(sweep_log, "a") as f:
                    f.write(msg)
                cpolicy, cset, cway = cp[5], int(cp[1]), int(cp[3])
                if cpolicy not in sr:
                    sr[cpolicy] = {}
                if cset not in sr[cpolicy]:
                    sr[cpolicy][cset] = {}
                sr[cpolicy][cset][cway] = { "hr": hr, "size": size }
                if ct: # only for single workload sweeps
                    sr[cpolicy][cset][cway]["ct_core"] = {
                        "reads": ct[0], "writes": ct[1]}
                    sr[cpolicy][cset][cway]["ct_mem"] = {
                        "reads": ct[2], "writes": ct[3]}

        if args.track and not running_single_best:
            tt_now, tt_taken = tt()
            print(f"{PROG_BAT}/{PROG_BAT}. Done at {tt_now}. Taken: {tt_taken}")

        # for each policy, sort by the keys: first sets then ways
        for cpolicy, pe in sr.items():
            for cset, se in pe.items():
                sr[cpolicy][cset] = dict(
                    sorted(se.items(), key=lambda x: x[0]))
            sr[cpolicy] = dict(sorted(sr[cpolicy].items(), key=lambda x: x[0]))

    sweep_results = sweep_log.replace(".log", "_best.json")
    if args.load_stats:
        with open(sweep_results, "r") as f:
            sr = convert_keys_to_int(json.load(f))
    elif args.save_stats:
        with open(sweep_results, "w") as f:
            f.write(reformat_json(sr, max_depth=3))

    # plot HR wrt num of ways
    lbl = lambda x, y: f"{x}, sets: {y}"
    max_nrows = 3 if ck == "dcache" else 2
    nrows = max_nrows if len(workloads) == 1 else 1
    fig, axs = create_plot(
        ck.capitalize(), f"{sweep_name}", nrows=nrows, title_2=args.chart_title)
    axs_hr = axs[0] if len(workloads) == 1 else axs
    ax = axs_hr[0]
    for cpolicy, pe in sr.items():
        for cset, se in pe.items():
            hit_rate = np.array([se[way]["hr"] for way in se.keys()])
            ax.plot(se.keys(), hit_rate,
                    label=lbl(cpolicy, cset), marker=MK[0][0], lw=LW)
    ax.set_xlabel("Ways")
    ax.set_xticks(sweep_params["ways"])
    ax.set_title(f"Hit Rate vs Number of Ways")

    # plot HR wrt cache size (excl tag & metadata)
    ax = axs_hr[1]
    for cpolicy, pe in sr.items():
        for cset, se in pe.items():
            hit_rate = np.array([se[way]["hr"] for way in se.keys()])
            sizes = np.array([se[way]["size"] for way in se.keys()])
            ax.plot(sizes, hit_rate, label=lbl(cpolicy, cset),
                    marker=MK[0][0], lw=LW)
    ax.set_xlabel("Size [B]")
    ax.set_xticks(XTICKS)
    ax.set_xticklabels(XLABELS, rotation=45)
    ax.set_title(f"Hit Rate vs Size")

    axs_ct = []
    # ct meaningless for workload sweeps
    if len(workloads) == 1:
        ctmt = "Cache to Memory Traffic"
        ctct = "Core to Cache Traffic"
        for axs_d,direction in zip(axs[1:], ["reads", "writes"]):
            if ck == "icache" and direction == "writes":
                continue # icache has no writes
            # plot CT mem wrt num of ways
            ax = axs_d[0]
            fk = list(se.keys())[0] # first key
            ct_core = se[fk]["ct_core"][direction]
            prefix = ["", "K", "M", "G"]
            ct_core_s = int(ct_core)
            while ct_core_s > 1024:
                ct_core_s = round(ct_core_s/1024,2)
                prefix.pop(0)
            ct_core_str = f"{ct_core_s} {prefix[0]}B"

            for cpolicy, pe in sr.items():
                for cset, se in pe.items():
                    ct_mem_read = np.array(
                        [se[way]["ct_mem"][direction]for way in se.keys()])
                    ax.plot(se.keys(), ct_mem_read, label=lbl(cpolicy, cset),
                            marker=MK[0][0], lw=LW)
            ax.set_xlabel("Ways")
            ax.set_xticks(sweep_params["ways"])
            y_ct = ct_core
            if args.plot_no_ct_thr and ax.get_ylim()[1] < ct_core:
                y_ct = np.nan
            ax.axhline(y=y_ct, color="r", linestyle="--",
                       label=f"{ctct} = {ct_core_str}")
            scale_ax_for_ct(ax)
            ax.set_title(f"{ctmt} {direction.capitalize()} vs Number of Ways")
            axs_ct.append(ax)

            # plot CT mem wrt cache size (excl tag & metadata)
            ax = axs_d[1]
            for cpolicy, pe in sr.items():
                for cset, se in pe.items():
                    ct_mem_read = np.array(
                        [se[way]["ct_mem"][direction]for way in se.keys()])
                    sizes = np.array([se[way]["size"]for way in se.keys()])
                    ax.plot(sizes, ct_mem_read,
                            label=lbl(cpolicy, cset), marker=MK[0][0], lw=LW)
            ax.set_xlabel("Size [B]")
            ax.set_xticks(XTICKS)
            ax.set_xticklabels(XLABELS, rotation=45)
            ax.axhline(y=y_ct, color="r", linestyle="--",
                       label=f"{ctct} = {ct_core_str}")
            scale_ax_for_ct(ax)
            ax.set_title(f"{ctmt} {direction.capitalize()} vs Size")
            axs_ct.append(ax)

    # common for HR
    for a in axs_hr:
        a.set_ylabel("Hit Rate [%]")
        a.legend(loc="lower right")
        ymin = min(a.get_ylim()[0], 99)
        if args.plot_hr_thr:
            ymin = max(args.plot_hr_thr, ymin)
        a.set_ylim(ymin, 100.1) # make room for 100% markers to be fully visible

    # common for CT
    for a in axs_ct:
        a.legend(loc="upper right")
        a.set_ylabel(f"Traffic [B]")
        if args.plot_ct_thr:
            ymax = min(args.plot_ct_thr, a.get_ylim()[1])
            a.set_ylim(0, ymax)

    # common for all plots
    for a in list(axs_hr) + axs_ct:
        a.grid(True)
        a.margins(x=0.02)

    if args.save_png:
        fig.savefig(sweep_log.replace(".log", ".png"))

    return sr

def scale_ax_for_ct(ax):
    yticks = ax.get_yticks()
    while yticks[0] < 0: # remove negative if present
        yticks = yticks[1:]

    y_min, y_max = min(yticks), max(yticks)
    y_range = y_max - y_min

    if y_range < 1024:
        unit = ''
        scale = 1
    elif y_range < 1024**2:
        unit = 'K'
        scale = 1024
    else:
        unit = 'M'
        scale = 1024**2

    y_maxn = (2 ** (int(np.ceil(np.log2(y_max))))) / scale
    yticks_out = np.arange(0, y_maxn + 1, y_maxn/8)
    while (yticks_out[-1] * scale) > y_max: # remove until it matches the og max
        yticks_out = yticks_out[:-1]

    inc_precision = (yticks_out[-1] < 8)
    ax.set_yticks(yticks_out * scale)
    if inc_precision:
        ax.set_yticklabels([f"{round(tick,1)}{unit}" for tick in yticks_out])
    else:
        ax.set_yticklabels([f"{int(tick)}{unit}" for tick in yticks_out])

def get_cache_size(params: Dict[str, Any]) -> int:
    return params["sets"] * params["ways"] * CACHE_LINE_BYTES

def gen_cache_sweep_params(cache, cache_sweep, size_lim) \
-> List[List[Tuple[str, Any]]]:
    sk = cache_sweep.keys()
    sk = [k for k in sk if "ways" in k or "sets" in k or "policy" in k]

    cache_args = list(sk)
    # cartesian product of all the params
    cache_combs = product(*(cache_sweep[cache_arg] for cache_arg in cache_args))

    cmds = []
    sizes = []
    for comb in cache_combs:
        cache_size = get_cache_size(dict(zip(cache_args, comb)))
        if not within_size(cache_size, size_lim):
            continue
        command = []
        for k, v in zip(cache_args, comb):
            command.append(f"--{cache}_{k}")
            command.append(str(v))

        cmds.append(command)
        sizes.append(cache_size)

    return sizes, cmds

def gen_cache_final_params(cache, sr: Dict) -> List[List[Tuple[str, Any]]]:
    cmds = []
    for policy, pe in sr.items():
        for sets, se in pe.items():
            for ways, we in se.items():
                command = []
                # order is important when evaluating ret from futures
                command.append(f"--{cache}_sets")
                command.append(f"{sets}")
                command.append(f"--{cache}_ways")
                command.append(f"{ways}")
                command.append(f"--{cache}_policy")
                command.append(f"{policy}")
                cmds.append(command)
    return cmds

def get_bp_size(bp: str, params: Dict[str, Any]) -> int:
    bp = bp.replace("bp_", "", 1) # in case it's passed with the prefix

    if bp == "static":
        return 0
    if bp == "bimodal" or bp == "combined":
        cnt_entries = 1 << (params["pc_bits"])
        return (cnt_entries*params["cnt_bits"] + 8) >> 3
    if bp == "local":
        cnt_entries = 1 << (params["lhist_bits"])
        hist_entries = 1 << (params["pc_bits"])
        return (hist_entries*params["lhist_bits"] + \
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
    sk = [k for k in sk if "bits" in k or "method" in k]

    bp_args = list(sk)
    # cartesian product of all the params
    bp_products = product(*(bp_sweep[bp_arg] for bp_arg in bp_args))

    cmds = []
    sizes = []
    for bp_prod in bp_products:
        bp_size = get_bp_size(bp, dict(zip(bp_args, bp_prod)))
        if not within_size(bp_size, size_lim):
            if bp != "bp_static": # static is always included
                continue
        command = []
        for k, v in zip(bp_args, bp_prod):
            if "method" in k or "combined" in bp:
                command.append(f"--{bp}_{k}")
            else:
                command.append(f"--bp_{k}")
            command.append(str(v))

        cmds.append(command)
        sizes.append(bp_size)

    return sizes, cmds

def gen_bp_final_params(sr: Dict) -> List[List[Tuple[str, Any]]]:
    out = {}
    for bp, entry in sr.items():
        cmds = []
        #print(f"Best configs for {bp}:")
        for size, d in entry.items():
            #print(f"Size: {size} B, Acc: {d['acc']}%")
            command = []
            for k, v in d.items():
                if 'bp' not in k: # only keys starting with bp are cli args
                    continue
                command.append(f"--{k}")
                command.append(f"{v}")
            cmds.append(command)
        out[bp] = cmds

    if "bp_static" not in out: # in case not in the input sr
        out["bp_static"] = [[]]

    return out

# needed only for guided_bp_search_for_combined()
# but needs to be at the top level for parallel processing to work
def compute_max_cols(args):
    col_pairs, df = args
    return {
        f"{col1}-{col2}": df[[col1, col2]].max(axis=1)
        for col1, col2 in col_pairs
    }

def guided_bp_search_for_combined(
    args: argparse.Namespace,
    sweep_params: Dict[str, Any],
    bp_handle: str
):
    wd = args.work_dir
    bpc = sweep_params[bp_handle]
    bp1 = bpc["predictors"][0]
    bp2 = bpc["predictors"][1]
    bp_csvs = glob.glob(os.path.join(wd, f"*{bp1}*", "branches.csv"))
    if bp1 != bp2:
        bp_csvs += glob.glob(os.path.join(wd, f"*{bp2}*", "branches.csv"))
    bp_csvs.sort()

    df_list = {} # a dict of lists
    # put bp stats for all benchmarks in separate dfs
    for csv in bp_csvs:
        sim_dir = os.path.dirname(csv).split('/')[-1]
        bench_name = sim_dir.split('_bp_')[0].replace('out_', '')
        bp_sweep_name = f"bp_{sim_dir.split('_bp_')[1]}"
        bp_id = bp_sweep_name.split('_')[1]
        og_bp_name = f'P_{bp_id}'
        dfl = pd.read_csv(csv, usecols=['PC', 'All', og_bp_name])
        dfl['bench'] = bench_name
        dfl = dfl.loc[:, ['bench', 'PC', 'All', og_bp_name]]
        dfl = dfl.rename(columns={og_bp_name : bp_sweep_name})
        if bench_name not in df_list:
            df_list[bench_name] = []
        df_list[bench_name].append(dfl)

    # first merge all dfs for different predictors for the same benchmark
    merge_columns = ['bench', 'PC', 'All']
    df_dict = {}
    for bench,bench_dfs in df_list.items():
        df_dict[bench] = pd.merge(bench_dfs[0], bench_dfs[1], on=merge_columns)
        for d in bench_dfs[2:]:
            df_dict[bench] = pd.merge(df_dict[bench], d, on=merge_columns)

    # then concatenate all benchmarks into single df
    dfc = pd.concat(df_dict.values(), ignore_index=True)

    # sort columns by prefix priority, fallback to original order
    prefix_order = [
        'bench', 'PC', 'All',
        'bp_static', 'bp_bimodal', 'bp_local',
        'bp_global', 'bp_gselect', 'bp_gshare']

    sorted_cols = sorted(
        dfc.columns,
        key=lambda col: next((i for i, p in enumerate(prefix_order)
                              if col.startswith(p)), len(prefix_order))
    )

    dfc = dfc[sorted_cols]

    # find max in parallel
    iter_cols = dfc.columns.tolist()[3:]
    all_combinations = list(combinations(iter_cols, 2))
    n_workers = args.max_workers
    chunk_size = (len(all_combinations) + n_workers - 1) // n_workers
    chunks = [all_combinations[i:i+chunk_size]
              for i in range(0, len(all_combinations), chunk_size)]

    with ProcessPoolExecutor(max_workers=n_workers) as executor:
        results = executor.map(compute_max_cols,
                               [(chunk, dfc) for chunk in chunks])

    max_cols = dict(chain.from_iterable(d.items() for d in results))
    dfc = pd.concat([dfc, pd.DataFrame(max_cols)], axis=1)

    def get_size(two_bps_str):
        try:
            bp1, bp2 = two_bps_str.split('-')
        except:
            # assume only one bp passed in, may be wrong, tbd
            bp1 = two_bps_str
            bp2 = "a_b_c"
        size = 0
        for bp in [bp1, bp2]:
            try:
                _, bp_type, bp_params = bp.split('_')
            except:
                raise ValueError(f"cant parse bp: {bp}")
            if bp_type == 'static':
                size += get_bp_size(bp_type, {})
            elif bp_type == 'bimodal':
                size += get_bp_size(bp_type, {"pc_bits": int(bp_params[0]),
                                              "cnt_bits": int(bp_params[1])})
            elif bp_type == 'local':
                size += get_bp_size(bp_type, {"pc_bits": int(bp_params[0]),
                                              "lhist_bits": int(bp_params[1]),
                                              "cnt_bits": int(bp_params[2])})
            elif bp_type == 'global':
                size += get_bp_size(bp_type, {"gr_bits": int(bp_params[0]),
                                              "cnt_bits": int(bp_params[1])})
            elif bp_type in ['gselect', 'gshare']:
                size += get_bp_size(bp_type, {"pc_bits": int(bp_params[0]),
                                              "gr_bits": int(bp_params[1]),
                                              "cnt_bits": int(bp_params[2])})
            else:
                size += 0 # placeholder, maybe error out?
        return size

    def get_sim_args(two_bps_str):
        try:
            bp1, bp2 = two_bps_str.split('-')
        except:
            # assume only one bp passed in, may be wrong, tbd
            bp1 = two_bps_str
            bp2 = " _none_ "

        out = []
        out_args = []
        for idx,bp in enumerate([bp1, bp2]):
            _, bp_type, bp_params = bp.split('_')

            out.append(bp_type)
            bp_arg = '--bp' if idx == 0 else '--bp2'
            out_args += [bp_arg, bp_type]

            if bp_type == 'static':
                out_args += [f"{bp_arg}_static_method", bp_params]
            elif bp_type == 'bimodal':
                out_args += [f"{bp_arg}_pc_bits", bp_params[0],
                             f"{bp_arg}_cnt_bits", bp_params[1]]
            elif bp_type == 'local':
                out_args += [f"{bp_arg}_pc_bits", bp_params[0],
                             f"{bp_arg}_lhist_bits", bp_params[1],
                             f"{bp_arg}_cnt_bits", bp_params[2]]
            elif bp_type == 'global':
                out_args += [f"{bp_arg}_gr_bits", bp_params[0],
                             f"{bp_arg}_cnt_bits", bp_params[1]]
            elif bp_type in ['gselect', 'gshare']:
                out_args += [f"{bp_arg}_pc_bits", bp_params[0],
                             f"{bp_arg}_gr_bits", bp_params[1],
                             f"{bp_arg}_cnt_bits", bp_params[2]]
            else:
                pass # placeholder, maybe error out?

        out.append(out_args)
        return(out)

    # per bench
    df_max_list = []
    bench_df_columns = ['bp_tag', 'acc']
    for b in dfc.bench.unique().tolist():
        dfcb = dfc.loc[dfc.bench == b]
        bench_df = pd.DataFrame(
            (dfcb.drop(columns=['All'])
            .sum(numeric_only=True)
            .sort_values(ascending=False) / dfcb.All.sum()*100
            ).round(2)
            ).reset_index()
        bench_df.columns = bench_df_columns
        bench_df['bench'] = b
        bench_df['bp_size'] = bench_df.bp_tag.apply(get_size)
        bp_sim_args = bench_df.bp_tag.apply(get_sim_args)
        bench_df[['bp_type', 'bp2_type', 'bp_sim_args']] = pd.DataFrame(
            bp_sim_args.tolist(), index=bench_df.index)
        bench_df = bench_df[~(bench_df.bp2_type == 'none')]
        if bp1 != bp2:
            # not allowed to take the same bp for both unless specified
            bench_df = bench_df[~(bench_df.bp_type == bench_df.bp2_type)]
        df_max_list.append(bench_df)

    def get_top_n(dfs, columns, top_n, sort_bench=False, size_limit=None):
        # columns = [name, value]

        # get top-N names from each df
        #top_sets = [set(df.nlargest(n, columns[1])[columns[0]]) for df in dfs]
        top_sets = [set(df[columns[0]]) for df in dfs]

        if size_limit is not None:
            dfs = [df[df['bp_size'] <= size_limit].copy() for df in dfs]

        # intersect all sets to get best_bp names
        best_bp = set.intersection(*top_sets)

        # filter each df to only include those best_bp names
        filtered_all = [df[df[columns[0]].isin(best_bp)].copy() for df in dfs]

        # concatenate and group to compute average value
        all_common = pd.concat(filtered_all, ignore_index=True)
        avg_df = (
            all_common.groupby(columns[0], as_index=False)
            .agg(
                avg_acc=(columns[1], 'mean'),
                bp_size=('bp_size', 'first'),
                bp_type=('bp_type', 'first'),
                bp2_type=('bp2_type', 'first'),
                bp_sim_args=('bp_sim_args', 'first')
            )
            .sort_values('avg_acc', ascending=False)
        )

        top_bps = set(avg_df.head(top_n)[columns[0]])
        filtered = [df[df[columns[0]].isin(top_bps)].copy()
                    for df in filtered_all]
        avg_df = avg_df.head(top_n).reset_index(drop=True)

        if sort_bench:
            name_order = avg_df[columns[0]].tolist()
            filtered = [df.set_index(columns[0])
                        .loc[name_order]
                        .reset_index() for df in filtered]

        return filtered, avg_df

    TOP_N_PREDICTORS = bpc['use_max_top_n']
    best_bp_list = []
    sizes = sweep_params['common_settings']['size_bins']
    for sz in sizes:
        filtered, best_bp = get_top_n(
            df_max_list, bench_df_columns, TOP_N_PREDICTORS, True, sz
        )
        best_bp = best_bp.copy()
        best_bp['avg_acc'] = best_bp['avg_acc'].round(2)

        for df in filtered:
            bench_name = df['bench'].iloc[0]
            col_name = f'acc_{bench_name}'
            tmp = df[['bp_tag', 'acc']].rename(columns={'acc': col_name})
            best_bp = pd.merge(best_bp, tmp, on='bp_tag', how='left')

        best_bp_list.append(best_bp)

    # combine all sizes and drop duplicate bp_tags, if any
    best_bp_combined = pd.concat(best_bp_list, ignore_index=True)
    best_bp_combined = best_bp_combined.drop_duplicates(subset='bp_tag',
                                                        keep='first')

    bp12_p_all = [
        [size, args]
        for size, args
        in zip(best_bp_combined['bp_size'], best_bp_combined['bp_sim_args'])]

    return bp12_p_all

def run_bp_sweep(
    args: argparse.Namespace,
    workloads: List[str],
    sweep_params: Dict[str, Any],
    best_params: List[List[Tuple[str, Any]]] = None) \
    -> None:

    running_best = (best_params != None)
    multi_wl = len(workloads) > 1
    running_single_best = running_best and not multi_wl
    sweep_name, sweep_log = gen_sweep_log_name(
        args.sweep, args.work_dir, workloads, not running_best)
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
    save_bpp_for_combined_e = {}
    for bp_handle in sweep_params.keys():
        if args.load_stats:
            break # skip the sweep if loading previous stats
        if bp_handle == "common_settings":
            continue

        best = {}
        bp = bp_handle.split("-")[0]
        bp_act =  ["--bp", bp.replace("bp_", "", 1)]

        if running_best:
            bp_params = best_params[bp_handle]
            if bp == "bp_combined":
                bp_act = [""]# sim auto sets act=comb when second bp is provided

        # sweep runs
        elif bp == "bp_combined":
            bpc = sweep_params[bp_handle]
            bpc_sizes, bpc_params = gen_bp_sweep_params(bp, bpc, SIZE_LIM)
            bp1 = bpc["predictors"][0]
            bp2 = bpc["predictors"][1]
            bp_act = [""] # sim auto sets act=comb when second bp is provided

            if "use_max_top_n" in bpc: # guided approach with theoretical max
                bp12_p_all = guided_bp_search_for_combined(
                    args, sweep_params, bp_handle)

                bp_params = [
                    bp12_p + bpc_p
                    for (bp12_s, bp12_p), (bpc_s, bpc_p)
                    in product(bp12_p_all, zip(bpc_sizes, bpc_params))
                    if within_size(bp12_s + bpc_s, SIZE_LIM)
                ]

            else: # approximation for best, or brute force
                if bpc["exhaustive"][0]:
                    bp1_p_all = save_bpp_for_combined_e[bp1]
                else:
                    bp1_params = save_bpp_for_combined[bp1]
                    # split into two lists: keys(sizes) and values(params)
                    bp1_p_all = [[k, v] for k, v in bp1_params.items()]

                if bpc["exhaustive"][1]:
                    bp2_p_all = save_bpp_for_combined_e[bp2]
                else:
                    bp2_params = save_bpp_for_combined[bp2]
                    bp2_p_all = [[k, v] for k, v in bp2_params.items()]

                bp2_p_all = [
                    [size, [s.replace("--bp_", "--bp2_") for s in alst]]
                    for size, alst in bp2_p_all]

                bp_params_list = [
                    bp1_p + bp2_p + bpc_p
                    for (bp1_s, bp1_p), (bp2_s, bp2_p), (bpc_s, bpc_p)
                    in product(bp1_p_all, bp2_p_all, zip(bpc_sizes, bpc_params))
                    if within_size(bp1_s + bp2_s + bpc_s, SIZE_LIM)
                ]

                bp1_arg = [f"--bp", bp1.replace("bp_", "", 1)]
                bp2_arg = [f"--bp2", bp2.replace("bp_", "", 1)]
                for m in bp_params_list:
                    # add the combined predictor args into each command
                    m.extend(bp1_arg)
                    m.extend(bp2_arg)
                bp_params = bp_params_list

        else:
            save_bpp_for_combined[bp_handle] = {}
            save_bpp_for_combined_e[bp_handle] = []
            _, bp_params = gen_bp_sweep_params(
                bp, sweep_params[bp_handle], SIZE_LIM)

        idx_c = 1
        PROG_BAT = 4 # progress batches
        if args.track and not running_single_best:
            tt = track_time()
            print(INDENT,
                  f"BP: {bp_handle}, configs: {len(bp_params)}, progress: ",
                  end="")
            idx_c = (len(bp_params) // PROG_BAT) + 1

        with ProcessPoolExecutor(max_workers=args.max_workers) as executor:
            futures = {
                executor.submit(
                    run_workloads,
                    workload_params(
                        workloads=workloads,
                        sweep=bpk,
                        args=bpp + bp_act,
                        msg=f"==> SWEEP: {bp_handle} {bpp} <==",
                        save_sim=args.save_sim,
                        ignore_thr=(bp == "bp_static" or running_best),
                        tag=f"{bp_handle}_" + "".join(bpp[1::2]), # every even index is a number
                        ret_list=bpp
                    )
                ): bpp for bpp in bp_params
            }

            # get results as they come in
            cnt = 0
            for future in as_completed(futures):
                bp_size, acc, bpp, msg = future.result()

                cnt += 1
                if args.track and cnt % idx_c == 0 and not running_single_best:
                    print(f"{cnt//idx_c}/{PROG_BAT}", end=", ", flush=True)
                with open(sweep_log, "a") as f:
                    f.write(msg)

                if bp != "bp_combined" and not running_best:
                    save_bpp_for_combined_e[bp_handle].append([bp_size, bpp])
                if acc == None:
                    continue

                d = {}
                for i in range(0,len(bpp),2):
                    d[bpp[i].replace("--", "", 1)] = bpp[i+1]
                if (bp_size not in best or acc > best[bp_size]["acc"]):
                    best[bp_size] = {}
                    for k, v in d.items():
                        best[bp_size][k] = v
                    best[bp_size]["acc"] = acc
                    if bp != "bp_combined" and not running_best:
                        save_bpp_for_combined[bp_handle][bp_size] = bpp

        if args.track and not running_single_best:
            tt_now, tt_taken = tt()
            print(f"{PROG_BAT}/{PROG_BAT}. Done at {tt_now}. Taken: {tt_taken}")

        # sort the best dict by accuracy
        sr_best[bp_handle] = dict(
            sorted(best.items(), key=lambda x: x[1]["acc"], reverse=True))
        # take only if above the threshold for accuracy
        sr_best_thr = dict(
            filter(lambda x: x[1]["acc"] >= args.bp_top_thr,
                   sr_best[bp_handle].items()))
        # get the top N configs
        sr_best_num = dict(
            list(sr_best[bp_handle].items())[:args.bp_top_num])
        # merge the two dicts
        sr_best[bp_handle] = {**sr_best_thr, **sr_best_num}
        # sort again in ascending order (more intuitive)
        sr_best[bp_handle] = dict(
            sorted(sr_best[bp_handle].items(), key=lambda x: x[1]["acc"]))

        # sort the best dict by size
        #sr_best[bp_handle] = dict(
        #    sorted(sr_best[bp_handle].items(), key=lambda x: x[0]))

        # for each bin, find the best config for accuracy
        prev_bin = -1 # 0 valid size for static bp
        best_binned = {}
        for bin in bins:
            for bp_size, d in best.items():
                if prev_bin < bp_size <= bin:
                    if ((bin not in best_binned or
                        d["acc"] > best_binned[bin]["acc"])):
                        best_binned[bin] = d.copy() # so size is not added to og
                        best_binned[bin]["size"] = bp_size
            prev_bin = bin

        sr_bin[bp_handle] = best_binned
        # TODO: save to disk as each bp/cache is finished

    # sweep results
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
    ncols = 1 if running_best else 2
    fig, axs = create_plot(
        bpk.capitalize(), sweep_name, ncols, title_2=args.chart_title)
    for sr,ax in zip([sr_bin, sr_best], axs):
        title_add = ""
        for bp_h,entry in sr.items():
            if len(entry.keys()) == 0: # below thr limit for all available sizes
                continue
            if bp_h not in sweep_params:
                # e.g. input config changed between search and plot runs
                continue

            fmt = lambda x: x.replace("bp_", "", 1)
            mk = MK[0]
            label = fmt(bp_h)
            clr = BP_COLORS.get(label, 'k')
            if "bp_combined" in bp_h:
                bp1 = fmt(sweep_params[bp_h]["predictors"][0])
                bp2 = fmt(sweep_params[bp_h]["predictors"][1])
                label = f"{label}\n{bp1} & {bp2}"
                mk = MK[1]
                clr = BP_COLORS.get(bp2, 'k') # bp2 assumed as differentiator

            if "static" in bp_h:
                fk = list(entry.keys())[0] # first key
                method = entry[fk]["bp_static_method"]
                ax.axhline(y=entry[fk]["acc"], color="r", linestyle="--",
                           label=f"{label} {method} ({entry[fk]['acc']:.0f}%)")
                continue

            accs = [entry[s]["acc"] for s in entry.keys()]
            if sr == sr_bin: # bins are keys, sizes stored as entry
                sizes = [entry[s]["size"] for s in entry.keys()]
                title_add = "Best per bin size"
                ax.plot(sizes, accs, label=label, color=clr,
                        marker=mk[0], markersize=mk[1], lw=LW)
            else: # sizes are keys
                sizes = entry.keys()
                title_add = f"Best {args.bp_top_num} " \
                            f"and all above {args.bp_top_thr}%"
                ax.scatter(sizes, accs, label=label, color=clr,
                           marker=mk[0], s=mk[2])

        ax.set_title(title_add)

    for a in axs:
        a.set_xlim(SIZE_LIM)
        a.set_xlabel("Size (B)")
        a.set_xticks(bins)
        a.set_xticklabels(a.get_xticks(), rotation=45)
        a.set_ylabel("Accuracy [%]")
        a.legend(loc="upper left")
        a.grid(True)
        a.margins(x=0.02)
        ymin = a.get_ylim()[0] * 0.99
        if args.plot_acc_thr:
            ymin = max(args.plot_acc_thr, ymin)
        a.set_ylim(ymin, 100.1)

    if args.save_png:
        fig.savefig(sweep_log.replace(".log", ".png"))

    return sr_bin

MAX_WORKERS = int(os.cpu_count())
def parse_args() -> argparse.Namespace:
    SWEEP_CHOICES = ["icache", "dcache", "bpred"]
    parser = argparse.ArgumentParser(description="Sweep through specified hardware models for a given workload")
    parser.add_argument("-s", "--sweep", required=True, choices=SWEEP_CHOICES, help="Select the hardware model to sweep")
    parser.add_argument("-p", "--params", required=True, help="Path to the hardware model sweep params file")
    parser.add_argument("--save_sim", action="store_true", help="Save simulation stdout in a log file")
    parser.add_argument("--save_stats", action="store_true", help="Save combined simulation stats as json")
    parser.add_argument("--load_stats", action="store_true", default=None, help="Load the previously saved stats from a json file instead of running the sweep. Takes priority over --save_stats")
    parser.add_argument("--work_dir", default=os.getcwd(), help="Path to run the sweep in and store results. Either absolute, or relative to this script")
    parser.add_argument("--track", action="store_true", help="Print the sweep progress")
    parser.add_argument("--silent", action="store_true", help="Don't display chart(s) in pop-up window")
    parser.add_argument("--chart_title", type=str, default="", help="Adds this string to each chart's title")
    parser.add_argument("--save_png", action="store_true", help="Save chart(s) as PNG")
    parser.add_argument("--plot_hr_thr", type=int, default=None, help="Set the lower limit for the plot y-axis for cache hit rate")
    parser.add_argument("--plot_ct_thr", type=int, default=None, help="Set the upper limit for the plot y-axis for cache traffic")
    parser.add_argument("--plot_no_ct_thr", action="store_true", help="Don't the upper limit for the plot y-axis for cache traffic")
    parser.add_argument("--plot_acc_thr", type=int, default=None, help="Set the lower limit for the plot y-axis for branch predictor accuracy")
    parser.add_argument("--bp_top_num", type=int, default=16, help="Number of top branch predictor configs to always include based only on accuracy")
    parser.add_argument("--bp_top_thr", type=int, default=80, help="Accuracy threshold for the best branch predictor configs to always include")
    parser.add_argument("--add_all_workloads", action="store_true", default=False, help="Run with all workloads from the config (i.e. including 'skip_search: true' ones) after the main sweep finished. Creates a single chart as average values across all workloads. Used to validate HW model across all benchmarks")
    parser.add_argument("--add_per_workload", action="store_true", default=False, help="Run with all workloads from the config (i.e. including 'skip_search: true' ones) after the main sweep finished. Creates one chart per workload. Used to validate HW model across all benchmarks")
    parser.add_argument("--max_workers", type=int, default=MAX_WORKERS, help="Maximum number of workers")

    return parser.parse_args()

def run_main(args: argparse.Namespace) -> None:
    if not os.path.exists(args.params):
        raise FileNotFoundError(f"Params file not found at: {args.params}")

    if not os.path.exists(args.work_dir):
        os.makedirs(args.work_dir)

    with open(args.params, "r") as flavor:
        sweep_dict = json.load(flavor)

    workloads_all = []
    workloads_sweep = []
    workloads_dict = sweep_dict["workloads"]
    for wl,flavors in workloads_dict.items():
        for flavor in flavors[0]:
            elf = os.path.join(APPS_DIR, wl, f"{flavor}.elf")
            if not os.path.exists(elf) and not args.load_stats:
                raise FileNotFoundError(f"elf not found at: {elf}")
            else:
                for_sweep = not flavors[3]['skip_search']
                wl_args = flavors[2]["sim_args"].split(" ") # args as list
                e = {"app": elf, "thr": flavors[1], "args": wl_args}
                workloads_all.append(e)
                if for_sweep:
                    workloads_sweep.append(e)

    if len(workloads_sweep) == 0:
        raise FileNotFoundError("No workloads found for sweep")

    single_wl_sweep = len(workloads_sweep) == 1
    if single_wl_sweep:
        print(f"Sweep: {args.sweep} for {workloads_sweep[0]['app']}")
    else:
        print(f"Sweep: {args.sweep} with {len(workloads_sweep)} workload(s)")

    sweep_params = sweep_dict["hw_params"][args.sweep]
    if "cache" in args.sweep:
        tt = track_time()
        sr_best = run_cache_sweep(args, workloads_sweep, sweep_params)
        sweep_wrapper_end(tt, "\n")
        if single_wl_sweep:
            return

        # FIXME: redundant to run 'all workloads' and 'per workload'
        # as these are the same results, just plotted differently
        # ideally, save from 'all' run, and plot for 'per workload'
        params = gen_cache_final_params(args.sweep, sr_best)
        if args.add_all_workloads and workloads_all != workloads_sweep:
            sweep_best_wrapper_start(args.sweep, "all workloads", "\n")
            run_cache_sweep(args, workloads_all, sweep_params, params)
            sweep_wrapper_end(tt)

        if args.add_per_workload:
            for wl in workloads_all:
                sweep_best_wrapper_start(args.sweep, get_test_name(wl['app']))
                run_cache_sweep(args, [wl], sweep_params, params)
                sweep_wrapper_end(tt)

        return # only one sweep type at a time

    if "bp" in args.sweep:
        tt = track_time()
        sr_bin = run_bp_sweep(args, workloads_sweep, sweep_params)
        sweep_wrapper_end(tt, "\n")
        if single_wl_sweep:
            return

        # FIXME: same as above
        params = gen_bp_final_params(sr_bin)
        if args.add_all_workloads and workloads_all != workloads_sweep:
            sweep_best_wrapper_start(args.sweep, "all workloads", "\n")
            run_bp_sweep(args, workloads_all, sweep_params, params)
            sweep_wrapper_end(tt)

        if args.add_per_workload:
            for wl in workloads_all:
                sweep_best_wrapper_start(args.sweep, get_test_name(wl['app']))
                run_bp_sweep(args, [wl], sweep_params, params)
                sweep_wrapper_end(tt)

        return # only one sweep type at a time

    raise ValueError("Unknown sweep type and impossible to reach")

def sweep_best_wrapper_start(sweep: str, workload: str, end: str = ''):
    print(f"Sweeping best '{sweep}' configs for '{workload}'. ", end=end)

def sweep_wrapper_end(tt: track_time, append_str: str = ""):
    tt_now, tt_taken = tt()
    print(f"Done at {tt_now}. Taken: {tt_taken}{append_str}")

if __name__ == "__main__":
    if not os.path.exists(SIM):
        raise FileNotFoundError(f"Simulator not found at: {SIM}")
    tt = track_time()
    args = parse_args()
    if not args.load_stats:
        args.max_workers = min(MAX_WORKERS, args.max_workers)
        #print(f"Running sweep with {args.max_workers} workers"
        #      f"(max: {MAX_WORKERS})")
    run_main(args)
    print("\nAll sweeps finished. ", end="")
    sweep_wrapper_end(tt, "\n")

    if not args.silent:
        plt.show(block=False)
        if not is_notebook() and not is_headless():
            # for notebook session, no need to wait
            # headless doesn't display charts anyway
            input("Press Enter to close all plots...")
    plt.close("all")
