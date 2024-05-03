import os
import sys
import json
import pandas as pd
from analyze_profiling_log import json_prof_to_df

class branch_prof:
    branches = ["beq", "bne", "blt", "bge", "bltu", "bgeu"]
    jumps = ["jalr", "jal"]
    hw_cycles_w_pred = 0
    hw_speedup = 0
    b = {"taken": 0, "taken_fwd": 0, "taken_bwd": 0, 
         "not_taken": 0, "not_taken_fwd": 0, "not_taken_bwd": 0}
    p = {"pred": 0, "misp": 0, "acc": 0.0}
    
    def __init__(self, inst_profiler_path, hw_perf_metrics_path,
                 mispredict_penalty=1, prediction_resolution=0):
        self.name = os.path.basename(inst_profiler_path)
        df = json_prof_to_df(inst_profiler_path)
        
        with open(inst_profiler_path, 'r') as file:
            prof = json.load(file)
        
        for b in self.branches:
            self._log_branches(prof[b])

        with open(hw_perf_metrics_path, 'r') as file:
            hwpm = json.load(file)

        
        self.mispredict_penalty = mispredict_penalty
        self.prediction_resolution = prediction_resolution
        self.pipeline_latency = hwpm['pipeline_latency']
        self.cpu_frequency_mhz = hwpm['cpu_frequency_mhz']
        
        self.inst_total = df['count'].sum()
        self.branches = df.loc[df['name'].isin(self.branches)]['count'].sum()
        self.jumps = df.loc[df['name'].isin(self.jumps)]['count'].sum()

        other_inst = self.inst_total - self.branches - self.jumps
        self.hw_other_cycles = hwpm["pipeline_latency"] + other_inst + \
                               self.jumps * (1 + hwpm["jump_resolution"])
        self.branch_cycles = self.branches * (1 + hwpm["branch_resolution"])
        self.total_cycles = self.hw_other_cycles + self.branch_cycles
        self._predictor_btfn()
        
    def _log_branches(self, entry):
        for key in entry['breakdown']:
            self.b[key] += entry['breakdown'][key]

    def _predictor_btfn(self): # backwards taken, forwards not
        self.p["pred"] = self.b["taken_bwd"] + self.b["not_taken_fwd"]
        self.p["misp"] = self.b["taken_fwd"] + self.b["not_taken_bwd"]
        self.p["acc"] = (self.p["pred"] / self.branches) * 100

        self.new_branch_cycles = \
            self.p["pred"] * (1 + self.prediction_resolution) +\
            self.p["misp"] * (1 + self.mispredict_penalty)
        self.new_total_cycles = self.hw_other_cycles + self.new_branch_cycles
        self.saved_cycles = self.total_cycles - self.new_total_cycles
        self.hw_speedup = (self.saved_cycles / self.total_cycles) * 100
    
    def _estimated_perf(self, cycles):
        self.cpi = cycles / self.inst_total
        self.cpu_period = 1 / self.cpu_frequency_mhz
        self.exec_time_us = cycles * self.cpu_period
        self.mips = self.inst_total / self.exec_time_us
        out = f"Estimated HW performance at {self.cpu_frequency_mhz}MHz " + \
              f"with {cycles} cycles executed: CPI={self.cpi:.2f}, " + \
              f"exec time={self.exec_time_us:.1f}us, MIPS={self.mips:.1f}"
        return out
    
    def __str__(self):
        out = []
        out.append(f"Branches total: {self.branches} out of " + \
                   f"{self.inst_total} total instructions " + \
                   f"({(self.branches/self.inst_total)*100:.1f}% branches)")
        out.append(f"Taken: {self.b['taken']}, " + \
                   f"Forwards: {self.b['taken_fwd']}, " + \
                   f"Backwards: {self.b['taken_bwd']}")
        out.append(f"Not taken: {self.b['not_taken']}, " + \
                   f"Forwards: {self.b['not_taken_fwd']}, " + \
                   f"Backwards: {self.b['not_taken_bwd']}")
        out.append(f"Predicted: {self.p['pred']}, " + \
                   f"Mispredicted: {self.p['misp']}, " + \
                   f"Accuracy: {self.p['acc']:.1f}%")
        out.append(f"Cycles: Original/With prediction: " + \
                   f"{self.total_cycles}/{self.new_total_cycles} " + \
                   f"({self.saved_cycles} cycles saved)")
        out2 = f"Potential app speedup: {self.hw_speedup:.1f}%"
        delim = "\n    "
        branch_stats = f"{self.name}\n{delim.join(out)}\n{out2}"
        og_perf = self._estimated_perf(self.total_cycles)
        new_perf = self._estimated_perf(self.new_total_cycles)
        return f"{branch_stats}\n{og_perf}\n{new_perf}"

if __name__ == "__main__":
    inst_profiler_path = sys.argv[1]
    hw_perf_metrics_path = sys.argv[2]
    
    if not os.path.isfile(inst_profiler_path):
        raise ValueError(f"File {inst_profiler_path} not found")
    if not os.path.isfile(hw_perf_metrics_path):
        raise ValueError(f"File {hw_perf_metrics_path} not found")
    
    mispredict_penalty = 1
    prediction_resolution = 0
    if len(sys.argv) > 3:
        mispredict_penalty = int(sys.argv[3])
    
    if len(sys.argv) > 4:
        prediction_resolution = int(sys.argv[4])
    
    bp = branch_prof(inst_profiler_path, hw_perf_metrics_path,
                     mispredict_penalty, prediction_resolution)
    print(bp)
