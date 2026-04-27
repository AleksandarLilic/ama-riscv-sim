#!/usr/bin/env python3

import argparse
import json
import os

import pandas as pd
import plotly.express as px
from matplotlib.ticker import EngFormatter
from utils import INDENT, get_test_title

PLOTLY_COLORS = px.colors.qualitative.Plotly

COLOR_MAP = {
    "retiring": PLOTLY_COLORS[0], # #636efa (Blue)
    "backend": PLOTLY_COLORS[2], # #00CC96 (Green)
    "frontend": PLOTLY_COLORS[1], # #EF553B (Red)
    "lost": PLOTLY_COLORS[4], # #FFA15A (Orange/Yellow)
    "(?)": "rgba(0,0,0,0)" # fallback for undefined categories
}

CLASS_ORDER = [
    "cycles", "ret", "stall", "lost",
    "ret_*", "stall_*", "bad_spec", "l1i_*", "l1d_*", "bp_*",
]
DROP_KEYS = {"cpi", "ipc"}
EXACT_CLASS = {
    "cycles": "cycles", "ret": "ret", "stalls": "stall", "lost": "lost"}
PREFIX_CLASS = [
    ("ret_", "ret_*"), ("stall_", "stall_*"), ("bad_spec", "bad_spec"),
    ("bp_", "bp_*"), ("l1i_", "l1i_*"), ("l1d_", "l1d_*")
]
BAR_COLOR_MAP = {cls: PLOTLY_COLORS[i] for i, cls in enumerate(CLASS_ORDER)}

FIG_WIDTH = 800
FIG_HEIGHT = 550

def classify_and_sort_counters(core: dict) -> list[tuple[str, int, str]]:
    """Classify core counters into groups and sort by class order then name."""
    class_rank = {c: i for i, c in enumerate(CLASS_ORDER)}
    entries = []
    for key, val in core.items():
        # drop irrelevant counters
        if key in DROP_KEYS:
            continue

        # classify by exact match?
        cnt_class = EXACT_CLASS.get(key)

        # if not, classify by prefix?
        if cnt_class is None:
            for prefix, prefix_class in PREFIX_CLASS:
                if key.startswith(prefix):
                    cnt_class = prefix_class
                    break

        # if not, drop it
        if cnt_class is None:
            continue

        entries.append((key, val, cnt_class))

    # second-level sorting inside each class
    def sort_priority(key: str) -> int:
        if key.startswith(("l1i_ref", "l1d_ref")):
            return 0
        if key == "ret_int":
            return 1
        return 2

    # sort by class group, then priority within class, then alphabetically
    entries.sort(key=lambda e: (class_rank[e[2]], sort_priority(e[0]), e[0]))
    return entries

def plot_counters_bar(core: dict, test_title: str, args: argparse.Namespace):
    entries = classify_and_sort_counters(core)
    df_cnt = pd.DataFrame(entries, columns=["counter", "count", "class"])
    # reorder columns to class, counter, count
    df_cnt = df_cnt[["class", "counter", "count"]]
    df_cnt["count_e"] = df_cnt["count"].apply(lambda x: FMT(x, None))

    ipc = core.get("ipc", 0)
    title = f"Performance Counters for '{test_title}'<br>IPC: {ipc:.3f}"
    # scale up 2x wider if complete cosim counters are used
    sc = 2 if len(entries) > 20 else 1.2

    fig = px.bar(
        df_cnt,
        x="counter",
        y="count",
        color="class",
        color_discrete_map=BAR_COLOR_MAP,
        category_orders={"counter": [e[0] for e in entries]},
        title=title,
        text="count_e",
        width=FIG_WIDTH*sc,
        height=FIG_HEIGHT,
    )

    fig.update_traces(
        textposition="outside",
        textfont_size=9,
    )

    fig.update_layout(
        xaxis_tickangle=-45,
        title_x=0.5,
        title_font=dict(color="black", size=20),
        margin=dict(t=60, l=80, r=20, b=120),
        xaxis_title=None,
        yaxis_title="count",
        plot_bgcolor="white",
        paper_bgcolor="white",
        #xaxis=dict(gridcolor="#ddd", linecolor="#ccc"),
        yaxis=dict(gridcolor="#ddd", linecolor="#ccc", tickformat=".2s"),
    )

    log_txt = f"{title.split('<br>')[0]}\n{df_cnt.to_string(index=False)}"
    if not args.silent:
        print(log_txt)
        fig.show(renderer=args.renderer, width=FIG_WIDTH*sc, height=FIG_HEIGHT)

    if args.save_png:
        png_path = args.hw_stats.replace(".json", "_all_counters.png")
        fig.write_image(png_path, width=FIG_WIDTH*sc, height=FIG_HEIGHT)
        print(f"Saved PNG chart to: '{png_path}'")

    if args.save_svg:
        svg_path = args.hw_stats.replace(".json", "_all_counters.svg")
        fig.write_image(svg_path, width=FIG_WIDTH*sc, height=FIG_HEIGHT)
        print(f"Saved SVG chart to: '{svg_path}'")

    return log_txt

def get_stats(data: dict) -> dict:
    # try to derive stats from core stats if not present in the data
    # (e.g. when collected from emulation/runtime)

    def get_hr(hits: int, references: int, precision: int = 2) -> float:
        hr = (hits / references) * 100
        return round(hr, precision) if hr > 0 else 0

    def get_mpki(misses: int, ret: int, precision: int = 2) -> float:
        mpki = (misses / (ret / 1000))
        return round(mpki, precision) if mpki > 0 else 0

    cnt = data["core"]
    if "icache" not in data:
        icache_keys = ["l1i_ref", "l1i_miss", "ret"]
        if all(k in cnt for k in icache_keys):
            hits = cnt["l1i_ref"] - cnt["l1i_miss"]
            data["icache"] = {
                "references": cnt["l1i_ref"],
                "hits": {"all": hits},
                "misses": {"all": cnt["l1i_miss"]},
                "hr": get_hr(hits, cnt["l1i_ref"]),
                "mpki": get_mpki(cnt["l1i_miss"], cnt["ret"])
            }

    if "dcache" not in data:
        dcache_keys = ["l1d_ref", "l1d_miss", "ret"]
        if all(k in cnt for k in dcache_keys):
            hits = cnt["l1d_ref"] - cnt["l1d_miss"]
            data["dcache"] = {
                "references": cnt["l1d_ref"],
                "hits": {"all": hits},
                "misses": {"all": cnt["l1d_miss"]},
                "hr": get_hr(hits, cnt["l1d_ref"]),
                "mpki": get_mpki(cnt["l1d_miss"], cnt["ret"])
            }

    if "bpred" not in data:
        bpred_keys = ["bp_miss", "ret_ctrl_flow_br", "ret"]
        if all(k in cnt for k in bpred_keys):
            predicted = cnt["ret_ctrl_flow_br"] - cnt["bp_miss"]
            data["bpred"] = {
                "predicted": predicted,
                "mispredicted": cnt["bp_miss"],
                "accuracy": get_hr(predicted, cnt["ret_ctrl_flow_br"]),
                "mpki": get_mpki(cnt["bp_miss"], cnt["ret"])
            }

    out = ""
    if "core" in data:
        cnt = data['core']
        out += f"\ncore:\n{INDENT}"
        out += f"Cycles: {FMT(cnt['cycles'])}, "
        out += f"Retired: {FMT(cnt['ret'])}, "
        out += f"Empty: {FMT(cnt['cycles'] - cnt['ret'])}, "
        out += f"IPC: {cnt['ipc']:.3f} "
    else:
        out += f"core: N/A "

    def format_cache_stats(cache: dict) -> str:
        sd = lambda d: sum(d.values())
        return f"Ref: {FMT(cache['references'])}, " \
            f"H: {FMT(sd(cache['hits']))}, " \
            f"M: {FMT(sd(cache['misses']))}, " \
            f"HR: {cache['hr']:.2f}%, " \
            f"MPKI: {cache['mpki']:.2f} "

    out += f"\nicache:\n{INDENT}"
    out += format_cache_stats(data['icache']) if 'icache' in data else "N/A"

    out += f"\ndcache:\n{INDENT}"
    out += format_cache_stats(data['dcache']) if 'dcache' in data else "N/A"

    out += f"\nbpred:\n{INDENT}"
    if "bpred" in data:
        bp = data['bpred']
        out += f"P: {FMT(bp['predicted'])}, "
        out += f"M: {FMT(bp['mispredicted'])}, "
        out += f"ACC: {bp['accuracy']:.2f}%, "
        out += f"MPKI: {bp['mpki']:.2f} "
    else:
        out += f"N/A"

    return out

def main(args: argparse.Namespace):
    if not os.path.exists(args.hw_stats):
        raise FileNotFoundError(f"File '{args.hw_stats}' not found")

    with open(args.hw_stats, 'r') as f:
        data = json.load(f)

    d = data['core']
    row = [
        ['lost', 'bad_spec', d['bad_spec']],
        ['lost', 'other', d['lost_other']],
        ["frontend", "icache", d['stall_l1i']],
        ["frontend", "core", d['stall_fe_core']],
        ["backend", "dcache", d['stall_l1d']],
        ["backend", "core", d['stall_be_core']],
        ["retiring", "integer", d['ret_int']],
        ["retiring", "simd", d['ret_simd']],
    ]
    col = ["L1", "L2", "cycles"]
    df_tda = pd.DataFrame(row, columns=col)
    df_tda["cycles_e"] = df_tda["cycles"].apply(lambda x: FMT(x, None))

    ret = df_tda[df_tda["L1"] == "retiring"]["cycles"].sum()
    cycles = df_tda["cycles"].sum()
    ipc = ret / cycles
    ipc = round(ipc, 3) if ipc > 0 else "N/A"
    df_tda["root"] = f"pipeline<br>IPC: {ipc}"

    if "ipc" not in d:
        d["ipc"] = ipc
    if "cycles" not in d:
        d["cycles"] = cycles
    if "ret" not in d:
        d["ret"] = ret
    if "empty" not in d:
        d["empty"] = d["cycles"] - d["ret"]
    if "lost" not in d:
        d["lost"] = d['bad_spec'] + d['lost_other']
    if "stalls" not in d:
        d["stalls"] = d["cycles"] - d["ret"] - d["lost"]

    # make new df that's a group and sum on L1 only
    #df_tda_l1 = df_tda.groupby(col[0]).agg({col[2]: "sum"}).reset_index()
    #df_tda_l1.to_csv(args.hw_stats.replace(".json", "_tda_l1.csv"),index=False)

    fig = px.sunburst(
        df_tda,
        path=["root"] + col[:-1], # 'root' + 'L1' + 'L2'
        values=col[2], # 'cycles' column
        branchvalues="total",
        color=col[0], # color by 'L1' column
        color_discrete_map=COLOR_MAP,
        width=FIG_WIDTH,
        height=FIG_HEIGHT,
    )

    fig.update_traces(
        textinfo="label+percent entry",
        root=dict(color="rgba(0,0,0,0)"),
        # breakdown of the template:
        # %{label} -> name
        # %{count} -> number
        # %{percentRoot:.1%} -> % of the total (to 1 decimal)
        hovertemplate=
            '<b>%{label}</b><br>%{percentRoot:.1%}<br>%{count}<extra></extra>'
    )

    # add title
    title = args.title or f"{get_test_title(args.hw_stats)}"
    fig.update_layout(
        title_text=f"TDA for '{title}'",
        title_x=0.5,
        title_font=dict(color="black", size=20)
    )

    fig.update_layout(
        #template="plotly_dark",
        margin=dict(t=60, l=0, r=0, b=30),
    )

    log_txt = f"{title}\nIPC: {ipc}\n{df_tda.drop(columns=['root'])}"
    if not args.silent:
        print(log_txt)
        fig.show(renderer=args.renderer, width=FIG_WIDTH, height=FIG_HEIGHT)

    log_txt += "\n\n" + plot_counters_bar(d, title, args)

    if args.get_stats:
        stats_txt = get_stats(data)
        log_txt += "\n\n" + stats_txt
        if not args.silent:
            print(stats_txt)

    if args.save_png:
        png_path = args.hw_stats.replace(".json", "_tda.png")
        fig.write_image(png_path, width=FIG_WIDTH, height=FIG_HEIGHT)
        print(f"Saved PNG chart to: '{png_path}'")

    if args.save_svg:
        svg_path = args.hw_stats.replace(".json", "_tda.svg")
        fig.write_image(svg_path, width=FIG_WIDTH, height=FIG_HEIGHT)
        print(f"Saved SVG chart to: '{svg_path}'")

    if args.save_log:
        log_path = args.hw_stats.replace(".json", "_summary.log")
        with open(log_path, "w") as f:
            f.write(log_txt)
        print(f"Saved log to: '{log_path}'")

    if args.save_hw_stats:
        hw_stats_path = args.hw_stats.replace("_raw.json", "_hw_stats.json")
        with open(hw_stats_path, "w") as f:
            json.dump(data, f, indent=4)
        print(f"Saved hw_stats to: '{hw_stats_path}'")

def parse_args():
    parser = argparse.ArgumentParser(description="Plot TDA")
    parser.add_argument("hw_stats", help="Path to 'hw_stats.json' from RTL simulation for the given workload")
    parser.add_argument('-t', '--title', default=None, help="Title to use for the plots. If not provided, the title will be the test name.")
    parser.add_argument('-r', '--renderer', default='browser', help="Plotly renderer to use")
    parser.add_argument('-s', '--silent', action='store_true', help="Don't display plots")
    parser.add_argument("-p", "--places", type=int, default=1, help="Number of decimal places for formatted output (default: 1)")
    parser.add_argument('--get_stats', default=False, action='store_true', help="Print stats (from json or derived from counters) to the stdout")
    parser.add_argument('--save_hw_stats', action='store_true', help="Save hw_stats.json to a file")
    parser.add_argument('--save_png', action='store_true', help="Save plots as PNG")
    parser.add_argument('--save_svg', action='store_true', help="Save plots as SVG")
    parser.add_argument('--save_log', action='store_true', help="Save log to a file")
    return parser.parse_args()

if __name__ == "__main__":
    args = parse_args()
    FMT = EngFormatter(unit='', places=args.places, sep="")
    main(args)
