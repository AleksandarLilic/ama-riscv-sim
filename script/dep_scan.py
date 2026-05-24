#!/usr/bin/env python3
import argparse
import struct
from dataclasses import dataclass

import matplotlib.pyplot as plt
from matplotlib.ticker import EngFormatter
from run_analysis import icfg, rolling_mean
from utils import INDENT, get_test_title, smarter_eng_formatter

BRANCH_MNM = icfg.INST_T[icfg.k_branch]
STORE_MNM = icfg.INST_T_MEM[icfg.k_mem_s]
FENCE_MNM = icfg.INST_T[icfg.k_fence]
ALL_NO_RD_MNM = set(BRANCH_MNM + STORE_MNM + FENCE_MNM)

RDP_MNM = icfg.INST_T_SIMD_DATA_FMT[icfg.k_simd_widen] + \
    icfg.INST_T_SIMD_ARITH[icfg.k_simd_mul]

DOT_MNM = icfg.INST_T_SIMD_ARITH[icfg.k_simd_dot]

MNM_ANY = "_any_"

RF_ENTRY_FMT = 'BBBBBB' # opc_g_val, opc_b_val, rd, rs1, rs2, rd_val_zero
RF_ENTRY_SIZE = struct.calcsize(RF_ENTRY_FMT)

class dependency_results:
    n_inst = 0
    dep_arr_cnt = []
    dep_arr_cnt_dot_acc = []
    dep_inst_idx = []
    dep_inst_idx_dot_acc = []
    src_s = {"mnm": [], "str": ""}
    dep_s = {"mnm": [], "str": ""}

    def __str__(self):
        return f"dependency_results(src={self.src_s}, dep={self.dep_s}, " + \
               f"insts={self.n_inst}, dep_cnt={self.dep_arr_cnt})"

def read_rf_trace(path: str) -> list:
    with open(path, 'rb') as f:
        data = f.read()
    n = len(data) // RF_ENTRY_SIZE
    return [struct.unpack_from(RF_ENTRY_FMT, data, i * RF_ENTRY_SIZE)
            for i in range(n)]

def decode_mnm(opc_g_val: int, opc_b_val: int) -> str:
    if opc_g_val != icfg.OPC_NONE:
        return icfg.OPC_G_NAMES[opc_g_val]
    return icfg.OPC_B_NAMES[opc_b_val]

def valid_rd(rd):
    """
    instructions with REG_NONE in the trace don't have rd
    rd == 0 is not a dependency
    """
    return rd != icfg.REG_NONE and rd != 0

def inst_writes_rdp(mnemonic: str) -> bool:
    if mnemonic in RDP_MNM:
        return True
    return False

def dep_rs(rs1: int, rs2: int) -> list[int]:
    """
    for dependent side, return source register indices
    dot instructions handle their rd (accumulator = rs3) at the call site
    always ignore x0 and REG_NONE as sources (not true dependencies)
    """
    return [r for r in [rs1, rs2] if valid_rd(r)]

def src_rd(mnemonic: str, rd: int):
    """
    return written register(s) for source instructions
    skips instructions without write and writes to x0
    """
    if not valid_rd(rd):
        return None
    if inst_writes_rdp(mnemonic):
        return [rd, rd + 1] # return both rd and rdp
    return [rd]

@dataclass
class search_args:
    rf_trace: str
    src: str
    dep: str
    window: int

def parse_args():
    parser = argparse.ArgumentParser(description="Count register dependencies within a lookahead window from an ISA sim rf_trace.bin.")
    parser.add_argument("rf_trace", help="Path to rf_trace.bin")
    parser.add_argument("--src", required=True, help=f"Comma-separated list of source instruction mnemonics (e.g. 'addi,add,auipc') or '{MNM_ANY}' for any")
    parser.add_argument("--dep", required=True, help=f"Comma-separated list of dependent instruction mnemonics (e.g. 'add,bne,sw') or '{MNM_ANY}' for any")
    parser.add_argument("--window", type=int, default=8, help="Lookahead window size (number of subsequent executed instructions to inspect)")
    parser.add_argument("--count_zeros", action="store_true", help="Count how many instructions wrote 0x0 to rd")
    return parser.parse_args()

def search(args):
    res = dependency_results()
    if args.src == MNM_ANY:
        res.src_s['mnm'] = [m.lower() for m in icfg.ALL_INST]
        res.src_s['str'] = "any"
    else:
        res.src_s['mnm'] = [m.strip().lower()
                            for m in args.src.split(",") if m.strip()]
        res.src_s['str'] = ", ".join(sorted(res.src_s['mnm']))
        unknown = [m for m in res.src_s['mnm'] if m not in icfg.ALL_INST]
        if unknown:
            raise SystemExit(
                f"--src: unknown instruction(s): {', '.join(unknown)}")

    if args.dep == MNM_ANY:
        res.dep_s['mnm'] = [m.lower() for m in icfg.ALL_INST]
        res.dep_s['str'] = "any"
    else:
        res.dep_s['mnm'] = [m.strip().lower()
                            for m in args.dep.split(",") if m.strip()]
        res.dep_s['str'] = ", ".join(sorted(res.dep_s['mnm']))
        unknown = [m for m in res.dep_s['mnm'] if m not in icfg.ALL_INST]
        if unknown:
            raise SystemExit(
                f"--dep: unknown instruction(s): {', '.join(unknown)}")

    window = args.window
    if window < 1:
        raise SystemExit("Window must be >= 1")
    elif window > 32:
        # simply to prevent overly large searches by mistake, increase if needed
        raise SystemExit("Window too large; max 32 supported")

    # dep_arr: index i counts distance i (i==0 unused, 1..window used)
    res.dep_arr_cnt = [0] * (window + 1)

    # dep_inst_idx: index i holds a list of trace indices for that distance
    res.dep_inst_idx = [[] for _ in range(window + 1)]

    # dot acc: subset where dep is dot->dot with only rs3 (accumulator) matching
    res.dep_arr_cnt_dot_acc = [0] * (window + 1)
    res.dep_inst_idx_dot_acc = [[] for _ in range(window + 1)]

    entries = read_rf_trace(args.rf_trace)
    n = len(entries)
    res.n_inst = n

    rd_val_zero_cnt = 0
    #rd_val_zero_trace = []
    for pos in range(n):
        e_og, e_ob, e_rd, _, _, e_rdz = entries[pos]
        mnm = decode_mnm(e_og, e_ob)
        rd_val_zero_cnt += e_rdz
        #rd_val_zero_trace.append(e_rdz)

        if mnm not in res.src_s['mnm']:
            continue

        rd_l = src_rd(mnm, e_rd) # source reg destinations list
        if rd_l is None:
            # either non-writer or x0 -> skip as a source
            continue

        # scan lookahead window over subsequent executed instructions
        # terminate early if a redefinition of rd occurs (kills dependency)
        max_pos = min(pos + window, n - 1)
        for nx_pos in range(pos + 1, max_pos + 1):
            nxe_og, nxe_ob, nxe_rd, nxe_rs1, nxe_rs2, _ = entries[nx_pos]
            nx_mnm = decode_mnm(nxe_og, nxe_ob)

            # check dep before kill:
            # same-reg read+write counts as dependency, then ends the chain
            # e.g. `add x1, x1, x2` checks for dep on x1, then kills it
            if nx_mnm in res.dep_s['mnm']:

                nx_rs_l = dep_rs(nxe_rs1, nxe_rs2) # dependent reg sources list
                if nx_mnm in DOT_MNM and valid_rd(nxe_rd):
                    nx_rs_l = [nxe_rd] + nx_rs_l # dot: rd is also rs3/acc

                if any((r in rd_l) for r in nx_rs_l):
                    dist = nx_pos - pos # distance in executed-instruction steps
                    res.dep_arr_cnt[dist] += 1
                    res.dep_inst_idx[dist].append(nx_pos + 1)

                    is_dot_acc = (
                        mnm in DOT_MNM and # src is dot
                        nx_mnm in DOT_MNM and # dep is dot
                        nxe_rd in rd_l and # dep rd is src rd (dot acc)
                        # and no other src regs (rs1/2) in dep
                        not any(
                            (r in rd_l)
                            for r in [nxe_rs1, nxe_rs2] if valid_rd(r)
                        )
                    )
                    if is_dot_acc:
                        # only one increment per dep even if both rs1,rs2 == rd
                        res.dep_arr_cnt_dot_acc[dist] += 1
                        res.dep_inst_idx_dot_acc[dist].append(nx_pos + 1)

            # if this instruction writes the same rd,
            # kill the dependency search for this source
            if valid_rd(nxe_rd) and nxe_rd in rd_l:
                break

    #import pandas as pd
    #fig, ax = plt.subplots()
    #ax.plot(rolling_mean(pd.Series(rd_val_zero_trace), 128))
    return res, rd_val_zero_cnt

def main(args):
    win = args.window
    FMT = smarter_eng_formatter()
    res, zcnt = search(args)
    exec_inst = res.n_inst
    if zcnt and args.count_zeros:
        perc = (zcnt / exec_inst) * 100
        print(f"Note: {zcnt} instructions had 0x0 written to rd ({perc:.2f}%)")
    # move forward only if there are dependencies found
    if sum(res.dep_arr_cnt) == 0:
        print("No dependencies found for the given source/dependent mnemonics "
              f"(source: '{args.src}', dependent '{args.dep}')"
              f" for {FMT(exec_inst)} executed instructions and window {win}.")
        return

    LN = 5
    print(f"Source inst ({len(res.src_s['mnm'])}): '{res.src_s['str']}'\n" +
          f"Dependent inst ({len(res.dep_s['mnm'])}): '{res.dep_s['str']}'\n")
    print(f"Dependency counts by distance " +
          f"for '{get_test_title(args.rf_trace)}': " +
          f"{FMT(exec_inst)} executed instructions, " +
          f"window {win}, first {LN} sample instruction indices:")

    max_digits = len(str(max(res.dep_arr_cnt))) + 1
    for d in range(1, win + 1):
        perc = (res.dep_arr_cnt[d] / exec_inst) * 100
        da = res.dep_arr_cnt_dot_acc[d]
        da_s = f"  dot_acc: {FMT(da)}" if da else ""
        print(f"{INDENT}{d:{2}}: {FMT(res.dep_arr_cnt[d]):>{max_digits}}" +
              f" ({perc:5.2f}%){da_s}   {res.dep_inst_idx[d][:LN]}")

    total_dot_acc = sum(res.dep_arr_cnt_dot_acc)
    if total_dot_acc:
        total_dep = sum(res.dep_arr_cnt)
        print(f"\nOf {FMT(total_dep)} total deps, " +
              f"{FMT(total_dot_acc)} are dot acc (rd -> rs3 only)")

    _, ax = plt.subplots(figsize=(2.2 + win * 0.5, 4.5))
    x = range(1, win + 1)
    has_dot_acc = sum(res.dep_arr_cnt_dot_acc) > 0
    if has_dot_acc:
        other = [res.dep_arr_cnt[d] - res.dep_arr_cnt_dot_acc[d]
                 for d in x]
        dot_acc = res.dep_arr_cnt_dot_acc[1:]
        r1 = ax.bar(x, other)
        r2 = ax.bar(x, dot_acc, bottom=other, label="dot acc",
                    color=icfg.HL_COLORS_OPS[icfg.k_simd_arith])
        ax.bar_label(r2, labels=[FMT(res.dep_arr_cnt[d]) for d in x], padding=3)
        ax.legend()
    else:
        r = ax.bar(x, res.dep_arr_cnt[1:])
        ax.bar_label(r, padding=3, fmt=FMT)

    ax.set_ylim(0, max(res.dep_arr_cnt[1:]) * 1.1)
    ax.set_title(
        f"Register dependency distances for\n'{get_test_title(args.rf_trace)}'")
    ax.set_xlabel("Distance (next executed instruction = 1)")
    ax.set_ylabel("Count")
    ax.yaxis.set_major_formatter(EngFormatter(unit='', places=1, sep=''))
    ax.set_xticks(list(range(1, win + 1)))
    ax.grid(True, axis="y")
    ax.margins(x=0.01)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    args = parse_args()
    main(args)
