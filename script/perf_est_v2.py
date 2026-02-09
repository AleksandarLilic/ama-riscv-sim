#!/usr/bin/env python3
import argparse
import json
import os

import numpy as np
import pandas as pd
from dep_scan import search, search_args
from ruamel.yaml import YAML
from run_analysis import icfg, load_inst_prof
from utils import DELIM, INDENT, smarter_eng_formatter

yaml = YAML()
yaml.preserve_quotes = True

# main mem configuration assumptions based on the port contention from hwpm

# | rd  | wr  | mem cfg |
# | --- | --- | ------- |
# | 0   | 0   | 2R 1W   | - no contention, no arbitration
# | >0  | 0   | 1R 1W   | - IMEM and DMEM contend only for read port
# | 0   | >0  | 1R 1RW  | - DMEM read and write ports contend on dc writeback
# | >0  | >0  | 1RW     | - as above, and contention with IMEM read

class perf:
    b_inst_a = icfg.INST_T[icfg.k_branch]
    jd_inst_a = icfg.INST_T_JUMP[icfg.k_jump_direct]
    ji_inst_a = icfg.INST_T_JUMP[icfg.k_jump_indirect]
    ld_inst_a = icfg.INST_T_MEM[icfg.k_mem_l]
    st_inst_a = icfg.INST_T_MEM[icfg.k_mem_s]
    scp_inst_a = icfg.INST_T[icfg.k_mem_hint]
    mul_inst_a = icfg.INST_T[icfg.k_mul]
    div_inst_a = icfg.INST_T[icfg.k_div]
    dot_inst_a = icfg.INST_T_SIMD_ARITH[icfg.k_simd_dot]
    csr_inst_a = icfg.INST_T[icfg.k_csr]
    expected_hw_metrics = [
        "cpu_frequency_mhz", "pipeline",
        # bp and caches
        "bp_hit", "bp_miss",
        "icache_hit", "icache_miss",
        "dcache_hit", "dcache_miss", "dcache_writeback",
        # main memory access
        #"mem_rd_port_contention", "mem_wr_port_contention",
        # pipeline latencies
        "jump_direct", "jump_indirect",
        "mul", "div", "dot",
        "dcache_load", #"dcache_store",
        # names
        "icache_name", "dcache_name", "bpred_name"
    ]

    def __init__(self, inst_profile, hw_stats, hw_perf_metrics, exec_log=None):
        self.inst_profile = inst_profile
        self.name = inst_profile
        df = load_inst_prof(inst_profile, allow_internal=True)
        # get internal keys into dfi and remove from df
        dfi = df.loc[df['name'].str.startswith('_')]
        df = df.loc[df['name'].str.startswith('_') == False]
        self.sp_usage = dfi[dfi['name'] == "_max_sp_usage"]['count'].tolist()[0]

        self.b = {"taken": 0, "taken_fwd": 0, "taken_bwd": 0,
                  "not_taken": 0, "not_taken_fwd": 0, "not_taken_bwd": 0}
        self.est = {}

        with open(inst_profile, 'r') as file:
            i_prof = json.load(file)
        for b in self.b_inst_a:
            if b in i_prof:
                self._log_branches(i_prof[b])

        with open(hw_stats, 'r') as file:
            self.hw_stats = json.load(file)

        with open(hw_perf_metrics, 'r') as file:
            hwpm = yaml.load(file)

        # check if all expected metrics are present
        for metric in self.expected_hw_metrics:
            if metric not in hwpm:
                raise ValueError(
                    f"Missing metric '{metric}' in " +
                    "HW performance metrics YAML file")
            if "contention" in metric and hwpm[metric] > 1:
                raise ValueError(
                    f"Contention '{metric}' can't be above 100% but was " + \
                    f"specified as '{hwpm[metric]*100}%'")

        # class can be printed, save all stats as member variables
        self.c_pipeline = hwpm['pipeline']

        self.c_jd_res = hwpm['jump_direct']
        self.c_ji_res = hwpm['jump_indirect']
        self.c_bp_miss = hwpm['bp_miss']
        self.c_bp_hit = hwpm["bp_hit"]
        self.c_ic_hit = hwpm['icache_hit']
        self.c_ic_miss = hwpm['icache_miss']
        self.c_dc_hit = hwpm['dcache_hit']
        self.c_dc_miss = hwpm['dcache_miss']
        self.c_dc_wb = hwpm['dcache_writeback']

        #self.mrpc = hwpm["mem_rd_port_contention"]
        #self.mwpc = hwpm["mem_wr_port_contention"]

        self.c_mul = hwpm['mul']
        self.c_div = hwpm['div']
        self.c_dot = hwpm['dot']
        self.c_dc_load = hwpm['dcache_load']
        #self.c_dc_store = hwpm['dcache_store']

        self.ic_name = hwpm['icache_name']
        self.dc_name = hwpm['dcache_name']
        self.bp_name = hwpm['bpred_name']

        sd = lambda d: sum(d.values())

        hw_bp = self.hw_stats[self.bp_name]
        self.bp_stats = {
            "pred": hw_bp["predicted"],
            "mispred": hw_bp["mispredicted"],
            "acc": hw_bp["accuracy"],
            "mpki": hw_bp["mpki"],
            "type": hw_bp["type"]
        }
        hw_ic = self.hw_stats[self.ic_name]
        hw_dc = self.hw_stats[self.dc_name]
        self.ic_stats = {
            "references": hw_ic["references"],
            "hits": hw_ic["hits"]["reads"],
            "misses": hw_ic["misses"]["reads"],
            "hit_rate": (hw_ic["hits"]["reads"] / hw_ic["references"]) * 100,
            "sets": hw_ic["size"]["sets"],
            "ways": hw_ic["size"]["ways"],
            "data": hw_ic["size"]["data"]
        }
        self.dc_stats = {
            "references": hw_dc["references"],
            "hits": sd(hw_dc["hits"]),
            "misses": sd(hw_dc["misses"]),
            "writebacks": hw_dc["writebacks"],
            "hit_rate": (sd(hw_dc["hits"]) / hw_dc["references"]) * 100,
            "sets": hw_dc["size"]["sets"],
            "ways": hw_dc["size"]["ways"],
            "data": hw_dc["size"]["data"]
        }

        self.freq = hwpm['cpu_frequency_mhz']
        self.period = 1 / self.freq

        sum_up = lambda arr: df.loc[df['name'].isin(arr)]['count'].sum()
        self.b_inst = sum_up(self.b_inst_a)
        self.jd_inst = sum_up(self.jd_inst_a)
        self.ji_inst = sum_up(self.ji_inst_a)
        self.ld_inst = sum_up(self.ld_inst_a)
        self.st_inst = sum_up(self.st_inst_a)
        self.scp_inst = sum_up(self.scp_inst_a)
        self.dc_inst = self.ld_inst + self.st_inst + self.scp_inst
        self.mul_inst = sum_up(self.mul_inst_a)
        self.div_inst = sum_up(self.div_inst_a)
        self.dot_inst = sum_up(self.dot_inst_a)

        self.inst_total = df['count'].sum()
        # ipc = 1 -> best case, at least this many cycles needed
        self.ipc_1_cycles = self.c_pipeline + self.inst_total

        ### extra cycles for insts that might stall the pipeline
        ### 'stalls' are non-negotiable - they will happen, but can overlap
        ### 'hazards' may (will stall) or may not happen (won't stall)

        self.b_stalls = (self.c_bp_hit - 1) * self.bp_stats['pred']
        self.b_stalls += (self.c_bp_miss - 1) * self.bp_stats['mispred']

        self.j_stalls = (self.c_jd_res - 1) * self.jd_inst
        self.j_stalls += (self.c_ji_res - 1) * self.ji_inst

        self.ic_stalls = (self.c_ic_hit - 1) * hw_ic["hits"]["reads"]
        self.ic_stalls += self.c_ic_miss * hw_ic["misses"]["reads"]

        # dc miss incurs c_dc_miss clk always, like ic
        # if dc can't handle read and write to the same cache line at once
        # or main mem has only 1 R/W port to dc
        # writeback incurs c_dc_wb clk to first write evicted cache line
        self.dc_stalls = (self.c_dc_hit - 1) * sd(hw_dc["hits"])
        self.dc_stalls += self.c_dc_miss * sd(hw_dc["misses"])
        self.dc_stalls += self.c_dc_wb * hw_dc["writebacks"]

        use_dep_analysis = (exec_log is not None)
        def find_hazards(src, win):
            r, _ = search(search_args(
                exec_log, src=src, dep="_any_", window=win)
            )
            #print(r)
            #print(r.dep_arr_cnt)
            #print(r.line_fmt_issue)
            return sum(r.dep_arr_cnt)

        self.hazards = {"dcache": 0, "mul": 0, "div": 0, "dot": 0, }
        hazard_penalty = {
            "dcache": (self.c_dc_load - 1),
            "mul": (self.c_mul - 1),
            "div": (self.c_div - 1),
            "dot": (self.c_dot - 1),
        }
        fmt = lambda x: ','.join(x)
        # hazards occur when a 2+ clk inst is followed up by an instruction
        # that uses rd of that instruction as its rs1/2
        if (use_dep_analysis):
            self.hazards["dcache"] = find_hazards(
                fmt(icfg.INST_T_MEM[icfg.k_mem_l]), hazard_penalty['dcache'])
            self.hazards["mul"] = find_hazards(
                fmt(icfg.INST_T[icfg.k_mul]), hazard_penalty['mul'])
            self.hazards["div"] = find_hazards(
                fmt(icfg.INST_T[icfg.k_div]), hazard_penalty['div'])
            self.hazards["dot"] = find_hazards(
                fmt(icfg.INST_T_SIMD_ARITH[icfg.k_simd_dot]),
                hazard_penalty['dot']
            )

        else: # otherwise, estimate based on instruction count (pessimistic)
            self.hazards["dcache"] = \
                (hw_dc["hits"]["reads"] + hw_dc["misses"]["reads"])
            self.hazards["dcache"] *= hazard_penalty['dcache']
            self.hazards["mul"] = (self.c_mul - 1) * self.mul_inst
            self.hazards["div"] = (self.c_div - 1) * self.div_inst
            self.hazards["dot"] = (self.c_dot - 1) * self.dot_inst

        self.all_hazards = sum(self.hazards.values())

        # FIXME: this needs to be reworked, heavily dependent of main mem ports
        # memory read/write port contention stalls
        #self.mrpc_stalls = 0
        #self.mwpc_stalls = 0
        #if self.mrpc > 0:
        #    count_num = min(hw_ic["misses"]["reads"], hw_dc["misses"]["reads"])
        #    self.mrpc_stalls = int(count_num * self.c_ic_miss * self.mrpc)

        #if self.mwpc > 0:
        #    count_num = min(hw_dc["misses"]["reads"], hw_dc["misses"]["writes"])
        #    self.mwpc_stalls = int(count_num * self.c_dc_miss * self.mwpc)

        self.fe_stalls = self.ic_stalls + self.j_stalls
        self.be_stalls = self.dc_stalls + self.all_hazards

        self._est_stalls()

        self.branches_perc = round((self.b_inst / self.inst_total) * 100, 2)
        self.ls_perc = round((self.dc_inst / self.inst_total) * 100, 2)
        self.t_clk_wc = int(np.ceil(self.t_clk_wc))
        self.t_clk_bc = int(np.ceil(self.t_clk_bc))

        self.perf_str = f"\nEstimated HW performance at {self.freq}MHz:"
        self.perf_str += f"{DELIM}Best:  "
        self.perf_str += self._estimated_perf(self.t_clk_bc, "best")
        self.perf_str += f"{DELIM}Worst: "
        self.perf_str += self._estimated_perf(self.t_clk_wc, "worst")

        width = self.est["worst"]["clk"] - self.est["best"]["clk"]
        midpoint = (self.est["best"]["clk"] + self.est["worst"]["clk"]) >> 1
        est_ratio = width / midpoint
        self.perf_str += (
            f"{DELIM}Estimated Cycles range: {FMT(width)} cycles, " +
            f"midpoint: {FMT(int(midpoint))}, " +
            f"ratio: {est_ratio*100:.2f}%"
        )

    # TODO:
    # D$ and COMP stalls can technically overlap, but unlikely in current uarch
    def _est_stalls(self):
        # worst case:
        #   fe stalls never overlap with be stalls
        #   alu hazard stall on each multi cycle instruction
        #   dcache hazard stall on each load instruction
        self.t_clk_wc = self.ipc_1_cycles
        self.t_clk_wc += self.b_stalls
        self.t_clk_wc += self.fe_stalls
        self.t_clk_wc += self.be_stalls
        #self.t_clk_wc += self.mrpc_stalls + self.mwpc_stalls

        # best case:
        #   fe stalls overlap with be stalls completely
        #   no alu hazard stall (e.g. best possible inst scheduling)
        #   no dcache hazard stall (quite unlikely on some workloads)
        self.t_clk_bc = self.ipc_1_cycles
        self.t_clk_bc += self.b_stalls
        self.t_clk_bc += max(self.be_stalls, self.fe_stalls)
        #self.t_clk_bc += self.mrpc_stalls + self.mwpc_stalls

    def _log_branches(self, entry):
        for key in entry['breakdown']:
            self.b[key] += entry['breakdown'][key]

    def _estimated_perf(self, cycles, mode):
        cycles = int(np.ceil(cycles))
        cpi = cycles / self.inst_total
        exec_time_us = cycles * self.period
        exec_time_s = exec_time_us / 1_000_000
        mips = self.inst_total / exec_time_us
        self.est[mode] = {}
        self.est[mode]["cpi"] = round(cpi, 3)
        self.est[mode]["ipc"] = round(1/cpi, 3)
        self.est[mode]["clk"] = int(cycles)
        self.est[mode]["exec_time_us"] = round(exec_time_us, 2)
        self.est[mode]["mips"] = round(mips, 2)
        out = f"Cycles: {FMT(cycles)}, CPI: {cpi:.3f} (IPC: {1/cpi:.3f}), " + \
              f"Time: {FMT_T(exec_time_s)}, MIPS: {mips:.1f}"
        return out

    def _cache_stats_str(self, name, stats):
        out = f"{name} " + \
              f"({stats['sets']} sets, {stats['ways']} ways, " + \
              f"{stats['data']}B data): " + \
              f"References: {FMT(stats['references'])}, " + \
              f"Hits: {FMT(stats['hits'])}, " + \
              f"Misses: {FMT(stats['misses'])}, "
        if "writebacks" in stats and stats['writebacks'] > 0:
            out += f"Writebacks: {FMT(stats['writebacks'])}, "
        out += f"Hit Rate: {stats['hit_rate']:.2f}%"
        return out

    def __str__(self):
        out_sp = f"Peak Stack usage: {self.sp_usage} bytes"
        out_ic = []
        out_dc = []
        out_b = []

        out_ic.append(f"Instructions executed: {FMT(self.inst_total)}")
        out_ic.append(self._cache_stats_str(self.ic_name, self.ic_stats))

        out_dc.append(
            f"DMEM inst: {FMT(self.dc_inst)} - "
            f"L/S: {FMT(self.ld_inst)}/{FMT(self.st_inst)} "
            f"({self.ls_perc:.2f}% instructions)"
            )
        out_dc.append(self._cache_stats_str(self.dc_name, self.dc_stats))

        out_b.append(
            f"Branches: {self.b_inst} "
            f"({self.branches_perc:.2f}% instructions)"
            )
        #out_b.append(
        #    f"Taken: {self.b['taken']}, "
        #    f"Forwards: {self.b['taken_fwd']}, "
        #    f"Backwards: {self.b['taken_bwd']}"
        #    )
        #out_b.append(
        #    f"Not taken: {self.b['not_taken']}, "
        #    f"Forwards: {self.b['not_taken_fwd']}, "
        #    f"Backwards: {self.b['not_taken_bwd']}"
        #    )
        out_b.append(
            f"Branch Predictor ({self.bp_stats['type']}): "
            f"Predicted: {FMT(self.bp_stats['pred'])}, "
            f"Mispredicted: {FMT(self.bp_stats['mispred'])}, "
            f"Accuracy: {self.bp_stats['acc']:.2f}%, "
            f"MPKI: {self.bp_stats['mpki']}"
            )

        out_stalls = f"Pipeline stalls (max): " + \
            f"{DELIM}Bad spec: {FMT(self.b_stalls)}" + \
            f"{DELIM}FE bound: {FMT(self.fe_stalls)} - " + \
            f"ICache: {FMT(self.ic_stalls)}, " + \
            f"Core: {FMT(self.j_stalls)}" + \
            f"{DELIM}BE bound: {FMT(self.be_stalls)} - " + \
            f"DCache: {FMT(self.dc_stalls)}, " + \
            f"Core: {FMT(self.all_hazards)}" #+ \
            #f"{DELIM}Memory contention: " + \
            #    f"{FMT(self.mrpc_stalls + self.mwpc_stalls)} "

        stats = f"{self.name}" + \
                f"\n{out_sp}" + \
                f"\n{DELIM.join(out_ic)}" + \
                f"\n{DELIM.join(out_dc)}" + \
                f"\n{DELIM.join(out_b)}" + \
                f"\n{out_stalls}"

        return f"{stats}\n{self.perf_str}"

    def save_as_df(self) -> None:
        attrs = vars(self).copy()
        branches = attrs.pop('b')
        est = attrs.pop('est')
        _ = attrs.pop('inst_profile')
        _ = attrs.pop('perf_str')
        all_flat = {**attrs, **branches, **est}
        df = pd.DataFrame([all_flat])
        df.to_csv(self.inst_profile.replace(".json", "_perf_est.csv"),
                  index=False)
        print(f"\nSaved performance estimation as CSV to " +
              f"{self.inst_profile.replace('.json', '_perf_est.csv')}")

def parse_args():
    parser = argparse.ArgumentParser(description="Count register dependencies within a lookahead window from an ISA sim exec log.")
    parser.add_argument("inst_profile", help="Path to 'inst_profile.json' for the given workload")
    parser.add_argument("hw_stats", help="Path to 'hw_stats.json' for the given workload")
    parser.add_argument("hw_perf_metrics", help="Path to 'hw_perf_metrics.yaml' for the given hardware configuration")
    parser.add_argument("-e", "--exec_log", help="Optional argument to provide 'exec.log' path for depndency/hazard analysis", default=None)
    parser.add_argument("-c", "--corr", type=int, default=0, help="If specified, run cycles correlation analysis with the given achieved cycles value")
    parser.add_argument("-p", "--places", type=int, default=1, help="Number of decimal places for formatted output (default: 1)")
    return parser.parse_args()

def main(args: argparse.Namespace):
    p_inst = args.inst_profile
    p_hws = args.hw_stats
    p_met = args.hw_perf_metrics
    p_exec = args.exec_log
    corr = args.corr
    paths = [p_inst, p_hws, p_met]
    if p_exec:
        paths.append(args.exec_log)

    for p in paths:
        if not os.path.isfile(p):
            raise ValueError(f"File {p} not found")

    res = perf(p_inst, p_hws, p_met, p_exec)
    print(res)

    if corr:
        import matplotlib.pyplot as plt
        print("\nCycles Correlation")
        clk_best = res.est["best"]["clk"]
        clk_worst = res.est["worst"]["clk"]
        diff = None
        if corr < clk_best:
            inout_str = "OUTSIDE (BELOW)"
            diff = clk_best - corr
            perc_diff = (diff / clk_best) * 100
            ref_str = "best"
        elif corr > clk_worst:
            inout_str = "OUTSIDE (ABOVE)"
            diff = corr - clk_worst
            perc_diff = (diff / clk_worst) * 100
            ref_str = "worst"
        else:
            inout_str = "INSIDE"
        print(f"{INDENT}Achieved cycles: {FMT(corr)} - " +
              f"result is {inout_str} estimated range")

        if diff is not None:
            print(f"{INDENT}Difference: {diff} cycles " +
                  f"({perc_diff:.2f}% of {ref_str} estimate)")

        # --- Plot ---
        fig, ax = plt.subplots(figsize=(8, 2))
        xmin = min(clk_best, corr)
        xmax = max(clk_worst, corr)
        xrange = xmax - xmin
        ax.set_xlim(xmin - xrange*.1, xmax + xrange*.1)

        limx = ax.get_xlim()
        # range bar
        ax.hlines(0, limx[0], limx[1],
                  color='lightgray', linewidth=15, label='Estimate range')

        # add markers
        ax.vlines(clk_worst, -0.1, 0.1,
                  color='tab:red', linewidth=2, label='Worst')
        ax.vlines(clk_best, -0.1, 0.1,
                  color='tab:green', linewidth=2, label='Best')
        ax.vlines(corr, -0.15, 0.15,
                  color='tab:blue', linewidth=2, label='Achieved')

        # annotate
        ax.text(clk_worst, 0.13, f'Worst:\n{FMT(clk_worst)}',
                ha='center', color='tab:red')
        ax.text(clk_best, 0.13, f'Best:\n{FMT(clk_best)}',
                ha='center', color='tab:green')
        ax.text(corr, -0.37, f'Achieved:\n{FMT(corr)}',
                ha='center', color='tab:blue')

        # style
        ax.margins(0,0)
        ax.set_ylim(-0.42, 0.42)
        ax.set_yticks([])
        ax.grid(axis='x', linestyle='--', alpha=0.5)
        ax.set_xlabel('Cycles')
        ax.xaxis.set_major_formatter(FMT)
        #ax.legend(loc='upper right', bbox_to_anchor=(1.35, 1))
        ax.set_title('Estimate vs Achieved Cycles')

        plt.tight_layout()
        plt.show()

if __name__ == "__main__":
    args = parse_args()
    FMT = smarter_eng_formatter(unit='', places=args.places, sep="")
    FMT_T = smarter_eng_formatter(unit='s', places=args.places, sep="")
    main(args)
