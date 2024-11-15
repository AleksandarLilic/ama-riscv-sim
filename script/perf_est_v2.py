import os
import sys
import json
import pandas as pd
import numpy as np
from run_analysis import json_prof_to_df

class perf:
    # TODO: needs compressed ISA
    b_inst_a = ["beq", "bne", "blt", "bge", "bltu", "bgeu"]
    j_inst_a = ["jalr", "jal"] # split jal vs jalr?
    dc_inst_a = ["lb", "lh", "lw", "lbu", "lhu", "sb", "sh", "sw",
                 "scp.ld", "scp.rel"]
    mul_inst_a = ["mul", "mulh", "mulhsu", "mulhu"]
    fma_inst_a = ["fma4", "fma8", "fma16"]
    div_inst_a = ["div", "divu", "rem", "remu"]
    csr_inst_a = ["csrrw", "csrrs", "csrrc", "csrrwi", "csrrsi", "csrrci"]
    expected_hw_metrics = ["cpu_frequency_mhz", "pipeline",
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
            "acc": (hw_bp["predicted"] / hw_bp["branches"]) * 100,
            "type": hw_bp["type"]
        }
        hw_ic = self.hw_stats[self.ic_name]
        hw_dc = self.hw_stats[self.dc_name]
        self.ic_stats = {
            "accesses": hw_ic["accesses"],
            "hits": hw_ic["hits"],
            "misses": hw_ic["misses"],
            "hit_rate": (hw_ic["hits"] / hw_ic["accesses"]) * 100,
            "sets": hw_ic["size"]["sets"],
            "ways": hw_ic["size"]["ways"],
            "data": hw_ic["size"]["data"]
        }
        self.dc_stats = {
            "accesses": hw_dc["accesses"],
            "hits": hw_dc["hits"],
            "misses": hw_dc["misses"],
            "hit_rate": (hw_dc["hits"] / hw_dc["accesses"]) * 100,
            "sets": hw_dc["size"]["sets"],
            "ways": hw_dc["size"]["ways"],
            "data": hw_dc["size"]["data"]
        }

        self.freq = hwpm['cpu_frequency_mhz']
        self.period = 1 / self.freq

        self.inst_total = df['count'].sum()
        self.b_inst = df.loc[df['name'].isin(self.b_inst_a)]['count'].sum()
        self.j_inst = df.loc[df['name'].isin(self.j_inst_a)]['count'].sum()
        self.dc_inst = df.loc[df['name'].isin(self.dc_inst_a)]['count'].sum()

        self.ipc_1_cycles = self.c_pipeline + self.inst_total
        # extra cycles for insts that might stall the pipeline
        self.b_stalls = \
            (self.c_bp - 1) * self.bp_stats['pred'] + \
            self.c_branch_resolution * self.bp_stats['mispred']
        self.j_stalls = (self.c_jump_resolution - 1) * self.j_inst
        self.dc_stalls = \
            (self.c_dc - 1) * self.hw_stats[self.dc_name]["hits"] + \
            self.c_mem * self.hw_stats[self.dc_name]["misses"]
        self.ic_stalls = \
            (self.c_ic - 1) * self.hw_stats[self.ic_name]["hits"] + \
            self.c_mem * self.hw_stats[self.ic_name]["misses"]

        self.total_cycles = self.ipc_1_cycles + \
                            self.j_stalls + self.b_stalls + \
                            self.dc_stalls + self.ic_stalls
        self.branches_perc = round((self.b_inst / self.inst_total) * 100, 2)
        self.ls_perc = round((self.dc_inst / self.inst_total) * 100, 2)
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
              f"Cycles: {cycles}, CPI: {cpi:.2f}, " + \
              f"Time: {exec_time_us:.1f}us, MIPS: {mips:.1f}"
        return out

    def _cache_stats_str(self, name, stats):
        out = f"{name} " + \
              f"({stats['sets']} sets, {stats['ways']} ways, " + \
              f"{stats['data']}B data): " + \
              f"Accesses: {stats['accesses']}, " + \
              f"Hits: {stats['hits']}, " + \
              f"Misses: {stats['misses']}, " + \
              f"Hit Rate: {stats['hit_rate']:.2f}%"
        return out

    def __str__(self):
        out_sp = f"Peak Stack usage: {self.sp_usage} bytes"
        out_ic = []
        out_dc = []
        out_b = []

        out_ic.append(f"Instructions executed: {self.inst_total}")
        out_ic.append(self._cache_stats_str(self.ic_name, self.ic_stats))

        out_dc.append(
            f"Loads & Stores: {self.dc_inst} " + \
            f"({self.ls_perc:.2f}% instructions)"
            )
        out_dc.append(self._cache_stats_str(self.dc_name, self.dc_stats))

        out_b.append(
            f"Branches: {self.b_inst} " + \
            f"({self.branches_perc:.2f}% instructions)"
            )
        #out_b.append(
        #    f"Taken: {self.b['taken']}, " + \
        #    f"Forwards: {self.b['taken_fwd']}, " + \
        #    f"Backwards: {self.b['taken_bwd']}"
        #    )
        #out_b.append(
        #    f"Not taken: {self.b['not_taken']}, " + \
        #    f"Forwards: {self.b['not_taken_fwd']}, " + \
        #    f"Backwards: {self.b['not_taken_bwd']}"
        #    )
        out_b.append(
            f"Branch Predictor ({self.bp_stats['type']}): " + \
            f"Predicted: {self.bp_stats['pred']}, " + \
            f"Mispredicted: {self.bp_stats['mispred']}, " + \
            f"Accuracy: {self.bp_stats['acc']:.2f}%"
            )

        out_stalls = f"Pipeline stalls: " + \
            f"Jumps: {self.j_stalls}, " + \
            f"Branches: {self.b_stalls}, " + \
            f"DCache: {self.dc_stalls}, ICache: {self.ic_stalls}"

        delim = "\n    "
        stats = f"{self.name}" + \
                f"\n{out_sp}" + \
                f"\n{delim.join(out_ic)}" + \
                f"\n{delim.join(out_dc)}" + \
                f"\n{delim.join(out_b)}" + \
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

    if not os.path.isfile(inst_profiler_path):
        raise ValueError(f"File {inst_profiler_path} not found")
    if not os.path.isfile(hw_stats_path):
        raise ValueError(f"File {hw_stats_path} not found")
    if not os.path.isfile(hw_perf_metrics_path):
        raise ValueError(f"File {hw_perf_metrics_path} not found")

    est = perf(inst_profiler_path, hw_stats_path, hw_perf_metrics_path)
    print(est)
