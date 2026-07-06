#!/usr/bin/env python3

# sampling profiler for the ISA sim on the host machine
# wraps one dedicated `perf record` run per workload and
# converts it to the folded-stack format
# needs symbols (build with PERF=1)
# and a usable perf (kernel.perf_event_paranoid)

import argparse
import os
import re
import subprocess
import sys

from hw_model_sweep import get_test_name
from sim_run_utils import (add_common_args, get_build_info, host_meta, is_pass,
                           perf_available, prepare, sim_cmd, write_json)
from utils import INDENT

SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))
STACKCOLLAPSE = os.path.join(SCRIPT_PATH, "FlameGraph", "stackcollapse-perf.pl")

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Sample-profile the host execution of the ISA sim (perf record -> folded stacks)")
    add_common_args(p)
    p.add_argument("-e", "--event", default="cycles", help="perf event(s) to sample; comma-separated for multiple independent streams, each written to its own folded file (e.g. 'cycles,instructions' -> per-function IPC via prof_stats -t/-s). Comma = separate streams; a {...} group is one stream and can't be split per event")
    p.add_argument("-F", "--freq", type=int, default=999, help="perf sampling frequency in Hz")
    p.add_argument("--call-graph", dest="call_graph", choices=["fp", "dwarf"], default="fp", help="perf stack unwind method (fp needs PERF=1 frame pointers)")
    p.add_argument("--top", type=int, default=0, help="Top-N hot symbols to print to stdout")
    return p.parse_args()

def record(cmd, data, freq, cg_mode, events, cwd):
    rec = ([
        "perf", "record",
        "-F", str(freq),
        "-e", events,
        "--call-graph", cg_mode,
        "-o", data, "--"
        ] + cmd
    )
    return subprocess.run(rec, cwd=cwd, capture_output=True, text=True)

# perf script sample header: "ama-riscv-sim 1234 5.67: 890 cycles: <ip> ..."
# the event name is the token right before its trailing colon
EVENT_LINE_RE = re.compile(r":\s*\d*\s+(\S+):\s*$")

def emitted_events(script_text):
    # distinct event names perf actually emitted
    # only used to build an error msg when a requested event turns up no samples
    seen = []
    for line in script_text.splitlines():
        m = EVENT_LINE_RE.search(line)
        if m and m.group(1) not in seen:
            seen.append(m.group(1))
    return seen

def collapse(data, out_dir, ev_req):
    # one `perf script`, then split per event with stackcollapse --event-filter
    # perf prints each event under the token passed to `-e`
    # so filter on the requested name directly
    # returns (success, error_message, folded_paths_by_event_base)
    script = subprocess.run(
        ["perf", "script", "-i", data],
        cwd=out_dir, capture_output=True, text=True
    )
    if script.returncode != 0:
        return False, script.stderr, {}

    folded_map = {}
    for ev in ev_req:
        collapsed = subprocess.run(
            [STACKCOLLAPSE, "--event-filter", ev],
            input=script.stdout, capture_output=True, text=True
        )
        if collapsed.returncode != 0:
            return False, collapsed.stderr, {}
        if not collapsed.stdout.strip():
            # filter matched nothing -> event wasn't recorded (typo/not in -e)
            return False, (
                f"event '{ev}' produced no samples (recorded: "
                f"{', '.join(emitted_events(script.stdout)) or 'none'})"
            ), {}
        ev_base = ev.split(":")[0] # filesystem-clean label for the filename
        # mirror the riscv isa sim trace naming (callstack_folded_<ev>_<source>)
        folded = os.path.join(out_dir, f"callstack_folded_{ev_base}_host.txt")
        with open(folded, "w") as f:
            f.write(collapsed.stdout)
        folded_map[ev_base] = folded
    return True, "", folded_map

def parse_folded(path):
    # leaf frame gets the self-weight; sum is total weight
    self_counts = {}
    total = 0
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                stack, cnt = line.rsplit(None, 1)
                cnt = int(cnt)
            except ValueError:
                continue
            frames = [x for x in stack.split(";") if x]
            if not frames:
                continue
            self_counts[frames[-1]] = self_counts.get(frames[-1], 0) + cnt
            total += cnt
    return self_counts, total

def top_symbols(self_counts, total, n):
    ranked = sorted(self_counts.items(), key=lambda kv: kv[1], reverse=True)
    return [{
        "symbol": s,
        "self_weight": c,
        "self_pct": round(c / total * 100, 2) if total else 0.0
        } for s, c in ranked[:n]
    ]

def profile_one(sim, app, sim_args, args):
    name = get_test_name(app)
    out_dir = os.path.abspath(os.path.join(args.work_dir, name))
    os.makedirs(out_dir, exist_ok=True)
    data = os.path.join(out_dir, "perf.data")

    cmd = sim_cmd(sim, app, sim_args, args.pin)
    ev_req = [e.strip() for e in args.event.split(",") if e.strip()]

    rec = record(cmd, data, args.freq, args.call_graph, args.event, out_dir)
    passed = is_pass(rec.stdout)
    if rec.returncode != 0:
        print(
            f"{INDENT}[ERROR] {name}: perf record failed (rc={rec.returncode})"
        )
        print("\n".join(rec.stderr.splitlines()[-10:]))
        return None

    ok, err, folded_map = collapse(data, out_dir, ev_req)
    if not ok:
        print(f"{INDENT}[ERROR] {name}: folding failed\n{err}")
        return None

    # stdout glance only; render folded with get_flamegraph.py / prof_stats.py
    status = "PASS" if passed else "FAIL"
    for ev, folded in folded_map.items():
        self_counts, total = parse_folded(folded)
        print(f"{INDENT}[{status}] {name} [{ev}]  ({total:,} weight, "
              f"{len(self_counts)} symbols)")
        for s in top_symbols(self_counts, total, args.top):
            print(f"{INDENT * 2}{s['self_pct']:>6.2f}%  {s['symbol']}")

    return {
        "name": name,
        "passed": passed,
        "perf_data": data,
        "folded": folded_map,
    }

def main():
    args = parse_args()
    isa_sim_args, workloads = prepare(args)

    if not perf_available():
        sys.exit(
            "ERROR: `perf` unavailable. Sampling needs it - lower "
            "kernel.perf_event_paranoid (e.g. sudo sysctl -w "
            "kernel.perf_event_paranoid=1) or run with privileges."
        )

    if not os.path.exists(STACKCOLLAPSE):
        sys.exit(
            f"ERROR: {STACKCOLLAPSE} missing (init the FlameGraph submodule)"
        )

    build_info = get_build_info(args.isa_sim)
    cxxflags = build_info.get("CXXFLAGS", "")
    if args.call_graph == "fp" and "fno-omit-frame-pointer" not in cxxflags:
        print(
            "WARNING: sim not built with PERF=1 (no frame pointers); "
            "fp call graphs may be shallow. "
            "Rebuild with PERF=1 or use --call-graph dwarf."
        )

    print(
        f"Profiling {len(workloads)} workload(s), "
        f"events={args.event}, "
        f"freq={args.freq}Hz, "
        f"call-graph={args.call_graph}, "
        f"pin={'none' if args.pin is None else args.pin} in "
        f"'{args.work_dir}'"
    )
    print(f"sim: {args.isa_sim}")
    if isa_sim_args:
        print(f"sim args: {' '.join(isa_sim_args)}")

    results = []
    for app in workloads:
        entry = profile_one(args.isa_sim, app, isa_sim_args, args)
        if entry is not None:
            results.append(entry)

    out = {
        "meta": host_meta(
            args.isa_sim, isa_sim_args, args.pin,
            event=args.event, freq=args.freq, call_graph=args.call_graph
        ),
        "results": results,
    }

    if args.json_out:
        write_json(args.json_out, out)
        print(f"\nWrote {args.json_out}")

    sys.exit(0 if results and all(r["passed"] for r in results) else 1)

if __name__ == "__main__":
    main()
