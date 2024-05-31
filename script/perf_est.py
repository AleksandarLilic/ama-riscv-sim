import os
import sys
import json
import pandas as pd
from analyze_profiling_log import json_prof_to_df

class perf:
    b_inst = ["beq", "bne", "blt", "bge", "bltu", "bgeu"]
    j_inst = ["jalr", "jal"]
    m_inst = ["lb", "lh", "lw", "lbu", "lhu", "sb", "sh", "sw"]
    expected_hw_metrics = ["cpu_frequency_mhz", "pipeline_latency",
                           "branch_resolution", "jump_resolution",
                           "memory_response",
                           "mispredict_penalty", "prediction_resolution"]

    def __init__(self, inst_profiler_path, hw_perf_metrics_path):
        self.inst_profiler_path = inst_profiler_path
        self.name = os.path.basename(inst_profiler_path)
        df = json_prof_to_df(inst_profiler_path)

        self.b = {"taken": 0, "taken_fwd": 0, "taken_bwd": 0,
                  "not_taken": 0, "not_taken_fwd": 0, "not_taken_bwd": 0}
        self.p = {"pred": 0, "mispred": 0, "acc": 0.0}
        self.est = {}

        with open(inst_profiler_path, 'r') as file:
            prof = json.load(file)

        for b in self.b_inst:
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
        self.pipeline_latency = hwpm['pipeline_latency']
        self.cpu_frequency_mhz = hwpm['cpu_frequency_mhz']
        self.cpu_period = 1 / self.cpu_frequency_mhz

        self.inst_total = df['count'].sum()
        self.b_inst = df.loc[df['name'].isin(self.b_inst)]['count'].sum()
        self.j_inst = df.loc[df['name'].isin(self.j_inst)]['count'].sum()
        self.m_inst = df.loc[df['name'].isin(self.m_inst)]['count'].sum()

        self.other_inst = self.inst_total - \
                          (self.b_inst + self.j_inst + self.m_inst)
        self.other_cycles = hwpm["pipeline_latency"] + self.other_inst
        self.j_cycles = self.j_inst * (1 + hwpm["jump_resolution"])
        self.b_cycles = self.b_inst * (1 + hwpm["branch_resolution"])
        self.m_cycles = self.m_inst * (1 + hwpm["memory_response"])
        self.non_b_cycles = self.other_cycles + self.j_cycles + self.m_cycles
        self.total_cycles = self.non_b_cycles + self.b_cycles
        self.branches_perc = round((self.b_inst / self.inst_total) * 100, 1)
        self._predictor_btfn()
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
        self.saved_cycles = self.total_cycles - self.new_total_cycles
        self.speedup = \
            round((self.saved_cycles / self.total_cycles) * 100,2)

    def _estimated_perf(self, cycles, tag):
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
        branch_stats = f"{self.name}\n{delim.join(out)}\n{out2}"

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
