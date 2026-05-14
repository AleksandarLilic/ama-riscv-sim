#!/usr/bin/env python3

import argparse
import json
import math
import os
import shutil
import subprocess
from pathlib import Path

import pandas as pd

script_dir = Path(__file__).resolve().parent
EMBENCH_DIR = script_dir / "../sw/baremetal/embench"
BASELINE_SPEED = (EMBENCH_DIR / "_info/baseline-data_1.0/speed.json").resolve()
BASELINE_SIZE = (EMBENCH_DIR / "_info/baseline-data_1.0/size.json").resolve()

CPU_MHZ_DEF = 100.0
ARCH_DEF = "rv32im_xsimd"
DESC_DEF = "ama-riscv RV32I GCC 14.2.1 -O2"

def parse_args():
    parser = argparse.ArgumentParser(description="Compute Embench IoT results")
    parser.add_argument("--speed", default=BASELINE_SPEED, help=f"Path to speed baseline .json file (default: {BASELINE_SPEED})")
    parser.add_argument("--size", default=BASELINE_SIZE, help=f"Path to size baseline .json file (default: {BASELINE_SIZE})")
    parser.add_argument("-r", "--results", required=True, help="Path to results CSV file (needs 'name' and 'cycles' columns)")
    parser.add_argument("-c", "--cpu_mhz", type=float, default=CPU_MHZ_DEF, help=f"Clock frequency in MHz used to compile/run tests (default: {CPU_MHZ_DEF})")
    parser.add_argument("-o", "--output", default=None, help="Output JSON file path (default: stdout)")
    parser.add_argument("--report", default=None, help="Output markdown report file path (default: stdout)")
    parser.add_argument("--arch", default=ARCH_DEF, help=f"Architecture label for summary table (default: {ARCH_DEF})")
    parser.add_argument("--desc", default=DESC_DEF, help=f"Benchmark description for summary table (default: {DESC_DEF})")
    return parser.parse_args()

def read_baseline(path):
    with open(path) as f:
        return json.load(f)

def read_results(path):
    df = pd.read_csv(path, usecols=["name", "cycles"])
    df["name"] = df["name"].str.removeprefix("embench_")
    return dict(zip(df["name"], df["cycles"]))

# Formulas from embench-iot: pylib/embench_core.py
# (compute_geomean, compute_geosd)
def compute_geomean(values):
    return pow(math.prod(values), 1.0 / len(values))

def compute_geosd(values, geomean):
    lnsize = sum(math.log(v / geomean) ** 2 for v in values)
    return math.exp(math.sqrt(lnsize / len(values)))

def get_size_tool():
    rv_gnu = os.environ.get("RV_GNU_LATEST")
    if not rv_gnu:
        raise EnvironmentError("RV_GNU_LATEST environment variable is not set")

    size_tool = f"{rv_gnu}-size"
    if not shutil.which(size_tool):
        raise FileNotFoundError(f"'{size_tool}' not found on PATH")

    return size_tool

def get_elf_sizes(baseline_size, size_tool):
    """Run size tool on each benchmark ELF; return dict of {name: text_size}."""
    sizes = {}
    for bench in baseline_size:
        elf = EMBENCH_DIR / bench / "embench.elf"
        if not elf.exists():
            raise FileNotFoundError(f"ELF not found: {elf}")

        res = subprocess.run(
            [size_tool, "-G", str(elf)],
            capture_output=True, text=True, check=True,
        )
        # Output format:
        #       text       data        bss      total filename
        #       7192       1028        216       8436 aha-mont64/embench.elf
        fields = res.stdout.splitlines()[1].split()
        sizes[bench] = int(fields[0])

    return sizes

def format_report(
    abs_size, rel_size, size_geomean, size_geosd,
    abs_speed, rel_speed, speed_geomean, speed_geosd,
    cpu_mhz, arch, desc
):
    benchmarks = list(abs_size.keys())

    # detailed per-benchmark results
    rows = {b: [abs_size[b], rel_size[b], abs_speed[b], rel_speed[b]]
            for b in benchmarks}
    rows["Geo. mean"] = ["", round(size_geomean, 2), "", round(speed_geomean,2)]
    rows["Geo. SD"] = ["", round(size_geosd, 2), "", round(speed_geosd, 2)]

    detail_df = pd.DataFrame.from_dict(
        rows, orient="index",
        columns=["Size Abs", "Size Rel", "Speed Abs", "Speed Rel"],
    )

    # summary scores
    # Speed/MHz = relative speed geomean; Speed = Speed/MHz * cpu_mhz
    # range from embench-iot-results: embres/data.py
    sz_gm, sz_gsd = size_geomean, size_geosd
    sp_gm, sp_gsd = speed_geomean, speed_geosd
    c = cpu_mhz
    summary_rows = [
        ("Size",      sz_gm,     sz_gm / sz_gsd,     sz_gm * sz_gsd),
        ("Speed",     sp_gm * c, sp_gm / sp_gsd * c, sp_gm * sp_gsd * c),
        ("Speed/MHz", sp_gm,     sp_gm / sp_gsd,     sp_gm * sp_gsd),
    ]
    summary_df = pd.DataFrame(
        [(t, round(s, 2), f"{lo:.2f} - {hi:.2f}")
         for t, s, lo, hi in summary_rows],
        columns=["Type", "Score", "Range"],
    )

    lines = [
        "***Summary***  ",
        "",
        f"Architecture:  `{arch}`  ",
        f"Description:   `{desc}`  ",
        f"CPU frequency: {int(cpu_mhz)} MHz  ",
        "",
        summary_df.to_markdown(index=False),
        "",
        "***Detailed Embench results***",
        "",
        detail_df.to_markdown(),
        "",
    ]
    return "\n".join(lines)

def main():
    args = parse_args()
    def_str = "default" if args.speed == BASELINE_SPEED else ""
    print(f"Using {def_str} '{args.speed}' speed baseline file")

    baseline_speed = read_baseline(args.speed)
    results = read_results(args.results)

    abs_speed = {}
    rel_speed = {}

    for bench, baseline_ms in baseline_speed.items():
        if bench not in results:
            raise ValueError(f"Benchmark '{bench}' not found in results CSV")

        time_ms = results[bench] / args.cpu_mhz / 1000
        abs_speed[bench] = round(time_ms)
        rel_speed[bench] = round(baseline_ms / time_ms, 2)

    speed_rel_values = list(rel_speed.values())
    speed_geomean = compute_geomean(speed_rel_values)
    speed_geosd = compute_geosd(speed_rel_values, speed_geomean)

    baseline_size = read_baseline(args.size)
    def_str = "default" if args.size == BASELINE_SIZE else ""
    print(f"Using {def_str} '{args.size}' size baseline file")

    size_tool = get_size_tool()
    elf_sizes = get_elf_sizes(baseline_size, size_tool)

    abs_size = {}
    rel_size = {}

    # size ratio: elf_size / baseline_size
    # from embench-iot: benchmark_size.py
    for bench, sections in baseline_size.items():
        baseline_text = sections["text"]
        abs_size[bench] = elf_sizes[bench]
        rel_size[bench] = round(elf_sizes[bench] / baseline_text, 2)

    size_rel_values = list(rel_size.values())
    size_geomean = compute_geomean(size_rel_values)
    size_geosd = compute_geosd(size_rel_values, size_geomean)

    output = {
        "absolute size results": {
            "detailed results": abs_size,
        },
        "relative size results": {
            "detailed results": rel_size,
            "geometric mean": round(size_geomean, 2),
            "geometric standard deviation": round(size_geosd, 2),
        },
        "absolute speed results": {
            "detailed results": abs_speed,
        },
        "relative speed results": {
            "detailed results": rel_speed,
            "geometric mean": round(speed_geomean, 2),
            "geometric standard deviation": round(speed_geosd, 2),
        },
    }

    json_str = json.dumps(output, indent=2)

    if args.output:
        with open(args.output, "w") as f:
            f.write(json_str + "\n")
        print(f"Saved results to {args.output}")
    else:
        print(json_str)

    report = format_report(
        abs_size, rel_size, size_geomean, size_geosd,
        abs_speed, rel_speed, speed_geomean, speed_geosd,
        args.cpu_mhz, args.arch, args.desc,
    )

    if args.report:
        with open(args.report, "w") as f:
            f.write(report)
        print(f"Saved report to {args.report}")
    else:
        print(report)

if __name__ == "__main__":
    main()
