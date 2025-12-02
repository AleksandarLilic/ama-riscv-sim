#!/usr/bin/env python3

import argparse
import json
import os

import pandas as pd
import plotly.express as px
from utils import get_test_title

PLOTLY_COLORS = px.colors.qualitative.Plotly

COLOR_MAP = {
    "retiring": PLOTLY_COLORS[0], # #636efa (Blue)
    "backend": PLOTLY_COLORS[2], # #00CC96 (Green)
    "frontend": PLOTLY_COLORS[1], # #EF553B (Red)
    "bad_spec": PLOTLY_COLORS[4], # #FFA15A (Orange/Yellow)
    "(?)": "rgba(0,0,0,0)" # fallback for undefined categories
}

def main(args: argparse.Namespace):
    if not os.path.exists(args.hw_stats):
        raise FileNotFoundError(f"File '{args.hw_stats}' not found")

    with open(args.hw_stats, 'r') as f:
        data = json.load(f)

    col = ["L1", "L2", "cycles"]
    df = pd.DataFrame(data["core"], columns=col)
    df["root"] = "pipeline"
    df = df.replace("None", pd.NA) # replace literal "None" with pandas NaN

    # make new df that's a group and sum on L1 only
    df_l1 = df.groupby(col[0]).agg({col[2]: "sum"}).reset_index()
    df_l1.to_csv(args.hw_stats.replace(".json", "_tda_l1.csv"), index=False)

    fig = px.sunburst(
        df,
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
        print(df.drop(columns=["root"]))
        fig.show(renderer=args.renderer)

    if args.save_png:
        png_path = args.hw_stats.replace(".json", "_tda.png")
        fig.write_image(png_path, scale=2)

    if args.save_svg:
        svg_path = args.hw_stats.replace(".json", "_tda.svg")
        fig.write_image(svg_path)

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
