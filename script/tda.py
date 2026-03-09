#!/usr/bin/env python3

import argparse
import json
import os

import pandas as pd
import plotly.express as px
from utils import get_test_title, smarter_eng_formatter

PLOTLY_COLORS = px.colors.qualitative.Plotly

COLOR_MAP = {
    "retiring": PLOTLY_COLORS[0], # #636efa (Blue)
    "backend": PLOTLY_COLORS[2], # #00CC96 (Green)
    "frontend": PLOTLY_COLORS[1], # #EF553B (Red)
    "bad_spec": PLOTLY_COLORS[4], # #FFA15A (Orange/Yellow)
    "(?)": "rgba(0,0,0,0)" # fallback for undefined categories
}

CLASS_ORDER = [
    "cycles", "ret", "stall", "bad_spec", "ret_*", "stall_*", "l1i_*", "l1d_*"]
DROP_KEYS = {"cpi", "ipc"}
EXACT_CLASS = {
    "cycles": "cycles", "ret": "ret", "stalls": "stall", "bad_spec": "bad_spec"}
PREFIX_CLASS = [
    ("ret_", "ret_*"), ("stall_", "stall_*"),
    ("l1i_", "l1i_*"), ("l1d_", "l1d_*")
]
BAR_COLOR_MAP = {cls: PLOTLY_COLORS[i] for i, cls in enumerate(CLASS_ORDER)}

def classify_and_sort_counters(core: dict) -> list[tuple[str, int, str]]:
    """Classify core counters into groups and sort by class order then name."""
    class_rank = {c: i for i, c in enumerate(CLASS_ORDER)}
    entries = []
    for key, val in core.items():
        if key in DROP_KEYS:
            continue
        cls = EXACT_CLASS.get(key)
        if cls is None:
            for prefix, prefix_cls in PREFIX_CLASS:
                if key.startswith(prefix):
                    cls = prefix_cls
                    break
        if cls is None:
            continue
        entries.append((key, val, cls))
    SORT_FIRST = {"ret_int"} # special case to override the default sorting
    entries.sort(
        key=lambda e: (class_rank[e[2]], 0 if e[0] in SORT_FIRST else 1, e[0]))
    return entries

def plot_counters_bar(core: dict, test_title: str, args: argparse.Namespace):
    entries = classify_and_sort_counters(core)
    df = pd.DataFrame(entries, columns=["counter", "value", "class"])

    eng_fmt = smarter_eng_formatter(places=1)
    df["text"] = df["value"].apply(lambda x: eng_fmt(x, None))

    ipc = core.get("ipc", 0)
    title = f"Performance Counters for '{test_title}'(IPC: {ipc:.3f})"

    fig = px.bar(
        df,
        x="counter",
        y="value",
        color="class",
        color_discrete_map=BAR_COLOR_MAP,
        category_orders={"counter": [e[0] for e in entries]},
        title=title,
        text="text",
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

    if not args.silent:
        print(title)
        print(df.to_string(index=False))
        fig.show(renderer=args.renderer)

    if args.save_png:
        png_path = args.hw_stats.replace(".json", "_counters.png")
        fig.write_image(png_path, scale=2)

    if args.save_svg:
        svg_path = args.hw_stats.replace(".json", "_counters.svg")
        fig.write_image(svg_path)

def main(args: argparse.Namespace):
    if not os.path.exists(args.hw_stats):
        raise FileNotFoundError(f"File '{args.hw_stats}' not found")

    with open(args.hw_stats, 'r') as f:
        data = json.load(f)

    d = data['core']
    row = [
        ['bad_spec', pd.NA, d['bad_spec']],
        ["frontend", "icache", d['stall_l1i']],
        ["frontend", "core", d['stall_fe_core']],
        ["backend", "dcache", d['stall_l1d']],
        ["backend", "core", d['stall_be_core']],
        ["retiring", "integer", d['ret_int']],
        ["retiring", "simd", d['ret_simd']],
    ]
    col = ["L1", "L2", "cycles"]
    df_tda = pd.DataFrame(row, columns=col)
    df_tda["root"] = "pipeline"

    # make new df that's a group and sum on L1 only
    df_tda_l1 = df_tda.groupby(col[0]).agg({col[2]: "sum"}).reset_index()
    df_tda_l1.to_csv(args.hw_stats.replace(".json", "_tda_l1.csv"), index=False)

    fig = px.sunburst(
        df_tda,
        path=["root"] + col[:-1], # 'root' + 'L1' + 'L2'
        values=col[2], # 'cycles' column
        branchvalues="total",
        color=col[0], # color by 'L1' column
        color_discrete_map=COLOR_MAP
    )

    fig.update_traces(
        textinfo="label+percent entry",
        root=dict(color="rgba(0,0,0,0)"),
        # breakdown of the template:
        # %{label} -> name
        # %{value} -> number
        # %{percentRoot:.1%} -> % of the total (to 1 decimal)
        hovertemplate=
            '<b>%{label}</b><br>%{percentRoot:.1%}<br>%{value}<extra></extra>'
    )

    # add title
    title = f"TDA for '{get_test_title(args.hw_stats)}'"
    fig.update_layout(
        title_text=title,
        title_x=0.5,
        title_font=dict(color="black", size=20)
    )

    fig.update_layout(
        #template="plotly_dark",
        margin=dict(t=60, l=0, r=0, b=30),
    )

    if not args.silent:
        print(title)
        print(df_tda.drop(columns=["root"]))
        fig.show(renderer=args.renderer)

    if args.save_png:
        png_path = args.hw_stats.replace(".json", "_tda.png")
        fig.write_image(png_path, scale=2)

    if args.save_svg:
        svg_path = args.hw_stats.replace(".json", "_tda.svg")
        fig.write_image(svg_path)

    plot_counters_bar(d, get_test_title(args.hw_stats), args)

def parse_args():
    parser = argparse.ArgumentParser(description="Plot TDA")
    parser.add_argument("hw_stats", help="Path to 'hw_stats.json' from RTL simulation for the given workload")
    parser.add_argument('-r', '--renderer', default='browser', help="Plotly renderer to use")
    parser.add_argument('-s', '--silent', action='store_true', help="Don't display plot")
    parser.add_argument('--save_png', action='store_true', help="Save plot as PNG")
    parser.add_argument('--save_svg', action='store_true', help="Save plot as SVG")
    return parser.parse_args()

if __name__ == "__main__":
    args = parse_args()
    main(args)
