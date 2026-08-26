#!/usr/bin/env python3

import argparse
import glob
import json
import os
import subprocess
import sys

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import yaml
from matplotlib.ticker import MultipleLocator
from tda import BAR_COLOR_MAP, COLOR_MAP, PLOTLY_COLORS
from utils import (FMT_AXIS, INDENT, get_reporoot, get_test_title,
                   print_file_saved, smarter_eng_formatter)

REPO_ROOT = os.environ.get("REPO_ROOT") or get_reporoot()
os.environ["REPO_ROOT"] = REPO_ROOT # so '${REPO_ROOT}' in a yaml expands too
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
TDA_PY = os.path.join(SCRIPT_DIR, "tda.py")
TDA_ARGS = [
    "--get_stats", "--silent", "--save_csv", "--save_log", "--save_hw_stats"
]
PROF_GLOB = "inst_profile*.json" # '_cosim' suffixed on the RTL side
ELF_TEXT_KEY = "_elf_text_size" # '.text' bytes, as GNU 'size -G' reports them

FMT = smarter_eng_formatter(places=1)

IPC_COLOR = "#2b2b2b" # very dark gray
TEXT_SIZE_COLOR = PLOTLY_COLORS[2]
SEP_COLOR = "#bbbbbb" # light gray
GRID_COLOR = "#dddddd" # very light gray

FIG_H = 4.5
BAR_W = 0.65
GROUP_LABEL_Y = -0.28 # room for rotated tick labels
LEGEND_W = 1.7 # room added on the right
LEGEND_PAD = .55 # gap to the axes, past the IPC label on the right y-axis

# scaling of the stacked values
SC_CYCLES = "cycles" # absolute
SC_TOTAL = "total" # fraction of that run's cycles
SC_GROUP_BEST = "group_best" # ratio to the fastest run in the same group

# y axis limits
YL_UNIT = "unit" # fixed 0..1
YL_NICE = "nice" # 0..next step up from the largest bar
YL_AUTO = "auto" # autoscaled
YL_ENG = "eng" # autoscaled, eng formatted

# labels on top of the bars
TL_REL = "rel" # bar value, relative
TL_ENG = "eng" # bar value, eng formatted

# (key, csv column, legend label, color); listed bottom-to-top in the stack
TOP_CATS = [
    ("retiring", "retiring", "Retiring", COLOR_MAP["retiring"]),
    ("lost", "lost", "Lost", COLOR_MAP["lost"]),
    ("frontend", "frontend", "Frontend Bound", COLOR_MAP["frontend"]),
    ("backend", "backend", "Backend Bound", COLOR_MAP["backend"]),
]
BE_CATS = [
    ("be_core", "core", "Core Bound", BAR_COLOR_MAP["stall_*"]),
    ("be_dcache", "dcache", "Dcache Bound", BAR_COLOR_MAP["l1d_*"]),
]
FE_CATS = [
    ("fe_core", "core", "Core Bound", BAR_COLOR_MAP["stall_*"]),
    ("fe_icache", "icache", "Icache Bound", BAR_COLOR_MAP["l1i_*"]),
]
# no TDA breakdown here for plain cycles plot
CYC_CATS = [
    ("total_cycles", "cycles", "Cycles", BAR_COLOR_MAP["cycles"]),
]
# drawn as a second series, beside the cycles bar
TEXT_CATS = [
    ("text_size", "text_size", "Text size", TEXT_SIZE_COLOR),
]

PLOTS = [
    dict(name="top_level", cats=TOP_CATS, title="Top Level",
         ylabel="cycles [%]", scale=SC_TOTAL, ylim=YL_UNIT, ipc=True),
    dict(name="backend_level", cats=BE_CATS, title="Backend Level",
         ylabel="cycles [%]", scale=SC_TOTAL, ylim=YL_NICE),
    dict(name="frontend_level", cats=FE_CATS, title="Frontend Level",
         ylabel="cycles [%]", scale=SC_TOTAL, ylim=YL_NICE),
    dict(name="cycles_size", cats=CYC_CATS, cats2=TEXT_CATS,
         title="Cycles & Text size",
         ylabel="cycles & text size (group relative)", scale=SC_GROUP_BEST,
         ylim=YL_AUTO, totals=TL_REL),
    dict(name="cycles_abs", cats=CYC_CATS, title="Cycles (absolute)",
         ylabel="cycles", scale=SC_CYCLES, ylim=YL_ENG, totals=TL_ENG,
         opt=True),
]

def load_entries(yaml_path: str) -> list[tuple[str, str]]:
    """parse the input yaml into an ordered list of (tag, hw_stats path)"""
    with open(yaml_path, 'r') as f:
        cfg = yaml.safe_load(f) or {}

    raw = cfg.get("hw_stats")
    if not raw:
        raise ValueError(f"No 'hw_stats' entries found in '{yaml_path}'")

    entries = []
    for tag, path in raw:
        # resolve relative paths against the repo root, keep absolute as-is
        path = os.path.expanduser(os.path.expandvars(str(path)))
        if not os.path.isabs(path):
            path = os.path.join(REPO_ROOT, path)
        entries.append((str(tag), path))

    missing = [p for _, p in entries if not os.path.exists(p)]
    if missing:
        raise FileNotFoundError(
            f"'hw_stats' file(s) from '{yaml_path}' not found:\n" +
            "\n".join(f"{INDENT}{p}" for p in missing))

    return entries

def get_text_sizes(entries: list[tuple[str, str]]) -> list[int]:
    """'.text' bytes per run, captured into the inst profile at run time"""
    sizes, missing = [], []
    for _, path in entries:
        prof = glob.glob(os.path.join(os.path.dirname(path), PROF_GLOB))
        size = None
        if prof:
            with open(prof[0], 'r') as f:
                size = json.load(f).get(ELF_TEXT_KEY)
        if size is None:
            missing.append(path)
        sizes.append(size)

    if missing:
        raise ValueError(
            f"No '{ELF_TEXT_KEY}' in the '{PROF_GLOB}' next to:\n" +
            "\n".join(f"{INDENT}{p}" for p in missing))

    return sizes

def get_workload(path: str) -> str:
    """workload name from the dir holding the stats json, uniform for cosim
    ('<workload>_out_cosim') and perf est ('<workload>_out') outputs"""
    OUT_SUFFIXES = ("_cosim", "_out")
    name = os.path.basename(os.path.dirname(path))
    for suffix in OUT_SUFFIXES: # order matters, '_cosim' is the outer one
        name = name.removesuffix(suffix)
    return name

def run_tda(path: str) -> pd.DataFrame:
    """run tda.py on a single hw_stats.json and read back the TDA csv"""
    cmd = [sys.executable, TDA_PY, path] + TDA_ARGS
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(res.stdout, file=sys.stderr)
        print(res.stderr, file=sys.stderr)
        raise RuntimeError(f"'{' '.join(cmd)}' failed with {res.returncode}")

    # same naming as tda.py uses when saving
    title_path = get_test_title(path).replace(' ', '_')
    csv_path = os.path.join(os.path.dirname(path), f"{title_path}_tda.csv")
    if not os.path.exists(csv_path):
        raise FileNotFoundError(f"tda.py did not produce '{csv_path}'")

    return pd.read_csv(csv_path)

def summarize(df_tda: pd.DataFrame) -> dict:
    """collapse one run's TDA table into the fields the comparison plots need"""

    def s(l1: str, l2: str = None) -> int:
        m = df_tda["L1"] == l1 # all rows where l1 matches
        if l2 is not None:
            m &= df_tda["L2"] == l2 # ditto for l2, if provided
        return int(df_tda.loc[m, "cycles"].sum())

    total = int(df_tda["cycles"].sum())
    ret = s("retiring")

    return {
        "retiring": ret,
        "lost": s("lost"),
        "frontend": s("frontend"),
        "backend": s("backend"),
        "be_core": s("backend", "core"),
        "be_dcache": s("backend", "dcache"),
        "fe_core": s("frontend", "core"),
        "fe_icache": s("frontend", "icache"),
        "total_cycles": total,
        "ipc": ret / total if total else 0,
    }

def contiguous_groups(names: list[str]) -> list[tuple[str, int, int]]:
    """runs of the same workload, as (name, first index, last index)"""
    groups = []
    for i, name in enumerate(names):
        if groups and groups[-1][0] == name:
            groups[-1][2] = i
        else:
            groups.append([name, i, i])
    return [tuple(g) for g in groups]

def get_divisor(df: pd.DataFrame, scale: str, tops: np.ndarray) -> np.ndarray:
    """per-entry divisor for the stacked values, 'tops' are the stack heights"""
    if scale == SC_CYCLES:
        return np.ones(len(df))
    if scale == SC_TOTAL:
        return df["total_cycles"].to_numpy(dtype=float)

    # SC_GROUP_BEST: the smallest stack in the group,
    # i.e. referenced to the group's best, whichever one it is
    div = np.ones(len(df)) # just allocation
    for _, start, end in contiguous_groups(list(df["workload"])):
        div[start:end + 1] = tops[start:end + 1].min()
    return div

def stacked_values(df: pd.DataFrame, cats: list, scale: str) -> np.ndarray:
    """stacked values as (n_categories, n_entries), already scaled"""
    vals = np.array([df[key].to_numpy(dtype=float) for key, *_ in cats])
    div = get_divisor(df, scale, vals.sum(axis=0))
    return np.divide(vals, div, out=np.zeros_like(vals), where=div > 0)

def get_series(spec: dict) -> list[list]:
    """bar series of the plot, a second one is drawn beside the first"""
    return [spec["cats"]] + ([spec["cats2"]] if spec.get("cats2") else [])

def nice_top(vmax: float) -> float:
    """round up to the next 0.1 step,
    or a finer decade when the bars are small"""
    # safeguard against tiny data
    if vmax <= 0:
        return 1.0
    step = 0.1
    while vmax < step and step > 1e-6:
        step /= 10
    # common path - round up and clamp to 1
    return min(1.0, np.ceil(vmax / step) * step)

def label_workloads(ax, workloads: list[str]):
    """tick labels are tags only, workloads get a label under the axis"""
    for name, start, end in contiguous_groups(workloads):
        ax.annotate(
            name,
            xy=((start + end) / 2, GROUP_LABEL_Y),
            xycoords=("data", "axes fraction"),
            ha="center", va="top",
            fontsize=9, annotation_clip=False
        )
        if end + 1 < len(workloads): # separator to the next group
            ax.axvline(end + 0.5, color=SEP_COLOR, lw=0.8, zorder=0)

def plot_stacked(df: pd.DataFrame, spec: dict, name: str, legend_right=False):
    series, n = get_series(spec), len(df)
    x = np.arange(n)
    FIG_W = (0.3 * n + 2.5) if legend_right else max(6.0, 0.5 * n + 2.5)
    fig_w = FIG_W + LEGEND_W if legend_right else FIG_W
    fig, ax = plt.subplots(figsize=(fig_w, FIG_H))

    bar_w = BAR_W / len(series) + (.1 if len(series) > 1 else 0)
    tops = [] # (x offset, stack height) per series
    for i, cats in enumerate(series):
        off = (i - (len(series) - 1) / 2) * bar_w
        vals = stacked_values(df, cats, spec["scale"])
        bottom = np.zeros(n)
        for (_, _, label, color), val in zip(cats, vals):
            ax.bar(
                x + off, val, bar_w, bottom=bottom, label=label, color=color,
                edgecolor="white", linewidth=0.4
            )
            bottom += val
        tops.append((off, bottom))

    vmax = max(b.max() for _, b in tops)

    ax.set_xticks(x)
    ax.set_xticklabels(df["tag"], rotation=45, ha="right")
    ax.set_xlim(-0.5, n - 0.5)
    ax.set_ylabel(spec["ylabel"])
    ax.set_axisbelow(True)
    ax.yaxis.grid(True, color=GRID_COLOR)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)

    Y_EXTRA = .05
    if spec["ylim"] == YL_UNIT:
        ax.set_ylim(0, 1+Y_EXTRA)
        ax.yaxis.set_major_locator(MultipleLocator(0.1))
    elif spec["ylim"] == YL_NICE:
        ax.set_ylim(0, nice_top(vmax))
    else: # YL_AUTO and YL_ENG, headroom for the labels on top of the bars
        ax.set_ylim(0, vmax * (1+Y_EXTRA*2) if vmax > 0 else 1)
        if spec["ylim"] == YL_ENG:
            ax.yaxis.set_major_formatter(FMT_AXIS)

    if spec.get("totals"): # stack height, bars themselves are relative
        rel = spec["totals"] == TL_REL
        for off, bottom in tops:
            for xi, y in zip(x + off, bottom):
                #txt = f"{y * 100:.0f}%" if rel else FMT(y)
                txt = ("1" if y==1 else f"{y:.2f}") if rel else FMT(y)
                ax.text(xi, y, txt, ha="center", va="bottom", fontsize=8)

    label_workloads(ax, list(df["workload"]))

    handles, labels = ax.get_legend_handles_labels()
    if spec.get("ipc"):
        ax2 = ax.twinx()
        ax2.plot(
            x, df["ipc"], color=IPC_COLOR, ls=":", marker="^", ms=5, lw=1.2,
            label="IPC"
        )
        ax2.set_ylim(0, 1+Y_EXTRA)
        ax2.set_ylabel("IPC")
        ax2.spines["top"].set_visible(False)
        h2, l2 = ax2.get_legend_handles_labels()
        handles, labels = handles + h2, labels + l2

    fig.suptitle(f"{name}: {spec['title']}", y=0.98)
    if legend_right: # one item wide, beside the axes, top aligned on all plots
        top = 0.92
        right = FIG_W * 0.9 / fig_w # same axes box as the default layout
        fig.legend(
            handles, labels, ncol=1, loc="upper left", frameon=False,
            bbox_to_anchor=((FIG_W * 0.9 + LEGEND_PAD) / fig_w, top), fontsize=9
        )
        fig.subplots_adjust(top=top, bottom=0.27, right=right)
    else: # centered
        fig.legend(
            handles, labels, ncol=len(labels), loc="upper center",
            bbox_to_anchor=(0.5, 0.93), frameon=False, fontsize=9
        )
        fig.subplots_adjust(top=0.82, bottom=0.27)

    return fig

def build_csv(df: pd.DataFrame, spec: dict) -> pd.DataFrame:
    out = df[["workload", "tag", "label"]].copy()
    for cats in get_series(spec):
        vals = stacked_values(df, cats, spec["scale"])
        for (_, col, _, _), val in zip(cats, vals):
            out[col] = val.astype(np.int64) if spec["scale"] == SC_CYCLES \
                else np.round(val, 6)
    if spec["scale"] != SC_CYCLES: # otherwise it's already the plotted value
        out["total_cycles"] = df["total_cycles"].to_numpy()
    if spec.get("cats2"): # absolutes behind the ratios
        out["total_text_size"] = df["text_size"].to_numpy()
    if spec.get("ipc"):
        out["ipc"] = np.round(df["ipc"].to_numpy(), 3)
    return out

def main(args: argparse.Namespace):
    if not os.path.exists(args.yaml):
        raise FileNotFoundError(f"File '{args.yaml}' not found")

    if args.silent and not (args.save_png or args.save_svg or args.save_csv):
        raise RuntimeError("--silent without any --save_* flag, nothing to do")

    entries = load_entries(args.yaml)
    text_sizes = get_text_sizes(entries) # run first, fails if not found

    rows = []
    for i, ((tag, path), text_size) in enumerate(zip(entries, text_sizes), 1):
        workload = get_workload(path)
        label = f"{workload}_{tag}"
        print(f"[{i}/{len(entries)}] {label}")
        row = {"label": label, "workload": workload, "tag": tag,
               "text_size": text_size}
        row.update(summarize(run_tda(path)))
        rows.append(row)

    df = pd.DataFrame(rows)

    name = os.path.splitext(os.path.basename(args.yaml))[0]
    out_dir = os.path.join(os.path.dirname(os.path.abspath(args.yaml)), name)
    os.makedirs(out_dir, exist_ok=True)

    for spec in PLOTS:
        if spec.get("opt") and not args.cycles_abs:
            continue

        fig = plot_stacked(df, spec, name, args.legend_right)

        if args.save_csv:
            csv_path = os.path.join(out_dir, f"{spec['name']}.csv")
            build_csv(df, spec).to_csv(csv_path, index=False)
            print_file_saved("CSV", csv_path)

        if args.save_png:
            png_path = os.path.join(out_dir, f"{spec['name']}.png")
            fig.savefig(png_path, dpi=150)
            print_file_saved("PNG", png_path)

        if args.save_svg:
            svg_path = os.path.join(out_dir, f"{spec['name']}.svg")
            fig.savefig(svg_path)
            print_file_saved("SVG", svg_path)

    if not args.silent:
        plt.show()
    else:
        plt.close("all")

# yaml input example
# hw_stats:
# - [scalar, "workdir/conv1d_scalar/ukr_conv1d_int16_large_out/hw_stats_perf_est.json"]
# - [load_opt, "workdir/conv1d_load_opt/ukr_conv1d_int16_large_out/hw_stats_perf_est.json"]
# - [simd, "workdir/conv1d_simd/ukr_conv1d_int16_large_out/hw_stats_perf_est.json"]
# ...

def parse_args():
    parser = argparse.ArgumentParser(description="Compare TDA across runs")
    parser.add_argument("yaml", help="Path to the yaml listing [tag, 'hw_stats.json'] pairs to compare")
    parser.add_argument('-s', '--silent', action='store_true', help="Don't display plots")
    parser.add_argument('--cycles_abs', action='store_true', help="Also plot cycles as absolute counts")
    parser.add_argument('--legend_right', '--lr', action='store_true', help="Move the legend off the plot, to the right, one item wide")
    parser.add_argument('--save_png', action='store_true', help="Save plots as PNG")
    parser.add_argument('--save_svg', action='store_true', help="Save plots as SVG")
    parser.add_argument('--save_csv', action='store_true', help="Save plot data as CSV")
    return parser.parse_args()

if __name__ == "__main__":
    main(parse_args())
