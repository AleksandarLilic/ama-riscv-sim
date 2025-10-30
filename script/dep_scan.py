#!/usr/bin/env python3
import argparse
from collections import deque

import matplotlib.pyplot as plt
from matplotlib.ticker import EngFormatter
from perf_est_v2 import smarter_eng_formatter
from utils import INDENT, get_test_title

BRANCH_MNM = {"beq", "bne", "blt", "bge", "bltu", "bgeu"}
STORE_MNM  = {"sb", "sh", "sw"} # rv32 only
FENCE_MNM  = {"fence", "fence.i"}
ALL_MNM = BRANCH_MNM.union(STORE_MNM).union(FENCE_MNM)

def parse_args():
    parser = argparse.ArgumentParser(description="Count register dependencies within a lookahead window from an ISA sim exec log.")
    parser.add_argument("exec_log", help="Path to exec.log")
    parser.add_argument("--src", required=True, help="Comma-separated list of source instruction mnemonics (e.g. 'addi,add,auipc')")
    parser.add_argument("--dep", required=True, help="Comma-separated list of dependent instruction mnemonics (e.g. 'add,bne,sw')")
    parser.add_argument("--window", type=int, required=True, help="Lookahead window size (number of subsequent executed instructions to inspect)")
    return parser.parse_args()

def is_valid_line(line: str) -> bool:
    """
    Only consider executed-instruction lines: they start with whitespace
    """
    return len(line) > 0 and line[0].isspace()

def extract_seq_num(line: str) -> int | None:
    """
    Return the leftmost decimal instruction counter (e.g. 324 in '   324: ...')
    """
    return line.strip().split(" ")[0].split(":")[0]

def extract_mnemonic(line: str) -> str | None:
    try:
        parts = line.split(":")
        # parts[2]: " fdd00593 addi x11,x0,-35          x11"
        tokenized = parts[2].strip().split()
        if len(tokenized) >= 2:
            return tokenized[1].lower()
    except Exception:
        # couldn't parse line properly, dc about this line
        return None

def extract_regs_in_order(line: str) -> list[int]:
    """
    Return list of integer register IDs (0..31) matched in left-to-right order
    """
    regs = line.strip().split(" ")[4].split(",")
    # for loads and stores, reg in parentheses
    regs = [r.replace('(', ' ').replace(')', '').split() for r in regs]
    regs = [item for sublist in regs for item in sublist] # flatten
    regs = [int(r[1:]) for r in regs if r.startswith('x')]
    return regs

def inst_writes_rd(mnemonic: str) -> bool:
    """
    Heuristic: most non-branch/non-store/non-fence GPR ops write rd
    """
    if mnemonic in ALL_MNM:
        return False
    # ECALL/EBREAK don't write; treat them as no-write
    if mnemonic in {"ecall", "ebreak"}:
        return False
    # everything else in rv32im_zicsr assumed to write rd
    # (e.g., jal, jalr, lui, auipc, addi, csrrw, etc.)
    return True

def dep_rs(mnemonic: str, regs_in_order: list[int]) -> list[int]:
    """
    For dependent side:
      - store/branch: first two x regs are sources
      - other: first x is rd, sources are the remainder (rs1/rs2/…)
    Always ignore x0 as a source (not a true dependency).
    """
    if (mnemonic in STORE_MNM) or (mnemonic in BRANCH_MNM):
        srcs = regs_in_order[:2]
    else:
        srcs = regs_in_order[1:] if regs_in_order else []
    # drop x0 if present
    return [r for r in srcs if r != 0]

def src_rd(mnemonic: str, regs_in_order: list[int]) -> int | None:
    """
    Producer rd is the first register on the line for writer instructions.
    Skip stores/branches/fences and any rd==x0
    """
    if not inst_writes_rd(mnemonic):
        return None
    if not regs_in_order:
        return None
    rd = regs_in_order[0]
    if rd == 0:
        return None  # Skip x0 producers
    return rd

def main():
    args = parse_args()
    set_src = {m.strip().lower() for m in args.src.split(",") if m.strip()}
    set_stl = {m.strip().lower() for m in args.dep.split(",") if m.strip()}
    window = args.window
    if window < 1:
        raise SystemExit("Window must be >= 1")
    elif window > 32:
        # simply to prevent overly large searches by mistake, increase if needed
        raise SystemExit("Window too large; max 32 supported")

    # dep_arr: index i counts distance i (i==0 unused, 1..window used)
    dep_arr = [0] * (window + 1)

    # dep_lines: index i holds a list of dep line numbers for that distance
    dep_lines = [[] for _ in range(window + 1)]

    # load all lines (need lookahead)
    with open(args.exec_log, "r", encoding="utf-8", errors="replace") as f:
        lines = f.readlines()

    # pre-filter valid instruction line indices
    # to define "next instruction" semantics cleanly
    inst_idxs = [i for i, ln in enumerate(lines) if is_valid_line(ln)]

    n = len(inst_idxs)
    line_fmt_issue = 0
    for pos, li in enumerate(inst_idxs):
        line = lines[li]
        mnm = extract_mnemonic(line)

        if mnm is None:
            line_fmt_issue += 1
            continue

        if mnm not in set_src:
            continue

        regs = extract_regs_in_order(line)
        if not regs:
            # No registers parsed, dc about this line
            #line_fmt_issue += 1
            continue

        #print(regs)
        rd = src_rd(mnm, regs)
        if rd is None:
            # either non-writer or x0 -> skip as a producer
            continue

        # scan lookahead window over subsequent executed instructions
        # terminate early if a redefinition of rd occurs (kills dependency)
        max_pos = min(pos + window, n - 1)
        for nx_pos in range(pos + 1, max_pos + 1):
            nx_idx = inst_idxs[nx_pos]
            nx_line = lines[nx_idx]
            nx_mnm = extract_mnemonic(nx_line)
            if nx_mnm is None:
                line_fmt_issue += 1
                continue

            nx_regs = extract_regs_in_order(nx_line)
            if nx_regs is None:
                line_fmt_issue += 1
                continue

            # if this instruction writes the same rd,
            # kill the dependency search for this producer
            if inst_writes_rd(nx_mnm) and nx_regs:
                nx_rd = nx_regs[0]
                if nx_rd == rd:
                    # stop scanning this window
                    break

            # if this is a dependent inst, check sources for a match
            if nx_mnm in set_stl:
                srcs = dep_rs(nx_mnm, nx_regs)
                if rd in srcs:
                    dist = nx_pos - pos # distance in executed-instruction steps
                    if 1 <= dist <= window:
                        dep_arr[dist] += 1
                        seq = extract_seq_num(nx_line)
                        if seq is not None:
                            dep_lines[dist].append(int(seq))
                    # Only one increment per dep even if both rs1/rs2 == rd

    fmt = smarter_eng_formatter()
    exec_inst = fmt(len(inst_idxs))
    # move forward only if there are dependencies found
    if sum(dep_arr) == 0:
        print("No dependencies found for the given source/dependent mnemonics"+\
              f" for {exec_inst} executed instructions and window {window}.")
        return

    LN = 5
    print(f"Source inst: '{', '.join(sorted(set_src))}'\n" + \
          f"Dependent inst: '{', '.join(sorted(set_stl))}'")
    print(f"Dependency counts by distance: " + \
          f"{exec_inst} executed instructions, " + \
          f"window {window}, first {LN} sample line numbers:")

    max_digits = len(str(max(dep_arr)))
    for d in range(1, window + 1):
        print(f"{INDENT}{d}: {dep_arr[d]:>{max_digits}}  {dep_lines[d][:LN]}")

    if line_fmt_issue:
        print(f"\nSkipped lines with formatting issue or " + \
              f"otherwise ignored: {line_fmt_issue}")

    fig, ax = plt.subplots(figsize=(2+window*.8, 4.5))
    r = ax.bar(range(1, window + 1), dep_arr[1:])
    ax.bar_label(r, padding=3, fmt=fmt)
    ax.set_ylim(0, max(dep_arr[1:]) * 1.1)
    ax.set_title(
        f"Register dependency distances for {get_test_title(args.exec_log)}")
    ax.set_xlabel("Distance (next executed instruction = 1)")
    ax.set_ylabel("Count")
    ax.yaxis.set_major_formatter(EngFormatter(unit='', places=0, sep=''))
    ax.set_xticks(list(range(1, window + 1)))
    ax.grid(True, axis="y")
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
