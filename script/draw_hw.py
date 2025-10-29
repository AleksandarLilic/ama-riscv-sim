#!/usr/bin/env python3

import argparse
import json
import os

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from hw_model_sweep import get_bp_size
from run_analysis import CACHE_LINE_BYTES, MEM_SIZE

ADDR_BITS = int(np.log2(MEM_SIZE))
CACHE_BYTE_ADDR_BITS = int(np.log2(CACHE_LINE_BYTES))

# common
def center_cell_text(table, fontsize=9):
    table.auto_set_font_size(False)
    table.set_fontsize(fontsize)
    for cell in table.get_celld().values():
        cell.set_text_props(ha='center', va='center')

# caches
def get_cache_fig_w(all_col_bits):
    cols_sum = sum(all_col_bits)
    cols_len = len(all_col_bits)
    if cols_sum <= 6 and cols_len == 3:
        return cols_sum/1.3
    return cols_sum/1.5

def draw_cache(sets, ways, res_dict, wr_pol=""):
    fig_w = ways*2.5
    fig_h = 0.1 + sets/2.5
    _, axs = plt.subplots(nrows=1, ncols=ways, figsize=(fig_w,fig_h))

    sets_bits = int(np.log2(sets))
    tags_bits = ADDR_BITS - sets_bits - CACHE_BYTE_ADDR_BITS

    col_widths = {0: 0.1, 1: 0.2, 2: 0.7}  # total = 1.0
    df2 = pd.DataFrame(
        [['v', f"{tags_bits}b", "64B"]],
        index=range(sets),
        columns=['v', 'tag', 'data']
    )

    if wr_pol == "wb":
        # insert dirty bit column after valid bit
        df2.insert(1, 'd', ['d']*sets)
        col_widths = {0: 0.1, 1: 0.1, 2: 0.2, 3: 0.6}

    axs = np.array(axs) if ways == 1 else axs
    for i,ax in enumerate(axs.flatten()): # L-R, T-B
        table = ax.table(
            cellText=df2.values,
            loc='center',
            colLabels=df2.columns,
            rowLabels=[f" {i} " for i in df2.index],
            rowLoc='center',
            colLoc='center',
            # [xmin, ymin, width, height]
            bbox=[0, 0, 1, 1]
        )
        ax.axis('off')

        for (row, col), cell in table.get_celld().items():
            cell.set_width(col_widths.get(col, 0.5)) # default fallback

        center_cell_text(table)

        if i == 0: # set the title, and add the address block
            left_anchor = -1.1

            txt_str = f"{args.block}: Sets: {sets}, Ways: {ways}, " + \
                      f"Size: {res_dict['size']} B"
            if res_dict['hr'] != None:
                txt_str += f", Hit Rate: {res_dict['hr']}%"
            ax.text(left_anchor, 1 + 0.7/sets, txt_str,
                    ha='left', va='bottom', fontsize=11)

            # address block
            #col_total = tags_bits + sets_bits + CACHE_BYTE_ADDR_BITS
            if sets_bits > 0:
                addr_cols = ['tag', 'index', 'byte']
                val_cols = [tags_bits, sets_bits, CACHE_BYTE_ADDR_BITS]
                #col_widths = {
                #    0: round(tags_bits/col_total,2),
                #    1: round(sets_bits/col_total,2),
                #    2: round(CACHE_BYTE_ADDR_BITS/col_total,2)
                #}
            else:
                addr_cols = ['tag', 'byte']
                val_cols = [tags_bits, CACHE_BYTE_ADDR_BITS]
                #col_widths = {
                #    0: round(tags_bits/col_total,2),
                #    1: round(CACHE_BYTE_ADDR_BITS/col_total,2)
                #}

            dfa = pd.DataFrame(
                [[f"{bits}b" for bits in val_cols]],
                index=[1], columns=addr_cols)

            h = 1.3/sets
            addr_table = ax.table(
                cellText=dfa.values,
                loc='center',
                colLabels=dfa.columns,
                rowLabels=None,
                rowLoc='center',
                colLoc='center',
                # [xmin, ymin, width, height]
                bbox=[left_anchor, 1-h, .9, h]
            )

            #for (row, col), cell in addr_table.get_celld().items():
            #    cell.set_width(col_widths.get(col, 0.5)) # default fallback
            ax.text(-.65, 1+ .1/sets, "Address",
                    ha='center', va='bottom', fontsize=10)
            center_cell_text(addr_table)

    plt.show()

# branch predictors
def bp_combined_to_components(params):
    bp1c = {}
    bp2c = {}
    bpc = {}
    for k,v in params.items():
        if k.startswith("bp_combined"):
            bpc[k.replace("bp_combined_", "bp_")] = v
        elif k.startswith("bp2_"):
            bp2c[k.replace("bp2_", "bp_")] = v
        elif k.startswith("bp_"):
            bp1c[k] = v
        elif k in ['acc', 'size']:
            # acc and size, save only to combined
            bpc[k] = v
            bp2c[k] = None
            bp1c[k] = None
    d3 = {
        "bp_combined" : bpc,
        f"bp_{params['bp']}" : bp1c,
        f"bp_{params['bp2']}" : bp2c,
    }

    return d3

def rotate_left(lst, n):
    n = n % len(lst) # handle cases where n > len(lst)
    return lst[n:] + lst[:n]

def get_bp_row_entries(bp_name, col_bits):
    if bp_name == "bp_static":
        return [1] # placeholder
    if bp_name == "bp_local":
        return [1, 2**col_bits[0], 2**col_bits[1]]
    if bp_name == "bp_gshare":
        return [2, 1, 2**col_bits[1]] # raise to the pow of max(pc, gr)
    if bp_name == "bp_gselect":
        return [2, 1, 2**col_bits[1]] # raise to the pow of sum(pc, gr)
    if bp_name in ["bp_bimodal", "bp_global", "bp_combined"]:
        return [1, 2**col_bits[0]]

    raise ValueError(f"Unsupported bp_name for row entries: {bp_name}")

def get_bp_all_col_bits(bp_name, params):
    if bp_name == "bp_static":
        return [1] # placeholder
    if bp_name in ["bp_bimodal", "bp_combined"]:
        return [int(params['bp_pc_bits']), int(params['bp_cnt_bits'])]
    if bp_name == "bp_gshare":
        return [
            max(int(params['bp_pc_bits']), int(params['bp_gr_bits'])),
            max(int(params['bp_pc_bits']), int(params['bp_gr_bits'])),
            int(params['bp_cnt_bits'])
        ]
    if bp_name == "bp_gselect":
        return [
            max(int(params['bp_pc_bits']), int(params['bp_gr_bits'])),
            (int(params['bp_pc_bits']) + int(params['bp_gr_bits'])),
            int(params['bp_cnt_bits'])
        ]
    if bp_name == "bp_global":
        return [int(params['bp_gr_bits']), int(params['bp_cnt_bits'])]
    if bp_name == "bp_local":
        return [
            int(params['bp_pc_bits']),
            int(params['bp_lhist_bits']),
            int(params['bp_cnt_bits'])
        ]

    raise ValueError(f"Unsupported bp_name for all col bits: {bp_name}")


def get_bp_table_names(bp_name):
    if bp_name == "bp_static":
        return [""] # placeholder
    if bp_name == "bp_gselect" or bp_name == "bp_gshare":
        return ["", "IDX", "CNT"]
    if bp_name in ["bp_bimodal", "bp_global", "bp_combined"]:
        return ["IDX", "CNT"]
    if bp_name == "bp_local":
        return ["PC", "HIST", "CNT"]

    raise ValueError(f"Unsupported bp_name for table names: {bp_name}")

STR_INDEXES = []
# presumably, no more than 16 indexes are required, increase as needed
for i in range(17, -1, -1):
    STR_INDEXES.append(f"[{i}]")

BP_ROW_LABELS = {
    "bp_static": [None, None, None], # placeholder
    "bp_bimodal": [["PC"], None, None],
    "bp_gshare":  [["PC", "GR"], ["PC⊕GR"], None],
    "bp_gselect": [["PC", "GR"], ["{PC,GR}"], None],
    "bp_global": [["GR"], None, None],
    "bp_local": [["PC"], None, None],
    "bp_combined": [["PC"], None, None],
}

def get_bp_fig_h(rows_max):
    if rows_max <= 4:
        return rows_max/2.5
    elif rows_max <= 8:
        return rows_max/3
    else:
        return rows_max/4

def get_bp_fig_w(all_col_bits):
    cols_sum = sum(all_col_bits)
    cols_len = len(all_col_bits)
    if cols_sum <= 6 and cols_len == 3:
        return cols_sum/1.3
    return cols_sum/1.5

def get_bp_pc_gr_strings(params):
    pc_bits = int(params['bp_pc_bits'])
    gr_bits = int(params['bp_gr_bits'])

    max_bits = max(pc_bits, gr_bits)
    max_strs = [f"" for b in range(max_bits)]

    pc_all_strs = max_strs[0:max_bits-pc_bits] + STR_INDEXES[-pc_bits:]
    gr_all_strs = max_strs[0:max_bits-gr_bits] + STR_INDEXES[-gr_bits:]
    return pc_all_strs, gr_all_strs

def get_bp_idx_strings(params, bp_name):
    pc_bits = int(params['bp_pc_bits'])
    gr_bits = int(params['bp_gr_bits'])

    max_bits = max(pc_bits, gr_bits)
    max_strs = [f"" for b in range(max_bits)]

    pc_all_strs = max_strs[0:max_bits-pc_bits] + STR_INDEXES[-pc_bits:]
    gr_all_strs = max_strs[0:max_bits-gr_bits] + STR_INDEXES[-gr_bits:]

    if bp_name == "bp_gselect":
        out_str = [f"PC{s}" for s in pc_all_strs] + \
                  [f"GR{s}" for s in gr_all_strs]
        out_str = [o for o in out_str if '[' in o]
        return out_str

    if bp_name == "bp_gshare":
        # GR xor'd with top PC bits if not the same length
        out_str = [
            f"PC{pcs}⊕GR{grs}"
            for pcs,grs
            in zip(pc_all_strs,rotate_left(gr_all_strs, max_bits-gr_bits))
        ]
        out_str = [o.replace('PC⊕','') for o in out_str]
        out_str = [o.replace('⊕GR','') if o.endswith('⊕GR') else o
                   for o in out_str]
        out_str = [o.replace('⊕', '\n⊕\n') for o in out_str]
        return out_str

    raise ValueError(f"Unsupported bp_name for idx strings: {bp_name}")

def get_max_fig_height(fig, txt, ax):
    renderer = fig.canvas.get_renderer()
    bb = txt.get_window_extent(renderer=renderer)
    inv_axes = ax.transAxes.inverted()
    x_axes, y_axes = inv_axes.transform((bb.x0, bb.y1))
    return y_axes

def draw_predictor(bp_name, params, title_name=""):
    all_col_bits = get_bp_all_col_bits(bp_name, params)
    row_entries = get_bp_row_entries(bp_name, all_col_bits)
    rows_max = max(row_entries)
    bbox_y = [x/rows_max for x in row_entries]
    w_watios = np.array(all_col_bits) #+1 # account for the row labels
    table_names = get_bp_table_names(bp_name)

    max_y = 0
    fig_h = get_bp_fig_h(rows_max)
    fig_w = get_bp_fig_w(all_col_bits)
    fig, axs = plt.subplots(
        nrows=1,
        ncols=len(row_entries),
        figsize=(fig_w,fig_h),
        width_ratios=w_watios)

    axs = np.array([axs]) if bp_name == "bp_static" else axs # placeholder
    for idx,ax in enumerate(axs):
        if bp_name == "bp_static": # don't actually run for static
            break

        y_size = bbox_y[idx]
        #y_midpoint = (bbox_y[idx])/2
        h_offset = min(row_entries) / max(row_entries)

        if bp_name in ['bp_gselect', 'bp_gshare'] and idx <= 1:
            # show and label gr and pc bits
            if idx == 0:
                # registers (GR/PC)
                pc_strs, gr_strs = (get_bp_pc_gr_strings(params))
                df = pd.DataFrame(
                    [pc_strs, gr_strs],
                    index=range(row_entries[idx]),
                    columns=STR_INDEXES[-all_col_bits[idx]:]
                )

            elif idx == 1:
                # how they form the index
                idx_strs = (get_bp_idx_strings(params, bp_name))
                df = pd.DataFrame(
                    [idx_strs],
                    index=range(row_entries[idx]),
                    columns=STR_INDEXES[-all_col_bits[idx]:]
                )
                if bp_name == 'bp_gshare':
                    y_size *= 4 # make room to fit the XORing of the bits

        else: # not gselect or gshare, no special labeling needed
            df = pd.DataFrame(
                '',
                index=range(row_entries[idx]),
                columns=STR_INDEXES[-all_col_bits[idx]:]
            )


        row_labels = BP_ROW_LABELS[bp_name][idx]
        if row_labels == None:
            row_labels = [f" {i} " for i in df.index]
        #row_labels == None

        col_labels = df.columns if table_names[idx] != "" else None
        #col_labels = df.columns \
        #             if table_names[idx] != "" else [f"" for i in df.columns]
        table = ax.table(
            cellText=df.values,
            loc='center',
            colLabels=col_labels,
            rowLabels=row_labels,
            rowLoc='center',
            colLoc='center',
            #textprops={'fontsize':20},
            # [xmin, ymin, width, height]
            #bbox=[0, .5-y_midpoint, 1, y_size+h_offset])
            bbox=[0, 1-y_size-h_offset, 1, y_size+h_offset]
        )

        center_cell_text(table)

        #for key, cell in table.get_celld().items():
        #    cell.set_height(cell.get_height()*4)

        #txt = ax.text(0.5, .5+y_midpoint+h_offset+.005, table_names[idx],
        #              ha='center', va='bottom', fontsize=10)
        txt = ax.text(
            0.5, 1.005, table_names[idx], ha='center', va='bottom', fontsize=10)
        # get max height to align the figure title later
        max_y = max(max_y, get_max_fig_height(fig, txt, ax))

        # hide axes
        ax.axis('off')
        #ax.axis('tight')
        #fig.patch.set_visible(False)

    title = title_name if title_name != "" else bp_name
    txt_str = f"BP: {title}\n"

    if bp_name != "bp_static":
        txt_str += f"F: {params['bp_fold_pc']}"
        fig.tight_layout()
    else:
        txt_str += f"M: {params['bp_static_method']}"
        ax.axis('off')

    # components of the combined predictor don't have acc and size specified
    if params['acc'] != None:
        txt_str += f", A: {params['acc']:.1f}%"
    if params['size'] != None:
        txt_str += f", S: {params['size']} B"
    txt_str += "\n"
    # static has no fold, so 2nd line starts with ", ", fix it
    txt_str = txt_str.replace("\n, ", "\n")

    axs[0].text(0, max_y, txt_str, ha='left', va='bottom', fontsize=11)

    return fig, axs

# parse args
DEFAULTS = {
    # caches
    "cache_sets": 2, "cache_ways": 2,
    "cache_policy": "lru", "dcache_wr_policy": "wb",
    # bp
    "bp": "gshare",
    "bp_static_method": "btfn", "bp_pc_bits": 5, "bp_cnt_bits": 2,
    "bp_lhist_bits": 5, "bp_gr_bits": 5, "bp_fold_pc": "none",
    "bp2": "none",
    "bp2_static_method": "at", "bp2_pc_bits": 5, "bp2_cnt_bits": 2,
    "bp2_lhist_bits": 5, "bp2_gr_bits": 5, "bp2_fold_pc": "none",
    "bp_combined_pc_bits": 6,
    "bp_combined_cnt_bits": 3,
    "bp_combined_fold_pc": "none",
}

CHOICES = {
    "block": ["icache", "dcache", "bpred"],
    "cache_policy": ["lru"],
    "dcache_wr_policy": ["wb", "wt"], # FIXME: currently missing in json results
    "bp_method": ["btfn", "ant", "at"],
    "bp_type": ["none", "ideal", "static",
                "bimodal", "gselect", "global", "gshare", "local"],
    "fold_pc": ["all", "none"],
}

MAX_BLOCKS = 10

def parse_args() -> argparse.Namespace:

    def power_of_two(value_str: str) -> int:
        value = int(value_str)
        if value <= 0 or (value & (value - 1)) != 0:
            raise argparse.ArgumentTypeError(f"{value} is not a power of 2")
        return value

    parser = argparse.ArgumentParser(description="Cache and branch predictor configuration")
    parser.add_argument("block", choices=CHOICES['block'], metavar="BLOCK",
                        help="Hardware block to draw")
    parser.add_argument("--config", type=str, metavar="JSON_FILE",
                        help="Path to JSON config file. If provided, all other config switches are ignored")
    # common
    parser.add_argument('--max_blocks', type=int, default=MAX_BLOCKS, help=F"Limit the number of hardware blocks to draw. Default is {MAX_BLOCKS}")

    # caches
    parser.add_argument("--cache_sets", type=power_of_two, default=DEFAULTS["cache_sets"], metavar="N",
                        help=f"Number of sets in the cache. Must be power of 2 (default: {DEFAULTS['cache_sets']})")
    parser.add_argument("--cache_ways", type=int, default=DEFAULTS["cache_ways"], metavar="N",
                        help=f"Number of ways in the cache (default: {DEFAULTS['cache_ways']})")
    parser.add_argument("--cache_policy", type=str, choices=CHOICES["cache_policy"], default=DEFAULTS["cache_policy"], metavar="POLICY",
                        help=f"Cache replacement policy.\nOptions: {', '.join(CHOICES['cache_policy'])} (default: {DEFAULTS['cache_policy']})")
    parser.add_argument("--dcache_wr_policy", type=str, choices=CHOICES["dcache_wr_policy"], default=DEFAULTS["dcache_wr_policy"], metavar="WR_POLICY",
                        help=f"Cache write policy (dcache only).\nOptions: {', '.join(CHOICES['dcache_wr_policy'])} (default: {DEFAULTS['dcache_wr_policy']})")

    # bp
    parser.add_argument("--bp", type=str, choices=CHOICES["bp_type"], default=DEFAULTS["bp"], metavar="PREDICTOR",
                        help=f"First branch predictor. Defaults as active, driving the I$. For combined predictor, counters will be weakly biased towards this predictor (impacts warm-up period)\nOptions: {', '.join(CHOICES['bp_type'])} (default: {DEFAULTS['bp']})")
    parser.add_argument("--bp2", type=str, choices=CHOICES["bp_type"], default=DEFAULTS["bp2"], metavar="PREDICTOR",
                        help=f"Second branch predictor, only for combined predictor. If provided, combined predictor becomes active\nOptions: {', '.join(CHOICES['bp_type'])} (default: {DEFAULTS['bp2']})")
    parser.add_argument("--bp_static_method", type=str, choices=CHOICES["bp_method"], default=DEFAULTS["bp_static_method"], metavar="METHOD",
                        help=f"Static predictor - method.\nOptions: {', '.join(CHOICES['bp_method'])} (default: {DEFAULTS['bp_static_method']})")
    parser.add_argument("--bp_pc_bits", type=int, default=DEFAULTS["bp_pc_bits"], metavar="N",
                        help=f"Branch predictor - PC bits (default: {DEFAULTS['bp_pc_bits']})")
    parser.add_argument("--bp_cnt_bits", type=int, default=DEFAULTS["bp_cnt_bits"], metavar="N",
                        help=f"Branch predictor - counter bits (default: {DEFAULTS['bp_cnt_bits']})")
    parser.add_argument("--bp_lhist_bits", type=int, default=DEFAULTS["bp_lhist_bits"], metavar="N",
                        help=f"Branch predictor - local history bits (default: {DEFAULTS['bp_lhist_bits']})")
    parser.add_argument("--bp_gr_bits", type=int, default=DEFAULTS["bp_gr_bits"], metavar="N",
                        help=f"Branch predictor - global register bits (default: {DEFAULTS['bp_gr_bits']})")
    parser.add_argument("--bp_fold_pc", type=str, choices=CHOICES["fold_pc"], default=DEFAULTS["bp_fold_pc"], metavar="FOLD",
                        help=f"Branch predictor - Fold higher order PC bits for indexing\nOptions: {', '.join(CHOICES['fold_pc'])} (default: {DEFAULTS['bp_fold_pc']})")
    parser.add_argument("--bp2_static_method", type=str, choices=CHOICES["bp_method"], default=DEFAULTS["bp2_static_method"], metavar="METHOD",
                        help=f"Static predictor - method.\nOptions: {', '.join(CHOICES['bp_method'])} (default: {DEFAULTS['bp2_static_method']})")
    parser.add_argument("--bp2_pc_bits", type=int, default=DEFAULTS["bp2_pc_bits"], metavar="N",
                        help=f"Branch predictor 2 - PC bits (default: {DEFAULTS['bp2_pc_bits']})")
    parser.add_argument("--bp2_cnt_bits", type=int, default=DEFAULTS["bp2_cnt_bits"], metavar="N",
                        help=f"Branch predictor 2 - counter bits (default: {DEFAULTS['bp2_cnt_bits']})")
    parser.add_argument("--bp2_lhist_bits", type=int, default=DEFAULTS["bp2_lhist_bits"], metavar="N",
                        help=f"Branch predictor 2 - local history bits (default: {DEFAULTS['bp2_lhist_bits']})")
    parser.add_argument("--bp2_gr_bits", type=int, default=DEFAULTS["bp2_gr_bits"], metavar="N",
                        help=f"Branch predictor 2 - global register bits (default: {DEFAULTS['bp2_gr_bits']})")
    parser.add_argument("--bp2_fold_pc", type=str, choices=CHOICES["fold_pc"], default=DEFAULTS["bp2_fold_pc"], metavar="FOLD",
                        help=f"Branch predictor 2 - Fold higher order PC bits for indexing\nOptions: {', '.join(CHOICES['fold_pc'])} (default: {DEFAULTS['bp2_fold_pc']})")
    parser.add_argument("--bp_combined_pc_bits", type=int, default=DEFAULTS["bp_combined_pc_bits"], metavar="N",
                        help=f"Combined predictor - PC bits (default: {DEFAULTS['bp_combined_pc_bits']})")
    parser.add_argument("--bp_combined_cnt_bits", type=int, default=DEFAULTS["bp_combined_cnt_bits"], metavar="N",
                        help=f"Combined predictor - counter bits (default: {DEFAULTS['bp_combined_cnt_bits']})")
    parser.add_argument("--bp_combined_fold_pc", type=str, choices=CHOICES["fold_pc"], default=DEFAULTS["bp_combined_fold_pc"], metavar="FOLD",
                        help=f"Combined predictor - Fold higher order PC bits for indexing\nOptions: {', '.join(CHOICES['fold_pc'])} (default: {DEFAULTS['bp_combined_fold_pc']})")

    return parser.parse_args()

def convert_switches_to_dict(args):
    config = {}
    if args.block in CHOICES['block'][0:2]: # caches
        size = args.cache_sets * args.cache_ways * CACHE_LINE_BYTES
        config = {
            args.cache_policy : {
                args.cache_sets : {
                    args.cache_ways : { "size": size, "hr": None }
                }
            }
        }

    elif args.block == CHOICES['block'][2]: # bp
        # common for all
        bp_entry = {
                "static_method": args.bp_static_method,
                "pc_bits": args.bp_pc_bits,
                "gr_bits": args.bp_gr_bits,
                "lhist_bits": args.bp_lhist_bits,
                "cnt_bits": args.bp_cnt_bits,
                "fold_pc": args.bp_fold_pc,
            }
        bp_size = get_bp_size(args.bp, bp_entry)
        entry = {f"bp_{k}": v for k, v in bp_entry.items()}
        bp_act = f"bp_{args.bp}"

        if args.bp2 != "none":
            # it's a combined predictor
            bp_act = f"bp_combined-{args.bp}_{args.bp2}"

            bp2_entry = {
                "static_method": args.bp2_static_method,
                "pc_bits": args.bp2_pc_bits,
                "gr_bits": args.bp2_gr_bits,
                "lhist_bits": args.bp2_lhist_bits,
                "cnt_bits": args.bp2_cnt_bits,
                "fold_pc": args.bp2_fold_pc,
            }
            bp_size += get_bp_size(args.bp2, bp2_entry)
            entry2 = {f"bp2_{k}": v for k, v in bp2_entry.items()}

            bpc_entry = {
                "pc_bits": args.bp2_pc_bits,
                "cnt_bits": args.bp2_cnt_bits,
                "fold_pc": args.bp2_fold_pc,
            }
            bp_size += get_bp_size("bp_combined", bpc_entry)
            entryc = {f"bp_combined_{k}": v for k, v in bpc_entry.items()}

            entry = {**entry, **entry2, **entryc}
            # bp_ not needed in this case, bp_combined_to_components handles it
            entry['bp'] = f"{args.bp}"
            entry['bp2'] = f"{args.bp2}"

        # again, common for both
        entry['size'] = bp_size
        entry['acc'] = None
        config = { bp_act: {1: entry}} # 1 for placeholder bin size

    else:
        raise ValueError(f"Unknown block type and impossible to reach")

    return config

def run_main(args) -> None:
    using_switches = (args.config == None)

    if using_switches:
        config = convert_switches_to_dict(args)
    else:
        if not os.path.exists(args.config):
            raise FileNotFoundError(f"Config '{args.config}' not found")
        with open(args.config, "r") as flavor:
            config = json.load(flavor)

    block_cnt = 0
    hit_limit = False
    if args.block in CHOICES['block'][0:2]:
        # only 'LRU' policy currently supported,
        # needs to also be in a loop if more are added
        wr_pol = ""
        if args.block == CHOICES['block'][1]:
            wr_pol = args.dcache_wr_policy
        lru_res = config['lru']
        for k_sets, v_ways in lru_res.items():
            for k_ways, v_res in v_ways.items():
                draw_cache(int(k_sets), int(k_ways), v_res, wr_pol)
                block_cnt += 1
                if block_cnt > args.max_blocks:
                    hit_limit = True
                    print(f"Max blocks limit ({args.max_blocks}) reached")
                    break # sets

            if hit_limit:
                break # ways

    elif args.block == CHOICES['block'][2]:
        for bp_name, bp_results in config.items():
            bpc_cnt = 0
            for size_bin, params in bp_results.items():
                if "combined" in bp_name:
                    all3_parts = bp_combined_to_components(params)
                    bpc_cnt += 1
                    for c_bp_name, c_results in all3_parts.items():
                        title = f"{bp_name} ({bpc_cnt})\npart: {c_bp_name}"
                        draw_predictor(c_bp_name, c_results, title)
                else:
                    draw_predictor(bp_name, params)

                block_cnt += 1
                if block_cnt > args.max_blocks:
                    hit_limit = True
                    print(f"Max blocks limit ({args.max_blocks}) reached")
                    break # results of this predictor

            if hit_limit:
                break # all predictors

    else:
        raise ValueError("Unknown block type and impossible to reach")

if __name__ == "__main__":
    args = parse_args()
    run_main(args)
