import os
import sys
import json
import pandas as pd
import numpy as np
from run_analysis import json_prof_to_df

class perf:
    b_inst_a = ["beq", "bne", "blt", "bge", "bltu", "bgeu"]
    j_inst_a = ["jalr", "jal"]
    dc_inst_a = ["lb", "lh", "lw", "lbu", "lhu", "sb", "sh", "sw"]
    mul_inst_a = ["mul", "mulh", "mulhsu", "mulhu"]
    div_inst_a = ["div", "divu", "rem", "remu"]
    expected_hw_metrics = ["cpu_frequency_mhz", "pipeline_latency",
                           "multiplier_latency", "divider_latency",
                           "branch_resolution", "jump_resolution",
                           "icache_response", "dcache_response",
                           "mispredict_penalty", "prediction_resolution"]

    def __init__(self, inst_profiler_path, hw_perf_metrics_path):
        self.inst_profiler_path = inst_profiler_path
        self.name = os.path.basename(inst_profiler_path)
        df = json_prof_to_df(inst_profiler_path, allow_internal=True)
        # get internal keys into dfi and remove from df
        dfi = df.loc[df['name'].str.startswith('_')]
        df = df.loc[df['name'].str.startswith('_') == False]
        self.sp_usage = dfi[dfi['name'] == "_max_sp_usage"]['count'].tolist()[0]

        self.b = {"taken": 0, "taken_fwd": 0, "taken_bwd": 0,
                  "not_taken": 0, "not_taken_fwd": 0, "not_taken_bwd": 0}
        self.p = {"pred": 0, "mispred": 0, "acc": 0.0}
        self.est = {}

        with open(inst_profiler_path, 'r') as file:
            prof = json.load(file)

        for b in self.b_inst_a:
            self._log_branches(prof[b])

        with open(hw_perf_metrics_path, 'r') as file:
            hwpm = json.load(file)

        # check if all expected metrics are present
        for metric in self.expected_hw_metrics:
            if metric not in hwpm:
                raise ValueError(f"Missing metric '{metric}' in " + \
                                  "HW performance metrics JSON file")

        self.mispredict_penalty = hwpm["mispredict_penalty"]
        self.prediction_resolution = hwpm["prediction_resolution"]
        self.branch_resolution = hwpm['branch_resolution']
        self.jump_resolution = hwpm['jump_resolution']
        self.ic_response = hwpm['icache_response']
        self.dc_response = hwpm['dcache_response']
        self.pipeline_latency = hwpm['pipeline_latency']
        self.multiplier_latency = hwpm['multiplier_latency']
        self.divider_latency = hwpm['divider_latency']

        self.cpu_frequency_mhz = hwpm['cpu_frequency_mhz']
        self.cpu_period = 1 / self.cpu_frequency_mhz

        self.inst_total = df['count'].sum()
        self.b_inst = df.loc[df['name'].isin(self.b_inst_a)]['count'].sum()
        self.j_inst = df.loc[df['name'].isin(self.j_inst_a)]['count'].sum()
        self.dc_inst = df.loc[df['name'].isin(self.dc_inst_a)]['count'].sum()
        self.mul_inst = df.loc[df['name'].isin(self.mul_inst_a)]['count'].sum()
        self.div_inst = df.loc[df['name'].isin(self.div_inst_a)]['count'].sum()

        self.other_inst = self.inst_total - \
                          (self.b_inst + self.j_inst + self.dc_inst + \
                           self.mul_inst + self.div_inst)
        # 1 instruction per cycle + pipeline latency
        self.other_cycles = self.pipeline_latency + self.other_inst

        self.j_cycles = self.j_inst * (1 + self.jump_resolution)
        self.b_cycles = self.b_inst * (1 + self.branch_resolution)
        self.dc_cycles = self.dc_inst * (1 + self.dc_response)
        self.mul_cycles = self.mul_inst * (1 + self.multiplier_latency)
        self.div_cycles = self.div_inst * (1 + self.divider_latency)
        self.non_b_cycles = self.other_cycles + self.j_cycles + \
                            self.dc_cycles + self.mul_cycles + self.div_cycles

        self.total_cycles = self.non_b_cycles + self.b_cycles
        self.total_cycles *= self.ic_response # add average icache response clks
        self.branches_perc = round((self.b_inst / self.inst_total) * 100, 1)
        self._predictor_btfn()
        self.total_cycles = int(np.ceil(self.total_cycles))
        self.new_total_cycles = int(np.ceil(self.new_total_cycles))
        self.og_perf_str = self._estimated_perf(self.total_cycles, "original")
        self.new_perf_str = self._estimated_perf(self.new_total_cycles, "new")

    def _log_branches(self, entry):
        for key in entry['breakdown']:
            self.b[key] += entry['breakdown'][key]

    def _predictor_btfn(self): # backwards taken, forwards not
        self.p["pred"] = self.b["taken_bwd"] + self.b["not_taken_fwd"]
        self.p["mispred"] = self.b["taken_fwd"] + self.b["not_taken_bwd"]
        self.p["acc"] = round((self.p["pred"] / self.b_inst) * 100, 2)

        self.new_branch_cycles = \
            self.p["pred"] * (1 + self.prediction_resolution) +\
            self.p["mispred"] * (1 + self.mispredict_penalty)
        self.new_total_cycles = self.non_b_cycles + self.new_branch_cycles
        self.new_total_cycles *= self.ic_response
        self.saved_cycles = self.total_cycles - self.new_total_cycles
        self.saved_cycles = int(np.ceil(self.saved_cycles))
        self.speedup = \
            round((self.saved_cycles / self.total_cycles) * 100,2)

    def _estimated_perf(self, cycles, tag):
        cycles = int(np.ceil(cycles))
        cpi = cycles / self.inst_total
        exec_time_us = cycles * self.cpu_period
        mips = self.inst_total / exec_time_us
        self.est[f"{tag}_cpi"] = round(cpi,2)
        self.est[f"{tag}_exec_time_us"] = round(exec_time_us,2)
        self.est[f"{tag}_mips"] = round(mips,2)
        out = f"Estimated HW performance at {self.cpu_frequency_mhz}MHz " + \
              f"with {cycles} cycles executed: CPI={cpi:.2f}, " + \
              f"exec time={exec_time_us:.1f}us, MIPS={mips:.1f}"
        return out

    def __str__(self):
        out1 = f"Peak Stack usage: {self.sp_usage} bytes"
        out = []
        out.append(f"Branches total: {self.b_inst} out of " + \
                   f"{self.inst_total} total instructions " + \
                   f"({self.branches_perc:.1f}% branches)")
        out.append(f"Taken: {self.b['taken']}, " + \
                   f"Forwards: {self.b['taken_fwd']}, " + \
                   f"Backwards: {self.b['taken_bwd']}")
        out.append(f"Not taken: {self.b['not_taken']}, " + \
                   f"Forwards: {self.b['not_taken_fwd']}, " + \
                   f"Backwards: {self.b['not_taken_bwd']}")
        out.append(f"Predicted: {self.p['pred']}, " + \
                   f"Mispredicted: {self.p['mispred']}, " + \
                   f"Accuracy: {self.p['acc']:.1f}%")
        out.append(f"Cycles: Original/With prediction: " + \
                   f"{self.total_cycles}/{self.new_total_cycles} " + \
                   f"({self.saved_cycles} cycles saved)")
        out2 = f"Potential app speedup: {self.speedup:.1f}%"
        delim = "\n    "
        branch_stats = f"{self.name}\n{out1}\n{delim.join(out)}\n{out2}"

        return f"{branch_stats}\n{self.og_perf_str}\n{self.new_perf_str}"

    def save_as_df(self) -> None:
        attrs = vars(self).copy()
        branches = attrs.pop('b')
        predictions = attrs.pop('p')
        est = attrs.pop('est')
        _ = attrs.pop('inst_profiler_path')
        _ = attrs.pop('og_perf_str')
        _ = attrs.pop('new_perf_str')
        all_flat = {**attrs, **branches, **predictions, **est}
        df = pd.DataFrame([all_flat])
        df.to_csv(self.inst_profiler_path.replace(".json", "_perf_est.csv"),
                  index=False)

if __name__ == "__main__":
    inst_profiler_path = sys.argv[1]
    hw_perf_metrics_path = sys.argv[2]

    if not os.path.isfile(inst_profiler_path):
        raise ValueError(f"File {inst_profiler_path} not found")
    if not os.path.isfile(hw_perf_metrics_path):
        raise ValueError(f"File {hw_perf_metrics_path} not found")

    est = perf(inst_profiler_path, hw_perf_metrics_path)
    print(est)
