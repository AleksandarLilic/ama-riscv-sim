#!/usr/bin/env python3
import argparse
import numpy as np
from dataclasses import dataclass

import matplotlib.pyplot as plt
from matplotlib.ticker import EngFormatter
from run_analysis import icfg, rolling_mean
from utils import INDENT, get_test_title, smarter_eng_formatter

MNM_ANY = "_any_"
DOT_MNM = set(icfg.INST_T_SIMD_ARITH[icfg.k_simd_dot])
RDP_MNM = set(
    icfg.INST_T_SIMD_DATA_FMT[icfg.k_simd_widen] +
    icfg.INST_T_SIMD_DATA_FMT[icfg.k_simd_txp] +
    icfg.INST_T_SIMD_ARITH[icfg.k_simd_wmul]
)

RF_DTYPE = np.dtype([
    ('opc_g', np.uint8), ('opc_b', np.uint8), ('rd', np.uint8),
    ('rs1', np.uint8), ('rs2', np.uint8), ('rdz', np.uint8), ('rdpz', np.uint8),
])

LUT_SIZE = np.iinfo(np.uint8).max + 1

def build_bool_lut(mnm_set: set):
    """Map each opcode ID -> bool for fast numpy indexed lookup."""
    g = np.zeros(LUT_SIZE, dtype=bool)
    b = np.zeros(LUT_SIZE, dtype=bool)
    for i, name in enumerate(icfg.OPC_G_NAMES):
        g[i] = name in mnm_set
    for i, name in enumerate(icfg.OPC_B_NAMES):
        b[i] = name in mnm_set
    return g, b

# B LUTs are all-zero for DOT/RDP: custom SIMD -> always General opcode
DOT_G_LUT, DOT_B_LUT = build_bool_lut(DOT_MNM)
RDP_G_LUT, RDP_B_LUT = build_bool_lut(RDP_MNM)

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

def read_rf_trace(path: str) -> np.ndarray:
    return np.fromfile(path, dtype=RF_DTYPE)

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
    parser.add_argument('--save_png', action='store_true', help="Save charts as PNG")
    parser.add_argument('--save_svg', action='store_true', help="Save charts as SVG")
    return parser.parse_args()

def search(args):
    res = dependency_results()
    if args.src == MNM_ANY:
        res.src_s['mnm'] = [m.lower() for m in icfg.ALL_INST]
        res.src_s['str'] = "any"
    else:
        res.src_s['mnm'] = [
            m.strip().lower() for m in args.src.split(",") if m.strip()
        ]
        res.src_s['str'] = ", ".join(sorted(res.src_s['mnm']))
        unknown = [m for m in res.src_s['mnm'] if m not in icfg.ALL_INST]
        if unknown:
            raise SystemExit(
                f"--src: unknown instruction(s): {', '.join(unknown)}")

    if args.dep == MNM_ANY:
        res.dep_s['mnm'] = [m.lower() for m in icfg.ALL_INST]
        res.dep_s['str'] = "any"
    else:
        res.dep_s['mnm'] = [
            m.strip().lower() for m in args.dep.split(",") if m.strip()
        ]
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

    opc_g = entries['opc_g']
    opc_b = entries['opc_b']
    use_g = opc_g != icfg.OPC_NONE  # True where entry uses General opcode table

    # element-wise: select G or B LUT result per entry depending on opcode type
    def apply_lut(g_lut, b_lut):
        return np.where(use_g, g_lut[opc_g], b_lut[opc_b])

    is_rdp = apply_lut(RDP_G_LUT, RDP_B_LUT)
    is_dot = apply_lut(DOT_G_LUT, DOT_B_LUT)
    rd_arr = entries['rd']
    is_valid_rd = (rd_arr != icfg.REG_NONE) & (rd_arr != 0)

    rd_val_zero_cnt = {'rd': 0, 'rdp': 0}
    rd_val_zero_cnt['rd'] = int(entries['rdz'][is_valid_rd].sum())
    rd_val_zero_cnt['rdp'] = int(entries['rdpz'][is_valid_rd & is_rdp].sum())

    # query-specific LUTs: built per call, unlike DOT/RDP which are module-level
    src_g_lut, src_b_lut = build_bool_lut(set(res.src_s['mnm']))
    dep_g_lut, dep_b_lut = build_bool_lut(set(res.dep_s['mnm']))
    is_src = apply_lut(src_g_lut, src_b_lut)
    is_dep = apply_lut(dep_g_lut, dep_b_lut)

    # column views and local aliases to avoid per-iteration attribute lookups
    all_rd = rd_arr
    all_rs1 = entries['rs1']
    all_rs2 = entries['rs2']
    REG_NONE = icfg.REG_NONE

    for pos in range(n):
        if not is_src[pos]:
            continue

        e_rd = int(all_rd[pos])
        if not is_valid_rd[pos]:
            continue

        # rdp writes consecutive reg pair
        rd_l = [e_rd, e_rd + 1] if is_rdp[pos] else [e_rd]

        # scan lookahead window over subsequent executed instructions
        # terminate early if a redefinition of rd occurs (kills dependency)
        max_pos = min(pos + window, n - 1)
        for nx_pos in range(pos + 1, max_pos + 1):
            nxe_rd = int(all_rd[nx_pos])
            nxe_rs1 = int(all_rs1[nx_pos])
            nxe_rs2 = int(all_rs2[nx_pos])

            # check dep before kill:
            # same-reg read+write counts as dependency, then ends the chain
            # e.g. `add x1, x1, x2` checks for dep on x1, then kills it
            if is_dep[nx_pos]:
                nx_rs_l = [r for r in (nxe_rs1, nxe_rs2)
                           if r != REG_NONE and r != 0]
                if is_dot[nx_pos] and nxe_rd != REG_NONE and nxe_rd != 0:
                    nx_rs_l = [nxe_rd] + nx_rs_l # dot: rd is also rs3/acc

                if any(r in rd_l for r in nx_rs_l):
                    dist = nx_pos - pos # distance in executed-instruction steps
                    res.dep_arr_cnt[dist] += 1
                    res.dep_inst_idx[dist].append(nx_pos + 1)

                    is_dot_acc = (
                        is_dot[pos] and # src is dot
                        is_dot[nx_pos] and # dep is dot
                        nxe_rd in rd_l and # dep rd is src rd (dot acc)
                        # and no other src regs (rs1/2) in dep
                        not any(r in rd_l for r in (nxe_rs1, nxe_rs2)
                                if r != REG_NONE and r != 0)
                    )
                    if is_dot_acc:
                        # only one increment per dep even if both rs1,rs2 == rd
                        res.dep_arr_cnt_dot_acc[dist] += 1
                        res.dep_inst_idx_dot_acc[dist].append(nx_pos + 1)

            # if this instruction writes the same rd,
            # kill the dependency search for this source
            if nxe_rd != REG_NONE and nxe_rd != 0 and nxe_rd in rd_l:
                break

    #import pandas as pd
    #fig, ax = plt.subplots()
    #ax.plot(rolling_mean(
    #    pd.Series(entries['rdz'][is_valid_rd].astype(float)), 128))
    #ax.plot(rolling_mean(
    #    pd.Series(entries['rdpz'][is_valid_rd & is_rdp].astype(float)), 128))
    return res, rd_val_zero_cnt

def main(args):
    win = args.window
    FMT = smarter_eng_formatter()
    res, zcnt = search(args)
    exec_inst = res.n_inst

    if zcnt and args.count_zeros:
        perc_rd = (zcnt['rd'] / exec_inst) * 100
        perc_rdp = (zcnt['rdp'] / exec_inst) * 100
        print(f"Note: {zcnt['rd']} insts wrote 0x0 to rd ({perc_rd:.2f}%), "
            f"{zcnt['rdp']} wrote 0x0 to rdp ({perc_rdp:.2f}%)")

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

    fig, ax = plt.subplots(figsize=(2.2 + win * 0.5, 4.5))
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

    name = args.rf_trace.replace(" ", "_") \
               .replace("_trace", "_dependency") \
               .replace(".bin", "")
    if args.save_png:
        out = f"{name}.png"
        fig.savefig(out)
        print(f"Saved PNG chart to: '{out}'")
    if args.save_svg:
        out = f"{name}.svg"
        fig.savefig(out)
        print(f"Saved SVG chart to: '{out}'")

if __name__ == "__main__":
    args = parse_args()
    main(args)
