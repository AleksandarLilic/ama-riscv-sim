#!/usr/bin/env python3
import argparse
import json
import os

import numpy as np
import pandas as pd
from dep_scan import search, search_args
from ruamel.yaml import YAML
from run_analysis import icfg, load_inst_prof
from types import SimpleNamespace
from utils import DELIM, INDENT, smarter_eng_formatter, get_test_title

yaml = YAML()
yaml.preserve_quotes = True

SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))
DEFAULT_HW_YAML = f"{SCRIPT_PATH}/hw_perf_metrics_v2.yaml"

FMT   = smarter_eng_formatter(unit='', places=2, sep="")
FMT_T = smarter_eng_formatter(unit='s', places=2, sep="")

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
    simd_dot_inst_a = icfg.INST_T_SIMD_ARITH[icfg.k_simd_dot]
    simd_mul_inst_a = icfg.INST_T_SIMD_ARITH[icfg.k_simd_mul] + \
        icfg.INST_T_SIMD_ARITH[icfg.k_simd_wmul]
    simd_add_sub_inst_a = icfg.INST_T_SIMD_ARITH[icfg.k_simd_add_sub]
    simd_arith_a = icfg.INST_T[icfg.k_simd_arith]
    simd_data_fmt_a = icfg.INST_T[icfg.k_simd_data_fmt]
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
        "mul", "div", "simd_dot", "simd_mul", #"simd_sdd_sub"
        "dcache_load", #"dcache_store",
        # names
        "icache_name", "dcache_name", "bpred_name"
    ]

    def __init__(self, inst_profile, hw_stats, hw_perf_metrics, rf_trace=None):
        self.inst_profile = inst_profile
        self.inputs = [inst_profile, hw_stats, hw_perf_metrics, rf_trace]
        df = load_inst_prof(inst_profile, allow_internal=True)
        # get internal keys into dfi and remove from df
        dfi = df.loc[df['name'].str.startswith('_')]
        df = df.loc[df['name'].str.startswith('_') == False]
        self.inst_total = df['count'].sum()
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
        self.c_div = SimpleNamespace()
        self.c_div.cache = hwpm['div']['cache']
        self.c_div.special = hwpm['div']['special']
        self.c_div.common_overhead = hwpm['div']['common_overhead']
        self.c_simd_dot = hwpm['simd_dot']
        self.c_simd_mul = hwpm['simd_mul']
        self.c_dc_load = hwpm['dcache_load']
        #self.c_dc_store = hwpm['dcache_store']

        self.ic_name = hwpm['icache_name']
        self.dc_name = hwpm['dcache_name']
        self.bp_name = hwpm['bpred_name']
        self.div_name = hwpm['divider_name']

        sd = lambda d: sum(d.values())

        hw_bp = self.hw_stats[self.bp_name]
        self.bp_stats = {
            "pred": hw_bp["predicted"],
            "mispred": hw_bp["mispredicted"],
            "acc": hw_bp["accuracy"],
            "mpki": hw_bp["mpki"],
            "type": hw_bp["type"]
        }
        hws_ic = self.hw_stats[self.ic_name]
        hws_dc = self.hw_stats[self.dc_name]

        def get_hr(hits, tot):
            if tot == 0:
                return 0
            return hits / tot * 100

        self.ic_stats = hws_ic
        self.dc_stats = hws_dc
        self.dc_stats['wb_rate'] = get_hr(
            hws_dc["writebacks"], hws_dc["references"])

        hw_div = self.hw_stats[self.div_name]
        self.div_stats = {
            "total": hw_div["total"],
            "cache": hw_div["cache"]["count"],
            "special": hw_div["special_cases"]["count"],
            "common": hw_div["common_cases"]["count"],
            "common_bits": hw_div["common_cases_info"]["total"],
            "cache_f": hw_div["cache"]["fraction"],
            "special_f": hw_div["special_cases"]["fraction"],
            "common_f": hw_div["common_cases"]["fraction"],
            "common_b": hw_div["common_cases_info"]["total"],
            "common_bd": hw_div["common_cases_info"]["avg"],
            "size": hw_div["size"]["total"]
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
        self.simd_dot_inst = sum_up(self.simd_dot_inst_a)
        self.simd_mul_inst = sum_up(self.simd_mul_inst_a)
        self.simd_add_sub_inst = sum_up(self.simd_add_sub_inst_a)
        self.simd_arith = sum_up(self.simd_arith_a)
        self.simd_data_fmt = sum_up(self.simd_data_fmt_a)

        # ipc = 1 -> best case, at least this many cycles needed
        self.ipc_1_cycles = self.c_pipeline + self.inst_total

        ### extra cycles for insts that might stall the pipeline
        ### 'stalls' are non-negotiable - they will happen, but can overlap
        ### 'hazards' may (will stall) or may not happen (won't stall)

        self.bp_stalls = (self.c_bp_hit - 1) * self.bp_stats['pred']
        self.bp_stalls += (self.c_bp_miss - 1) * self.bp_stats['mispred']

        self.j_stalls = (self.c_jd_res - 1) * self.jd_inst
        self.j_stalls += (self.c_ji_res - 1) * self.ji_inst

        self.ic_stalls = (self.c_ic_hit - 1) * hws_ic["hits"]["reads"]
        self.ic_stalls += (self.c_ic_miss - 1) * hws_ic["misses"]["reads"]
        ic_miss_rate = (100 - self.ic_stats['hr']) / 100
        ic_miss_penalty = self.c_ic_miss - self.c_ic_hit
        self.ic_amat = self.c_ic_hit + (ic_miss_rate * ic_miss_penalty)
        self.ic_amat = round(self.ic_amat, 2)

        # dc miss incurs c_dc_miss clk always, like ic
        # if dc can't handle read and write to the same cache line at once
        # or main mem has only 1 R/W port to dc
        # writeback incurs c_dc_wb clk to first write back evicted cache line
        self.dc_stalls = (self.c_dc_hit - 1) * sd(hws_dc["hits"])
        self.dc_stalls += (self.c_dc_miss - 1) * sd(hws_dc["misses"])
        self.dc_stalls += self.c_dc_wb * hws_dc["writebacks"]
        dc_miss_rate = (100 - self.dc_stats['hr']) / 100
        dc_miss_penalty = self.c_dc_miss - self.c_dc_hit
        self.dc_amat = (
            self.c_dc_hit + \
            (dc_miss_rate * dc_miss_penalty) + \
            (self.dc_stats['wb_rate']/100 * self.c_dc_wb)
        )
        self.dc_amat = round(self.dc_amat, 2)

        self.div_stalls = SimpleNamespace()
        self.div_stalls.cached = (
            self.c_div.cache - 1) * self.div_stats["cache"]
        self.div_stalls.special = (
            self.c_div.special - 1) * self.div_stats["special"]
        self.div_stalls.common = (
            self.c_div.common_overhead - 1) * self.div_stats["common"] + \
            self.div_stats["common_bits"]
        self.all_div_stalls = sum(vars(self.div_stalls).values())

        def find_hazards(win, src, dep="_any_"):
            r, _ = search(search_args(rf_trace, src=src, dep=dep, window=win))
            #print(r)
            #print(r.dep_arr_cnt)
            #print(r.dep_arr_cnt_dot_acc)
            return sum(r.dep_arr_cnt), sum(r.dep_arr_cnt_dot_acc)

        self.hazards = {
            "dcache": 0, "mul": 0, "div": 0,
            "simd_dot": 0, "simd_mul": 0,
        }
        hazard_penalty = {
            "dcache": (self.c_dc_load - 1),
            "mul": (self.c_mul - 1),
            "simd_dot": (self.c_simd_dot - 1),
            "simd_mul": (self.c_simd_mul - 1),
        }

        fmt = lambda x: ','.join(x)
        # hazards occur when inst that take >=2 clk is followed up by
        # an instruction that uses rd of that instruction as its rs1/2
        self.hazards["dcache"], _ = find_hazards(
            hazard_penalty['dcache'], fmt(self.ld_inst_a)
        )
        self.hazards["mul"], _ = find_hazards(
            hazard_penalty['mul'], fmt(self.mul_inst_a)
        )
        self.hazards["simd_dot"], dot_acc_hazards = find_hazards(
            hazard_penalty['simd_dot'], fmt(self.simd_dot_inst_a)
        )
        self.hazards["simd_dot"] -= dot_acc_hazards # late_c fwd in RTL
        self.hazards["simd_mul"], _ = find_hazards(
            hazard_penalty['simd_mul'], fmt(self.simd_mul_inst_a)
        )
        self.hazards["simd_add_sub"], _ = find_hazards(
            hazard_penalty['simd_mul'], fmt(self.simd_add_sub_inst_a)
        )

        self.all_hazards = sum(self.hazards.values())
        self.simd_mul_hazards = (
            self.hazards["mul"] + \
            self.hazards["simd_dot"] + \
            self.hazards["simd_add_sub"]
        )

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
        self.be_stalls = self.dc_stalls + self.all_hazards + self.all_div_stalls

        self._est_stalls()

        self.branches_perc = round((self.b_inst / self.inst_total) * 100, 2)
        self.ls_perc = round((self.dc_inst / self.inst_total) * 100, 2)
        self.div_perc = round((self.div_inst / self.inst_total) * 100, 2)
        self.t_clk_ec = int(np.ceil(self.t_clk_ec))
        self.t_clk_bc = int(np.ceil(self.t_clk_bc))

        self.perf_str = f"\nEstimated HW performance at {self.freq}MHz:"
        self.perf_str += f"{DELIM}Best:     "
        self.perf_str += self._estimated_perf(self.t_clk_bc, "best")
        self.perf_str += f"{DELIM}Expected: "
        self.perf_str += self._estimated_perf(self.t_clk_ec, "exp")

        width = self.est["exp"]["clk"] - self.est["best"]["clk"]
        midpoint = (self.est["best"]["clk"] + self.est["exp"]["clk"]) >> 1
        est_ratio = width / midpoint
        self.perf_str += (
            f"{DELIM}Estimated Cycles range: {FMT(width)} cycles, " +
            f"midpoint: {FMT(int(midpoint))}, " +
            f"ratio: {est_ratio*100:.2f}%"
        )

    # TODO:
    # D$ and COMP stalls can technically overlap, but unlikely in current uarch
    def _est_stalls(self):
        # expected case:
        #   fe stalls never overlap with be stalls
        self.t_clk_ec = self.ipc_1_cycles
        self.t_clk_ec += self.bp_stalls
        self.t_clk_ec += self.fe_stalls
        self.t_clk_ec += self.be_stalls
        #self.t_clk_ec += self.mrpc_stalls + self.mwpc_stalls

        # best case:
        #   fe stalls overlap with be stalls completely
        self.t_clk_bc = self.ipc_1_cycles
        self.t_clk_bc += self.bp_stalls
        self.t_clk_bc += max(self.be_stalls, self.fe_stalls)
        #self.t_clk_bc += self.mrpc_stalls + self.mwpc_stalls

    def _log_branches(self, entry):
        for key in entry['breakdown']:
            self.b[key] += entry['breakdown'][key]

    def _estimated_perf(self, cycles, mode):
        # core
        cycles = int(np.ceil(cycles))
        cpi = cycles / self.inst_total
        exec_time_us = cycles * self.period
        exec_time_s = exec_time_us / 1_000_000
        self.est[mode] = {}
        self.est[mode]["cpi"] = round(cpi, 3)
        self.est[mode]["ipc"] = round(1/cpi, 3)
        self.est[mode]["clk"] = int(cycles)
        self.est[mode]["exec_time_us"] = round(exec_time_us, 2)

        # memory
        to_mb = lambda d: d / exec_time_s / 2**20
        bw_ic = to_mb(self.ic_stats["ct_core"]["reads"])
        bw_dc_r = to_mb(self.dc_stats["ct_core"]["reads"])
        bw_dc_w = to_mb(self.dc_stats["ct_core"]["writes"])
        bw_dc = bw_dc_r + bw_dc_w
        bw_mem_r = to_mb(
            self.ic_stats["ct_mem"]["reads"] + self.dc_stats["ct_mem"]["reads"])
        bw_mem_w = to_mb(self.dc_stats["ct_mem"]["writes"])
        bw_mem = bw_mem_r + bw_mem_w
        self.est[mode]["bw_ic"] = {"read": bw_ic}
        self.est[mode]["bw_dc"] = {"read": bw_dc_r, "write": bw_dc_w}
        self.est[mode]["bw_mem"] = {"read": bw_mem_r, "write": bw_mem_w}

        # print
        out = f"{FMT(cycles)} cycles ({FMT_T(exec_time_s)}), " \
              f"IPC: {1/cpi:.3f}; " \
              f"BW (avg MB/s) - icache: {bw_ic:.1f}, " \
              f"dcache (R/W): {bw_dc:.1f} ({bw_dc_r:.1f}/{bw_dc_w:.1f}), " \
              f"mem (R/W): {bw_mem:.1f} ({bw_mem_r:.1f}/{bw_mem_w:.1f})"

        return out

    def _cache_stats_str(self, name, stats):
        sd = lambda d: sum(d.values())
        out = f"{name} " + \
              f"({stats['size']['sets']} sets, " + \
              f"{stats['size']['ways']} ways, " + \
              f"{stats['size']['data']}B data): " + \
              f"References: {FMT(stats['references'])}, " + \
              f"Hits: {FMT(sd(stats['hits']))}, " + \
              f"Misses: {FMT(sd(stats['misses']))}, "

        if "writebacks" in stats and stats['writebacks'] > 0:
            out += f"Writebacks: {FMT(stats['writebacks'])}, "

        out += f"Hit Rate: {stats['hr']:.2f}%, " + \
               f"MPKI: {stats['mpki']:.2f}"

        return out

    def __str__(self):
        out_sp = f"Peak Stack usage: {self.sp_usage} bytes"
        out_ic = []
        out_dc = []
        out_b = []
        out_div = []

        out_ic.append(f"Instructions executed: {FMT(self.inst_total)}")
        out_ic.append(self._cache_stats_str(self.ic_name, self.ic_stats))

        out_dc.append(
            f"DMEM inst: {FMT(self.dc_inst)} - "
            f"L/S: {FMT(self.ld_inst)}/{FMT(self.st_inst)} "
            f"({self.ls_perc:.2f}% instructions)"
        )
        out_dc.append(self._cache_stats_str(self.dc_name, self.dc_stats))

        out_b.append(
            f"Branch inst: {self.b_inst} "
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
            f"bpred ({self.bp_stats['type']}): "
            f"Predicted: {FMT(self.bp_stats['pred'])}, "
            f"Mispredicted: {FMT(self.bp_stats['mispred'])}, "
            f"Accuracy: {self.bp_stats['acc']:.2f}%, "
            f"MPKI: {self.bp_stats['mpki']}"
        )

        out_div.append(
            f"DIV/REM inst: {self.div_inst} "
            f"({self.div_perc:.2f}% instructions)"
        )
        out_div.append(
            f"divider ({self.div_stats['size']}B): "
            f"Cache: {FMT(self.div_stats['cache'])} "
            f"({self.div_stats['cache_f']}%), "
            f"Special: {FMT(self.div_stats['special'])} "
            f"({self.div_stats['special_f']}%), "
            f"Common: {FMT(self.div_stats['common'])} "
            f"({self.div_stats['common_f']}%), "
            f"{self.div_stats['common_b']} b, "
            f"{self.div_stats['common_bd']} b/d"
        )

        out_stalls = f"\nPipeline stalls (max): " + \
            f"{DELIM}Bad spec: {FMT(self.bp_stalls)}" + \
            f"{DELIM}FE bound: {FMT(self.fe_stalls)} - " + \
            f"ICache: {FMT(self.ic_stalls)} (AMAT: {self.ic_amat}), " + \
            f"Core: {FMT(self.j_stalls)}" + \
            f"{DELIM}BE bound: {FMT(self.be_stalls)} - " + \
            f"DCache: {FMT(self.dc_stalls)} (AMAT: {self.dc_amat}), " + \
            f"Core: {FMT(self.all_hazards + self.all_div_stalls)} " + \
            f"(SIMD {FMT(self.simd_mul_hazards)}, " + \
            f"DIV {FMT(self.all_div_stalls)}, " + \
            f"Load {FMT(self.hazards['dcache'])})"
            #f"{DELIM}Memory contention: " + \
            #    f"{FMT(self.mrpc_stalls + self.mwpc_stalls)} "

        stats = f"Performance estimate breakdown for: \n{INDENT}" + \
                f'\n{INDENT}'.join(self.inputs) + "\n" + \
                f"\n{out_sp}" + \
                f"\n{DELIM.join(out_ic)}" + \
                f"\n{DELIM.join(out_dc)}" + \
                f"\n{DELIM.join(out_b)}" + \
                f"\n{DELIM.join(out_div)}" + \
                f"\n{out_stalls}"

        return f"{stats}\n{self.perf_str}"

    def _core_dict(self) -> dict:
        stall_fe = self.fe_stalls
        stall_be = self.be_stalls
        bad_spec = self.bp_stalls
        stalls = stall_fe + stall_be
        lost_other = 0
        lost = bad_spec + lost_other
        ret_simd = self.simd_data_fmt + self.simd_arith
        ret_int = self.inst_total - ret_simd
        return {
            "cycles":           int(self.t_clk_ec),
            "cycles_opt":       int(self.t_clk_bc),
            "empty":            int(stalls + lost),
            "stalls":           int(stalls),
            "lost":             int(lost),
            "lost_other":       int(lost_other),
            "bad_spec":         int(bad_spec),
            "stall_be":         int(stall_be),
            "stall_l1d":        int(self.dc_stalls),
            "stall_be_core":    int(self.all_hazards + self.all_div_stalls),
            "stall_fe":         int(stall_fe),
            "stall_l1i":        int(self.ic_stalls),
            "stall_fe_core":    int(self.j_stalls),
            "ret":              int(self.inst_total),
            "ret_simd":         int(ret_simd),
            "ret_int":          int(ret_int),
            "ret_ctrl_flow_br": int(self.b_inst),
            "bp_miss":          int(self.bp_stats['mispred']),
            "l1i_ref":          int(self.ic_stats['references']),
            "l1i_miss":         int(sum(self.ic_stats['misses'].values())),
            "l1d_ref":          int(self.dc_stats['references']),
            "l1d_miss":         int(sum(self.dc_stats['misses'].values())),
        }

    def save_as_json(self) -> None:
        p = os.path.join(
            os.path.dirname(self.inst_profile), "perf_est.json"
        )
        with open(p, "w") as f:
            json.dump({"core": self._core_dict()}, f, indent=4)
        print(f"\nSaved performance estimation as JSON to {p}")

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

def parse_args(argv=None):
    parser = argparse.ArgumentParser(description="Performance estimates based on the ISA sim results and the microarchitectural description.")
    parser.add_argument("inst_profile", help="Path to 'inst_profile.json' for the given workload")
    parser.add_argument("hw_stats", help="Path to 'hw_stats.json' for the given workload")
    parser.add_argument("rf_trace", help="Path to 'rf_trace.bin' for depndency/hazard analysis")
    parser.add_argument("--hw", "--hw_perf_metrics", help="Path to 'hw_perf_metrics.yaml' for the given hardware configuration", default=DEFAULT_HW_YAML)
    parser.add_argument("-c", "--corr", type=str, default=None, help="Path to RTL hw_stats JSON for cycles range and per-metric correlation analysis")
    parser.add_argument("-s", "--silent", action="store_true", help="Suppress all output to stdout. Requires running with -j/--save_json or -c/--corr")
    parser.add_argument("--plot", action="store_true", help="Show plots. Applicable only for correlation runs")
    parser.add_argument("-p", "--places", type=int, default=2, help="Number of decimal places for formatted output (default: 2)")
    parser.add_argument("-j", "--save_json", action="store_true", help="Save performance estimates as JSON (tda.py-compatible)")
    parser.add_argument("--save_corr_csv", action="store_true", help="Save correlation stats as csv")
    parser.add_argument("--save_corr_png", action="store_true", help="Save correlation plots as png")
    parser.add_argument("--save_corr_svg", action="store_true", help="Save correlation plots as svg")
    return parser.parse_args(argv)

def main(args: argparse.Namespace):
    global FMT, FMT_T
    FMT = smarter_eng_formatter(unit='', places=args.places, sep="")
    FMT_T = smarter_eng_formatter(unit='s', places=args.places, sep="")
    p_inst = args.inst_profile
    p_hws = args.hw_stats
    p_met = args.hw
    p_rft = args.rf_trace
    p_corr = args.corr
    paths = [p_inst, p_hws, p_met]
    if p_rft:
        paths.append(p_rft)
    if p_corr:
        paths.append(p_corr)

    valid_corr_run = args.corr and (
        args.save_corr_csv or \
        args.plot or args.save_corr_png or args.save_corr_svg
    )
    if args.silent and not (args.save_json or valid_corr_run):
        raise ValueError(
            "--silent without --save_json or --corr with at least one output" \
            " has nothing to do"
        )

    for p in paths:
        if not os.path.isfile(p):
            raise ValueError(f"File {p} not found")

    res = perf(p_inst, p_hws, p_met, p_rft)

    if not args.silent:
        print(res)

    if args.save_json:
        res.save_as_json()

    if p_corr:
        with open(p_corr, 'r') as f:
            hw_stats_rtl = json.load(f)
        rtl_core = hw_stats_rtl['core']
        corr = rtl_core['cycles']

        # --- Per-metric correlation ---
        est_core = res._core_dict()

        comp = []
        for k, e in est_core.items():
            if k not in rtl_core:
                continue
            r = rtl_core[k]
            diff = e - r
            dn = e if e else r # denominator
            diff_p = round(diff / dn * 100, 3) if dn else 0
            comp.append([k, e, r, diff, diff_p])

        dfc = pd.DataFrame(
            comp, columns=["metric", "est", "rtl", "diff", "diff%"]
        )

        if not args.silent:
            print("\nCorrelation:")
            print(dfc.to_string(index=False), "\n")

        if args.save_corr_csv:
            p = os.path.join(os.path.dirname(p_inst), "correlation.csv")
            dfc.to_csv(p, index=False)
            print(f"Saved correlation stats as CSV to {p}")

        if not (args.plot or args.save_corr_png or args.save_corr_svg):
            return

        import matplotlib.pyplot as plt
        testname = get_test_title(p_corr)

        # --- Cycles correlation range plot ---
        clk_best = res.est["best"]["clk"]
        clk_exp = res.est["exp"]["clk"]

        fig, ax = plt.subplots(figsize=(8, 2))
        xmin = min(clk_best, corr)
        xmax = max(clk_exp, corr)
        xrange = xmax - xmin
        ax.set_xlim(xmin - xrange*.1, xmax + xrange*.1)

        limx = ax.get_xlim()
        # range bar
        ax.hlines(0, limx[0], limx[1],
                  color='lightgray', linewidth=15, label='Estimate range')

        # add markers
        ax.vlines(clk_exp, -0.1, 0.1,
                  color='k', linewidth=2, label='Expected')
        ax.vlines(clk_best, -0.1, 0.1,
                  color='tab:green', linewidth=2, label='Best')
        ax.vlines(corr, -0.15, 0.15,
                  color='tab:blue', linewidth=2, label='Achieved')

        # annotate
        ax.text(clk_exp, 0.13, f'Expected:\n{FMT(clk_exp)}',
                ha='center', color='k')
        ax.text(clk_best, 0.13, f'Best:\n{FMT(clk_best)}',
                ha='center', color='tab:green')
        ax.text(corr, -0.37, f'Achieved:\n{FMT(corr)}',
                ha='center', color='tab:blue')

        # style
        ax.margins(0, 0)
        ax.set_ylim(-0.42, 0.42)
        ax.set_yticks([])
        ax.grid(axis='x', linestyle='--', alpha=0.5)
        ax.set_xlabel('Cycles')
        ax.xaxis.set_major_formatter(FMT)
        #ax.legend(loc='upper right', bbox_to_anchor=(1.35, 1))
        ax.set_title(
            f"Correlation for '{testname}' - Estimated vs Achieved Cycles")
        fig.tight_layout()

        # --- Per-metric correlation plot ---
        fig2, ax2 = plt.subplots(figsize=(8, 6))
        rect = ax2.bar(dfc['metric'], dfc['diff%'].round(2))
        ax2.bar_label(rect, padding=4, size=8)
        ax2.tick_params(axis='x', labelrotation=90)
        ax2.grid(which='major', axis='y')
        ax2.set_title(
            f"Correlation for '{testname}' - Per metric difference [%]")
        fig2.tight_layout()
        ax2.margins(0.02, 0.08)

        plot_args = [("png", args.save_corr_png), ("svg", args.save_corr_svg)]
        for ext, do_save in plot_args:
            if not do_save:
                continue
            pairs = [(fig, "correlation_cycles"), (fig2, "correlation_metrics")]
            for fig_obj, stem in pairs:
                p = os.path.join(os.path.dirname(p_inst), f"{stem}.{ext}")
                fig_obj.savefig(p)
                print(f"Saved correlation plot as {ext.upper()} to {p}")

        if not args.plot:
            plt.close('all')
        else:
            plt.show()

if __name__ == "__main__":
    main(parse_args())
