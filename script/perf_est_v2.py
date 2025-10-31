#!/usr/bin/env python3

import json
import os
import sys

import numpy as np
import pandas as pd
from matplotlib.ticker import EngFormatter, FuncFormatter
from run_analysis import json_prof_to_df
from utils import DELIM, INDENT

R={"fe_overlap": 0.4, "fe_range": 0.2, "hazard_freq": 0.5, "hazard_range": 0.2}

REALISTIC = False # use with caution, set to False by default

def smarter_eng_formatter(places=1, unit='', sep=''):
    base_fmt = EngFormatter(unit=unit, places=places, sep=sep)
    more_fmt = EngFormatter(unit=unit, places=places + 1, sep=sep)

    def _fmt(x, pos):
        s = base_fmt(x, pos)
        try:
            val = float(s.split(".")[0])
        except ValueError:
            return s
        # use extra precision if < 10 after scaling
        s = more_fmt(x, pos) if abs(val) < 10 and val != 0 else s
        # strip trailing .0 or zeros
        return s.rstrip('0').rstrip('.') if '.' in s else s

    return FuncFormatter(_fmt)

# main mem configuration assumptions based on the port contention from hwpm

# | rd  | wr  | mem cfg |
# | --- | --- | ------- |
# | 0   | 0   | 2R 1W   | - no contention, no arbitration
# | >0  | 0   | 1R 1W   | - IMEM and DMEM contend only for read port
# | 0   | >0  | 1R 1RW  | - DMEM read and write ports contend on dc writeback
# | >0  | >0  | 1RW     | - as above, and contention with IMEM read

# write contention is likely to be very small, as it's not easy to occur
# e.g. D$ read miss followed by D$ write miss in the next cycle

class perf:
    # TODO: needs compressed ISA
    b_inst_a = ["beq", "bne", "blt", "bge", "bltu", "bgeu"]
    j_inst_a = ["jalr", "jal"] # split jal vs jalr?
    ld_inst_a = ["lb", "lh", "lw", "lbu", "lhu"]
    st_inst_a = ["sb", "sh", "sw"]
    scp_inst_a = ["scp.ld", "scp.rel"]
    #dc_inst_a = ld_inst_a + st_inst_a + scp_inst_a
    mul_inst_a = ["mul", "mulh", "mulhsu", "mulhu"]
    div_inst_a = ["div", "divu", "rem", "remu"]
    dot_inst_a = ["dot4", "dot8", "dot16"]
    csr_inst_a = ["csrrw", "csrrs", "csrrc", "csrrwi", "csrrsi", "csrrci"]
    expected_hw_metrics = [
        "cpu_frequency_mhz", "pipeline", "branch_resolution", "jump_resolution",
        "bpred", "icache", "dcache",
        "mem_rd_port_contention", "mem_wr_port_contention",
        "mem", "mul", "div", "dot",
        "icache_name", "dcache_name", "bpred_name"]

    def __init__(
            self,
            inst_profiler_path,
            hw_stats_path,
            hw_perf_metrics_path
        ):
        self.inst_profiler_path = inst_profiler_path
        self.name = inst_profiler_path
        self.realistic = REALISTIC
        df = json_prof_to_df(inst_profiler_path, allow_internal=True)
        # get internal keys into dfi and remove from df
        dfi = df.loc[df['name'].str.startswith('_')]
        df = df.loc[df['name'].str.startswith('_') == False]
        self.sp_usage = dfi[dfi['name'] == "_max_sp_usage"]['count'].tolist()[0]

        self.b = {"taken": 0, "taken_fwd": 0, "taken_bwd": 0,
                  "not_taken": 0, "not_taken_fwd": 0, "not_taken_bwd": 0}
        self.est = {}

        with open(inst_profiler_path, 'r') as file:
            i_prof = json.load(file)
        for b in self.b_inst_a:
            self._log_branches(i_prof[b])

        with open(hw_stats_path, 'r') as file:
            self.hw_stats = json.load(file)

        with open(hw_perf_metrics_path, 'r') as file:
            hwpm = json.load(file)

        # check if all expected metrics are present
        for metric in self.expected_hw_metrics:
            if metric not in hwpm:
                raise ValueError(
                    f"Missing metric '{metric}' in " +
                    "HW performance metrics JSON file")
            if "contention" in metric and hwpm[metric] > 1:
                raise ValueError(
                    f"Contention '{metric}' can't be above 100% but was " + \
                    f"specified as '{hwpm[metric]*100}%'")

        # class can be printed, save all stats as member variables
        self.c_pipeline = hwpm['pipeline']
        self.c_branch_res = hwpm['branch_resolution']
        self.c_jump_res = hwpm['jump_resolution']
        self.c_bp = hwpm["bpred"]
        self.c_ic = hwpm['icache']
        self.c_dc = hwpm['dcache']
        self.rdc = hwpm["mem_rd_port_contention"]
        self.wrc = hwpm["mem_wr_port_contention"]
        self.c_mem = hwpm['mem']
        self.c_mul = hwpm['mul']
        self.c_div = hwpm['div']
        self.c_dot = hwpm['dot']
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

        self.inst_total = df['count'].sum()
        self.b_inst = df.loc[df['name'].isin(self.b_inst_a)]['count'].sum()
        self.j_inst = df.loc[df['name'].isin(self.j_inst_a)]['count'].sum()
        self.ld_inst = df.loc[df['name'].isin(self.ld_inst_a)]['count'].sum()
        self.st_inst = df.loc[df['name'].isin(self.st_inst_a)]['count'].sum()
        self.scp_inst = df.loc[df['name'].isin(self.scp_inst_a)]['count'].sum()
        self.dc_inst = self.ld_inst + self.st_inst + self.scp_inst
        self.mul_inst = df.loc[df['name'].isin(self.mul_inst_a)]['count'].sum()
        self.div_inst = df.loc[df['name'].isin(self.div_inst_a)]['count'].sum()
        self.dot_inst = df.loc[df['name'].isin(self.dot_inst_a)]['count'].sum()

        # ipc = 1 -> best case, at least this many cycles needed
        self.ipc_1_cycles = self.c_pipeline + self.inst_total

        # extra cycles for insts that might stall the pipeline
        self.b_stalls = (self.c_bp - 1) * self.bp_stats['pred']
        self.b_stalls += (self.c_branch_res - 1) * self.bp_stats['mispred']
        self.j_stalls = (self.c_jump_res - 1) * self.j_inst
        self.ic_stalls = (self.c_ic - 1) * hw_ic["hits"]["reads"]
        self.ic_stalls += self.c_mem * hw_ic["misses"]["reads"]
        # dc miss incurs c_mem clk always, like ic
        # if dc can't handle read and write to the same cache line at once
        # or main mem has only 1 R/W port to dc
        # writeback incurs another c_mem clk to first write evicted cache line
        self.dc_stalls = (self.c_dc - 1) * sd(hw_dc["hits"])
        self.dc_stalls += self.c_mem * sd(hw_dc["misses"])
        self.dc_stalls += self.c_mem * hw_dc["writebacks"]

        self.rdc_stalls = 0
        self.wrc_stalls = 0
        if self.rdc > 0:
            cont_num = min(hw_ic["misses"]["reads"], hw_dc["misses"]["reads"])
            self.rdc_stalls = int(cont_num * self.c_mem * self.rdc)

        if self.wrc > 0:
            cont_num = min(hw_dc["misses"]["reads"], hw_dc["misses"]["writes"])
            self.wrc_stalls = int(cont_num * self.c_mem * self.wrc)

        # FIXME: a bit of handwaving for a 1RW config by just adding rdc + wrc

        self.mul_stalls = (self.c_mul - 1) * self.mul_inst
        self.div_stalls = (self.c_div - 1) * self.div_inst
        self.dot_stalls = (self.c_dot - 1) * self.dot_inst
        self.alu_stalls = self.mul_stalls + self.div_stalls + self.dot_stalls

        self.fe_stalls = self.b_stalls + self.j_stalls + self.ic_stalls
        self.be_stalls = self.dc_stalls + self.alu_stalls

        self._est_stalls()

        self.branches_perc = round((self.b_inst / self.inst_total) * 100, 2)
        self.ls_perc = round((self.dc_inst / self.inst_total) * 100, 2)
        self.total_cycles_wc = int(np.ceil(self.total_cycles_wc))
        self.total_cycles_bc = int(np.ceil(self.total_cycles_bc))
        self.perf_str = f"Estimated HW performance at {self.freq}MHz:"
        self.perf_str += f"{DELIM}Best:  "
        self.perf_str += self._estimated_perf(self.total_cycles_bc, "best")
        self.perf_str += f"{DELIM}Worst: "
        self.perf_str += self._estimated_perf(self.total_cycles_wc, "worst")

        width = self.est["worst"]["clk"] - self.est["best"]["clk"]
        midpoint = (self.est["best"]["clk"] + self.est["worst"]["clk"]) >> 1
        est_ratio = width / midpoint
        self.perf_str += (
            f"{DELIM}Estimated Cycles range: {FMT(width)} cycles, " +
            f"midpoint: {FMT(int(midpoint))}, " +
            f"ratio: {est_ratio*100:.2f}%"
        )

    # TODO:
    # D$ and alu stalls can technically overlap, but unlikely in current uarch
    def _est_stalls(self):
        # worst case:
        #   fe stalls never overlap with dc stalls or alu hazard stall
        #   alu hazard stall on each multi cycle instruction
        self.total_cycles_wc = self.ipc_1_cycles
        if not self.realistic:
            self.total_cycles_wc += \
                self.fe_stalls + self.dc_stalls + self.alu_stalls
        else:
            alu_h = self.alu_stalls * (R["hazard_freq"] + R["hazard_range"])
            # min since there has to be enough cycles in be to overlap fe with
            fe_s = min(self.fe_stalls, self.dc_stalls + alu_h)
            fe_o = fe_s * (R["fe_overlap"] - R["fe_range"])
            self.total_cycles_wc += self.fe_stalls - fe_o # reduced by overlap
            self.total_cycles_wc += self.dc_stalls + alu_h

        self.total_cycles_wc += self.rdc_stalls + self.wrc_stalls

        # best case:
        #   fe stalls overlap with dc stalls completely
        #   no alu hazard stall (e.g. best possible inst scheduling)
        self.total_cycles_bc = self.ipc_1_cycles
        if not self.realistic:
            self.total_cycles_bc += max(self.dc_stalls, self.fe_stalls)
        else:
            alu_h = self.alu_stalls * (R["hazard_freq"] - R["hazard_range"])
            fe_s = min(self.fe_stalls, self.dc_stalls + alu_h)
            fe_o = fe_s * (R["fe_overlap"] + R["fe_range"])
            self.total_cycles_bc += self.fe_stalls - fe_o
            self.total_cycles_bc += self.dc_stalls + alu_h

        self.total_cycles_bc += self.rdc_stalls + self.wrc_stalls

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
            f"FE/BE: {FMT(self.fe_stalls)}/{FMT(self.be_stalls)}, " + \
            f"{DELIM}FE: " + \
            f"ICache: {FMT(self.ic_stalls)}, " + \
            f"Jumps: {FMT(self.j_stalls)}, " + \
            f"Branches: {FMT(self.b_stalls)}" + \
            f"{DELIM}BE: " + \
            f"DCache: {FMT(self.dc_stalls)}, " + \
            f"MUL: {FMT(self.mul_stalls)}, " + \
            f"DIV: {FMT(self.div_stalls)}, " + \
            f"DOT: {FMT(self.dot_stalls)}" + \
            f"\nMemory contention: {FMT(self.rdc_stalls + self.wrc_stalls)} "

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
        _ = attrs.pop('inst_profiler_path')
        _ = attrs.pop('perf_str')
        all_flat = {**attrs, **branches, **est}
        df = pd.DataFrame([all_flat])
        df.to_csv(self.inst_profiler_path.replace(".json", "_perf_est.csv"),
                  index=False)
        print(f"\nSaved performance estimation as CSV to " +
              f"{self.inst_profiler_path.replace('.json', '_perf_est.csv')}")

if __name__ == "__main__":
    inst_profiler_path = sys.argv[1]
    hw_stats_path = sys.argv[2]
    hw_perf_metrics_path = sys.argv[3]
    corr = int(sys.argv[4]) if len(sys.argv) > 4 else 0 # run correlation
    places = int(sys.argv[5]) if len(sys.argv) > 5 else 1

    FMT = smarter_eng_formatter(unit='', places=places, sep="")
    FMT_T = smarter_eng_formatter(unit='s', places=places, sep="")

    if not os.path.isfile(inst_profiler_path):
        raise ValueError(f"File {inst_profiler_path} not found")
    if not os.path.isfile(hw_stats_path):
        raise ValueError(f"File {hw_stats_path} not found")
    if not os.path.isfile(hw_perf_metrics_path):
        raise ValueError(f"File {hw_perf_metrics_path} not found")

    res = perf(inst_profiler_path, hw_stats_path, hw_perf_metrics_path)
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
        print(f"{INDENT}Achieved cycles result is {inout_str} estimated range")

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
        ax.set_xlabel('Cycles')
        ax.xaxis.set_major_formatter(FMT)
        #ax.legend(loc='upper right', bbox_to_anchor=(1.35, 1))
        ax.set_title('Estimate vs Achieved Cycles')

        plt.tight_layout()
        plt.show()
