#!/usr/bin/env python3

import argparse
import json
import os
import platform
import subprocess
import time

import regex
import yaml
from utils import INDENT, SIM_EARLY_EXIT_STRING, SIM_PASS_STRING, get_reporoot

reporoot = get_reporoot()

def default_isa_sim() -> str:
    return os.path.join(reporoot, "src", "build", "ama-riscv-sim")

def load_config(cfg_path: str):
    with open(cfg_path, "r") as f:
        cfg = yaml.safe_load(f) or {}

    isa_sim_args = cfg.get("isa_sim_args") or []
    if not isinstance(isa_sim_args, list):
        raise ValueError("'isa_sim_args' must be a list of CLI tokens")
    # split on whitespace so "--arg value" pairs can share a line
    # numbers (e.g. addresses) are coerced to str
    isa_sim_args = [tok for a in isa_sim_args for tok in str(a).split()]

    workloads = cfg.get("workloads")
    if not workloads:
        raise ValueError("'workloads' is required and must be non-empty")
    if not isinstance(workloads, list):
        raise ValueError("'workloads' must be a list of paths")

    return isa_sim_args, workloads

def resolve_workloads(workloads, filters):
    # resolve relative paths against the repo root, keep absolute as-is
    resolved = [p if os.path.isabs(p) else os.path.join(reporoot, p)
                for p in workloads]

    if filters:
        resolved = [p for p in resolved
                    if any(regex.search(f, p) for f in filters)]
        print(f"Filter enabled. Patterns: {filters}. "
              f"Number of workloads after filter: {len(resolved)}")

    if len(resolved) == 0:
        raise FileNotFoundError("No workloads to run (check filter/config)")

    missing = [p for p in resolved if not os.path.exists(p)]
    if missing:
        raise FileNotFoundError(
            "Workload elf(s) not found:\n" +
            "\n".join(f"{INDENT}{p}" for p in missing))

    return resolved

def parse_inst_counts(stdout: str):
    # the 'profiled' field is only present in PROFILERS_EN builds
    m = regex.search(
        r"Instruction Counters: executed: (\d+)(?:, profiled: (\d+))?", stdout)
    if not m:
        return None, None
    executed = int(m.group(1))
    profiled = int(m.group(2)) if m.group(2) is not None else None
    return executed, profiled

def is_pass(stdout: str) -> bool:
    return (SIM_PASS_STRING in stdout) or (SIM_EARLY_EXIT_STRING in stdout)

# host environment info
def get_build_info(sim: str) -> dict:
    try:
        out = subprocess.run(
            [sim, "--version"], capture_output=True, text=True).stdout
    except OSError:
        return {}
    info = {}
    for line in out.splitlines():
        if ":" in line:
            k, v = line.split(":", 1)
            info[k.strip()] = v.strip()
    return info

def get_governor() -> str:
    path = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
    try:
        with open(path) as f:
            return f.read().strip()
    except OSError:
        return "unknown"

def get_cpu_model() -> str:
    try:
        with open("/proc/cpuinfo") as f:
            for line in f:
                if line.startswith("model name"):
                    return line.split(":", 1)[1].strip()
    except OSError:
        pass
    return platform.processor() or "unknown"

def perf_available() -> bool:
    # probe, fails when kernel.perf_event_paranoid blocks user counters
    try:
        r = subprocess.run(
            ["perf", "stat", "-x,", "-e", "instructions", "true"],
            capture_output=True,
            text=True
        )
    except OSError:
        return False
    return r.returncode == 0

# shared CLI args and run scaffolding
def add_common_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("-c", "--config", required=True, help="YAML config (workloads + isa_sim_args)")
    parser.add_argument("--isa_sim", default=default_isa_sim(), help="Path to the ISA sim binary (defaults to the build/ one)")
    parser.add_argument("-f", "--filter", type=str, nargs='*', help="Regex pattern(s); only run workloads whose path matches any")
    parser.add_argument("--work_dir", default=os.getcwd(), help="Directory to run in and store sim outputs")
    parser.add_argument("--pin", type=int, default=None, help="Pin to a CPU core via taskset -c")
    parser.add_argument("--json", dest="json_out", default=None, help="Write results JSON to this path")

def prepare(args):
    # abspath: workloads run with cwd=work_dir, so a relative sim path breaks
    args.isa_sim = os.path.abspath(args.isa_sim)
    if not os.path.exists(args.config):
        raise FileNotFoundError(f"Config not found at: {args.config}")
    if not os.path.exists(args.isa_sim):
        raise FileNotFoundError(f"Simulator not found at: {args.isa_sim}")
    if not os.path.exists(args.work_dir):
        os.makedirs(args.work_dir)
    isa_sim_args, workloads = load_config(args.config)
    workloads = resolve_workloads(workloads, args.filter)
    return isa_sim_args, workloads

def sim_cmd(sim, app, sim_args, pin):
    prefix = ["taskset", "-c", str(pin)] if pin is not None else []
    return prefix + [sim, app] + sim_args

def host_meta(isa_sim, sim_args, pin=None, **extra) -> dict:
    meta = {
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "host": platform.node(),
        "cpu_model": get_cpu_model(),
        "governor": get_governor(),
        "isa_sim": isa_sim,
        "build_info": get_build_info(isa_sim),
        "sim_args": sim_args,
        "pinned_cpu": pin,
    }
    meta.update(extra)
    return meta

def write_json(path, obj) -> None:
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "w") as f:
        json.dump(obj, f, indent=2)
