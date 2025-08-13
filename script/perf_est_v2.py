#!/usr/bin/env python3

import json
import os
import sys

import numpy as np
import pandas as pd
from run_analysis import json_prof_to_df

DELIM = "\n    "
R={"fe_overlap": 0.4, "fe_range": 0.2, "hazard_freq": 0.5, "hazard_range": 0.2}

# main mem configuration assumptions based on the port contention from hwpm

# | rd  | wr  | mem cfg |
# | --- | --- | ------- |
# | 0   | 0   | 2R 1W   | - no contention
# | >0  | 0   | 1R 1W   | - IMEM and DMEM contend only for read port
# | 0   | >0  | 1R 1RW  | - DMEM read and write ports contend within a few clks
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
        "bpred", "icache", "dcache", "rd_port_contention", "wr_port_contention",
        "mem", "mul", "div", "dot",
        "icache_name", "dcache_name", "bpred_name"]

    def __init__(self, inst_profiler_path, hw_stats_path, hw_perf_metrics_path,
                 realistic=False):
        self.inst_profiler_path = inst_profiler_path
        self.name = inst_profiler_path
        self.realistic = realistic
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
        self.rdc = hwpm["rd_port_contention"]
        self.wrc = hwpm["wr_port_contention"]
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
        self.dc_stalls = (self.c_dc - 1) * sd(hw_dc["hits"])
        self.dc_stalls += self.c_mem * sd(hw_dc["misses"])

        self.rdc_stalls = 0
        self.wrc_stalls = 0
        if self.rdc > 0:
            occurences = min(hw_ic["misses"]["reads"], hw_dc["misses"]["reads"])
            self.rdc_stalls = int(occurences * self.c_mem * self.rdc)

        if self.wrc > 0:
            occurences = min(hw_dc["misses"]["reads"],hw_dc["misses"]["writes"])
            self.wrc_stalls = int(occurences * self.c_mem * self.wrc)

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
        self.perf_str += self._estimated_perf(self.total_cycles_bc)
        self.perf_str += f"{DELIM}Worst: "
        self.perf_str += self._estimated_perf(self.total_cycles_wc)

    # TODO:
    # D$ and alu stalls can technically overlap, but unlikely in current uarch
    def _est_stalls(self):
        # worst case:
        #   fe stalls never overlap with dc stalls or alu hazard stall
        #   alu hazard stall on each multi cycle instruction
        self.total_cycles_wc = self.ipc_1_cycles
        if not realistic:
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
        if not realistic:
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

    def _estimated_perf(self, cycles):
        cycles = int(np.ceil(cycles))
        cpi = cycles / self.inst_total
        exec_time_us = cycles * self.period
        mips = self.inst_total / exec_time_us
        self.est["cpi"] = round(cpi, 2)
        self.est["exec_time_us"] = round(exec_time_us, 2)
        self.est["mips"] = round(mips, 2)
        out = f"Cycles: {cycles}, CPI: {cpi:.2f}, " + \
              f"Time: {exec_time_us:.1f}us, MIPS: {mips:.1f}"
        return out

    def _cache_stats_str(self, name, stats):
        out = f"{name} " + \
              f"({stats['sets']} sets, {stats['ways']} ways, " + \
              f"{stats['data']}B data): " + \
              f"References: {stats['references']}, " + \
              f"Hits: {stats['hits']}, " + \
              f"Misses: {stats['misses']}, "
        if "writebacks" in stats and stats['writebacks'] > 0:
            out += f"Writebacks: {stats['writebacks']}, "
        out += f"Hit Rate: {stats['hit_rate']:.2f}%"
        return out

    def __str__(self):
        out_sp = f"Peak Stack usage: {self.sp_usage} bytes"
        out_ic = []
        out_dc = []
        out_b = []

        out_ic.append(f"Instructions executed: {self.inst_total}")
        out_ic.append(self._cache_stats_str(self.ic_name, self.ic_stats))

        out_dc.append(
            f"DMEM inst: {self.dc_inst} - "
            f"L/S: {self.ld_inst}/{self.st_inst} "
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
            f"Predicted: {self.bp_stats['pred']}, "
            f"Mispredicted: {self.bp_stats['mispred']}, "
            f"Accuracy: {self.bp_stats['acc']:.2f}%, "
            f"MPKI: {self.bp_stats['mpki']}"
            )

        out_stalls = f"Pipeline stalls (max): " + \
            f"FE/BE: {self.fe_stalls}/{self.be_stalls}, " + \
            f"{DELIM}FE: " + \
            f"ICache: {self.ic_stalls}, " + \
            f"Jumps: {self.j_stalls}, " + \
            f"Branches: {self.b_stalls}" + \
            f"{DELIM}BE: " + \
            f"DCache: {self.dc_stalls} " + \
            f"MUL: {self.mul_stalls}, " + \
            f"DIV: {self.div_stalls}, " + \
            f"DOT: {self.dot_stalls}" + \
            f"\nMemory contention: {self.rdc_stalls + self.wrc_stalls} "

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
        predictions = attrs.pop('p')
        est = attrs.pop('est')
        _ = attrs.pop('inst_profiler_path')
        _ = attrs.pop('perf_str')
        all_flat = {**attrs, **branches, **predictions, **est}
        df = pd.DataFrame([all_flat])
        df.to_csv(self.inst_profiler_path.replace(".json", "_perf_est.csv"),
                  index=False)

if __name__ == "__main__":
    inst_profiler_path = sys.argv[1]
    hw_stats_path = sys.argv[2]
    hw_perf_metrics_path = sys.argv[3]

    # use with caution
    realistic = sys.argv[4] == "-r" if len(sys.argv) > 4 else False

    if not os.path.isfile(inst_profiler_path):
        raise ValueError(f"File {inst_profiler_path} not found")
    if not os.path.isfile(hw_stats_path):
        raise ValueError(f"File {hw_stats_path} not found")
    if not os.path.isfile(hw_perf_metrics_path):
        raise ValueError(f"File {hw_perf_metrics_path} not found")

    est = perf(
        inst_profiler_path, hw_stats_path, hw_perf_metrics_path, realistic)
    print(est)
