#!/usr/bin/env python3
import argparse
from collections import deque
from dataclasses import dataclass

import matplotlib.pyplot as plt
from run_analysis import icfg
from utils import FMT_AXIS, INDENT, get_test_title, smarter_eng_formatter

BRANCH_MNM = icfg.INST_T[icfg.BRANCH]
STORE_MNM = icfg.INST_T_MEM[icfg.MEM_S]
FENCE_MNM = icfg.INST_T[icfg.FENCE]
ALL_SPECIAL_MNM = set(BRANCH_MNM + STORE_MNM + FENCE_MNM)

MNM_ANY = "_any_"

class dependency_results:
    inst_idxs = []
    dep_arr_cnt = []
    dep_line_num = []
    line_fmt_issue = 0
    src_s = {"mnm": [], "lst": ""}
    dep_s = {"mnm": [], "lst": ""}

    def __str__(self):
        return f"dependency_results(src={self.src_s}, dep={self.dep_s}, " + \
               f"insts={len(self.inst_idxs)}, dep_cnt={self.dep_arr_cnt}, " + \
               f"line_fmt_issue={self.line_fmt_issue})"

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
    if mnemonic in ALL_SPECIAL_MNM:
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

@dataclass
class search_args:
    exec_log: str
    src: str
    dep: str
    window: int

def parse_args():
    parser = argparse.ArgumentParser(description="Count register dependencies within a lookahead window from an ISA sim exec log.")
    parser.add_argument("exec_log", help="Path to exec.log")
    parser.add_argument("--src", required=True, help=f"Comma-separated list of source instruction mnemonics (e.g. 'addi,add,auipc') or '{MNM_ANY}' for any")
    parser.add_argument("--dep", required=True, help=f"Comma-separated list of dependent instruction mnemonics (e.g. 'add,bne,sw') or '{MNM_ANY}' for any")
    parser.add_argument("--window", type=int, default=8, help="Lookahead window size (number of subsequent executed instructions to inspect)")
    return parser.parse_args()

def search(args):
    res = dependency_results()
    if args.src == MNM_ANY:
        res.src_s['mnm'] = [m.lower() for m in icfg.ALL_INST]
        res.src_s['lst'] = "any"
    else:
        res.src_s['mnm'] = [m.strip().lower()
                            for m in args.src.split(",") if m.strip()]
        res.src_s['lst'] = ", ".join(sorted(res.src_s['mnm']))

    if args.dep == MNM_ANY:
        res.dep_s['mnm'] = [m.lower() for m in icfg.ALL_INST]
        res.dep_s['lst'] = "any"
    else:
        res.dep_s['mnm'] = [m.strip().lower()
                            for m in args.dep.split(",") if m.strip()]
        res.dep_s['lst'] = ", ".join(sorted(res.dep_s['mnm']))

    window = args.window
    if window < 1:
        raise SystemExit("Window must be >= 1")
    elif window > 32:
        # simply to prevent overly large searches by mistake, increase if needed
        raise SystemExit("Window too large; max 32 supported")

    # dep_arr: index i counts distance i (i==0 unused, 1..window used)
    res.dep_arr_cnt = [0] * (window + 1)

    # dep_lines: index i holds a list of dep line numbers for that distance
    res.dep_line_num = [[] for _ in range(window + 1)]

    # load all lines (need lookahead)
    with open(args.exec_log, "r", encoding="utf-8", errors="replace") as f:
        lines = f.readlines()

    # pre-filter valid instruction line indices
    # to define "next instruction" semantics cleanly
    res.inst_idxs = [i for i, ln in enumerate(lines) if is_valid_line(ln)]

    n = len(res.inst_idxs)
    res.line_fmt_issue = 0
    for pos, li in enumerate(res.inst_idxs):
        line = lines[li]
        mnm = extract_mnemonic(line)

        if mnm is None:
            res.line_fmt_issue += 1
            continue

        if mnm not in res.src_s['mnm']:
            continue

        regs = extract_regs_in_order(line)
        if not regs:
            # No registers parsed, dc about this line
            #res.line_fmt_issue += 1
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
            nx_idx = res.inst_idxs[nx_pos]
            nx_line = lines[nx_idx]
            nx_mnm = extract_mnemonic(nx_line)
            if nx_mnm is None:
                res.line_fmt_issue += 1
                continue

            nx_regs = extract_regs_in_order(nx_line)
            if nx_regs is None:
                res.line_fmt_issue += 1
                continue

            # if this instruction writes the same rd,
            # kill the dependency search for this producer
            if inst_writes_rd(nx_mnm) and nx_regs:
                nx_rd = nx_regs[0]
                if nx_rd == rd:
                    # stop scanning this window
                    break

            # if this is a dependent inst, check sources for a match
            if nx_mnm in res.dep_s['mnm']:
                srcs = dep_rs(nx_mnm, nx_regs)
                if rd in srcs:
                    dist = nx_pos - pos # distance in executed-instruction steps
                    if 1 <= dist <= window:
                        res.dep_arr_cnt[dist] += 1
                        seq = extract_seq_num(nx_line)
                        if seq is not None:
                            res.dep_line_num[dist].append(int(seq))
                    # Only one increment per dep even if both rs1/rs2 == rd

    return res

def main(args):
    win = args.window
    FMT = smarter_eng_formatter()
    res = search(args)
    exec_inst = FMT(len(res.inst_idxs))
    # move forward only if there are dependencies found
    if sum(res.dep_arr_cnt) == 0:
        print("No dependencies found for the given source/dependent mnemonics"+\
              f" for {exec_inst} executed instructions and window {win}.")
        return

    LN = 5
    print(f"Source inst ({len(res.src_s['mnm'])}): '{res.src_s['lst']}'\n" +
          f"Dependent inst ({len(res.dep_s['mnm'])}): '{res.dep_s['lst']}'\n")
    print(f"Dependency counts by distance " +
          f"for '{get_test_title(args.exec_log)}': " +
          f"{exec_inst} executed instructions, " +
          f"window {win}, first {LN} sample line numbers:")

    max_digits = len(str(max(res.dep_arr_cnt))) + 1
    for d in range(1, win + 1):
        perc = (res.dep_arr_cnt[d] / len(res.inst_idxs)) * 100
        print(f"{INDENT}{d:{2}}: {FMT(res.dep_arr_cnt[d]):>{max_digits}}" +
              f" ({perc:5.2f}%)   {res.dep_line_num[d][:LN]}")

    if res.line_fmt_issue:
        print(f"\n(Skipped lines: {res.line_fmt_issue})")

    fig, ax = plt.subplots(figsize=(2.2+win*.5, 4.5))
    r = ax.bar(range(1, win + 1), res.dep_arr_cnt[1:])
    ax.bar_label(r, padding=3, fmt=FMT)
    ax.set_ylim(0, max(res.dep_arr_cnt[1:]) * 1.1)
    ax.set_title(
        f"Register dependency distances for\n'{get_test_title(args.exec_log)}'")
    ax.set_xlabel("Distance (next executed instruction = 1)")
    ax.set_ylabel("Count")
    ax.yaxis.set_major_formatter(FMT_AXIS)
    ax.set_xticks(list(range(1, win + 1)))
    ax.grid(True, axis="y")
    ax.margins(x=0.01)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    args = parse_args()
    main(args)
