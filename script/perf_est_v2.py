import os
import sys
import json
import pandas as pd
import numpy as np
from run_analysis import json_prof_to_df

class perf:
    # TODO: needs compressed ISA
    b_inst_a = ["beq", "bne", "blt", "bge", "bltu", "bgeu"]
    j_inst_a = ["jalr", "jal"]
    dc_inst_a = ["lb", "lh", "lw", "lbu", "lhu", "sb", "sh", "sw",
                 "scp.ld", "scp.rel"]
    mul_inst_a = ["mul", "mulh", "mulhsu", "mulhu"]
    fma_inst_a = ["fma4", "fma8", "fma16"]
    div_inst_a = ["div", "divu", "rem", "remu"]
    csr_inst_a = ["csrrw", "csrrs", "csrrc", "csrrwi", "csrrsi", "csrrci"]
    expected_hw_metrics = ["cpu_frequency_mhz", "pipeline",
                           "alu", "mul", "fma", "div", "csr",
                           "branch_resolution", "jump_resolution",
                           "icache", "dcache", "bpred", "mem",
                           "icache_name", "dcache_name", "bpred_name"]

    def __init__(self, inst_profiler_path, hw_stats_path, hw_perf_metrics_path):
        self.inst_profiler_path = inst_profiler_path
        self.name = os.path.basename(inst_profiler_path)
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
                raise ValueError(f"Missing metric '{metric}' in " + \
                                  "HW performance metrics JSON file")

        self.c_pipeline = hwpm['pipeline']
        self.c_alu = hwpm['alu']
        self.c_csr = hwpm['csr']
        self.c_mul = hwpm['mul']
        self.c_fma = hwpm['fma']
        self.c_div = hwpm['div']
        self.c_branch_resolution = hwpm['branch_resolution']
        self.c_jump_resolution = hwpm['jump_resolution']
        self.c_ic = hwpm['icache']
        self.c_dc = hwpm['dcache']
        self.c_bp = hwpm["bpred"]
        self.c_mem = hwpm['mem']
        self.ic_name = hwpm['icache_name']
        self.dc_name = hwpm['dcache_name']
        self.bp_name = hwpm['bpred_name']

        hw_bp = self.hw_stats[self.bp_name]
        self.bp_stats = {
            "pred": hw_bp["predicted"],
            "mispred": hw_bp["mispredicted"],
            "acc": (hw_bp["predicted"] / hw_bp["branches"]) * 100
        }
        hw_ic = self.hw_stats[self.ic_name]
        hw_dc = self.hw_stats[self.dc_name]
        self.ic_stats = {
            "hits": hw_ic["hits"],
            "misses": hw_ic["misses"],
            "hit_rate": (hw_ic["hits"] / hw_ic["accesses"]) * 100
        }
        self.dc_stats = {
            "hits": hw_dc["hits"],
            "misses": hw_dc["misses"],
            "hit_rate": (hw_dc["hits"] / hw_dc["accesses"]) * 100
        }

        self.freq = hwpm['cpu_frequency_mhz']
        self.period = 1 / self.freq

        self.inst_total = df['count'].sum()
        self.b_inst = df.loc[df['name'].isin(self.b_inst_a)]['count'].sum()
        self.j_inst = df.loc[df['name'].isin(self.j_inst_a)]['count'].sum()
        self.dc_inst = df.loc[df['name'].isin(self.dc_inst_a)]['count'].sum()
        self.mul_inst = df.loc[df['name'].isin(self.mul_inst_a)]['count'].sum()
        self.fma_inst = df.loc[df['name'].isin(self.fma_inst_a)]['count'].sum()
        self.div_inst = df.loc[df['name'].isin(self.div_inst_a)]['count'].sum()
        self.csr_inst = df.loc[df['name'].isin(self.csr_inst_a)]['count'].sum()

        self.alu_inst = self.inst_total - \
                        (self.b_inst + self.j_inst + self.dc_inst + \
                         self.mul_inst + self.fma_inst + self.div_inst + \
                         self.csr_inst)
        self.alu_cycles = self.c_pipeline + self.alu_inst * self.c_alu
        self.b_cycles = self.b_inst * self.c_branch_resolution
        self.j_cycles = self.j_inst * self.c_jump_resolution
        self.dc_cycles = self.c_dc * self.hw_stats[self.dc_name]["hits"] + \
                         self.c_mem * self.hw_stats[self.dc_name]["misses"]
        self.fe_cycles = self.c_ic * self.hw_stats[self.ic_name]["hits"] + \
                         self.c_mem * self.hw_stats[self.ic_name]["misses"]
        # only count extra cycles the frontend needs to feed the intpipe
        self.fe_extra_cycles = self.fe_cycles - self.inst_total
        self.mul_cycles = self.mul_inst * self.c_mul
        self.fma_cycles = self.fma_inst * self.c_fma
        self.div_cycles = self.div_inst * self.c_div
        self.csr_cycles = self.csr_inst * self.c_csr
        self.non_b_cycles = self.fe_extra_cycles + \
                            self.alu_cycles + self.j_cycles + \
                            self.dc_cycles + self.mul_cycles + \
                            self.fma_cycles + self.div_cycles + self.csr_cycles

        self.total_cycles = self.non_b_cycles + self.b_cycles
        self.branches_perc = round((self.b_inst / self.inst_total) * 100, 2)
        self.total_cycles = int(np.ceil(self.total_cycles))
        self.perf_str = self._estimated_perf(self.total_cycles)

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
        out = f"Estimated HW performance at {self.freq}MHz: " + \
              f"cycles={cycles}, CPI={cpi:.2f}, " + \
              f"time={exec_time_us:.1f}us, MIPS={mips:.1f}"
        return out

    def __str__(self):
        out1 = f"Peak Stack usage: {self.sp_usage} bytes"
        out_b = []
        out_b.append(f"Branches total: {self.b_inst} out of " + \
                   f"{self.inst_total} total instructions " + \
                   f"({self.branches_perc:.2f}% branches)")
        out_b.append(f"Taken: {self.b['taken']}, " + \
                   f"Forwards: {self.b['taken_fwd']}, " + \
                   f"Backwards: {self.b['taken_bwd']}")
        out_b.append(f"Not taken: {self.b['not_taken']}, " + \
                   f"Forwards: {self.b['not_taken_fwd']}, " + \
                   f"Backwards: {self.b['not_taken_bwd']}")
        out_b.append(f"Predicted: {self.bp_stats['pred']}, " + \
                   f"Mispredicted: {self.bp_stats['mispred']}, " + \
                   f"Accuracy: {self.bp_stats['acc']:.2f}%")
        out_ic = f"ICache: hits={self.ic_stats['hits']}, " + \
                 f"misses={self.ic_stats['misses']}, " + \
                 f"hit rate={self.ic_stats['hit_rate']:.2f}%"
        out_dc = f"DCache: hits={self.dc_stats['hits']}, " + \
                 f"misses={self.dc_stats['misses']}, " + \
                 f"hit rate={self.dc_stats['hit_rate']:.2f}%"

        delim = "\n    "
        stats = f"{self.name}\n{out1}\n{delim.join(out_b)}" + \
                     f"\n{out_ic}\n{out_dc}"

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

    if not os.path.isfile(inst_profiler_path):
        raise ValueError(f"File {inst_profiler_path} not found")
    if not os.path.isfile(hw_stats_path):
        raise ValueError(f"File {hw_stats_path} not found")
    if not os.path.isfile(hw_perf_metrics_path):
        raise ValueError(f"File {hw_perf_metrics_path} not found")

    est = perf(inst_profiler_path, hw_stats_path, hw_perf_metrics_path)
    print(est)
