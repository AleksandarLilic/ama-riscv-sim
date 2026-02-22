
import json
import os
import re
import subprocess

import matplotlib
from matplotlib.ticker import EngFormatter, FuncFormatter

INDENT = " " * 4
DELIM = f"\n{INDENT}"
SIM_PASS_STRING = "    0x051e tohost       : 0x00000001"
SIM_EARLY_EXIT_STRING = SIM_PASS_STRING.replace("0x00000001", "0xf0000000")

def is_notebook():
    try:
        from IPython import get_ipython
        return get_ipython() is not None
    except ImportError:
        return False

def get_reporoot():
    try:
        repo_root = subprocess.check_output(
            ["git", "rev-parse", "--show-toplevel"],
            stderr=subprocess.STDOUT
        ).strip().decode("utf-8")
        return repo_root
    except subprocess.CalledProcessError as e:
        print(f"Error: {e.output.decode('utf-8')}")
        raise e

def is_headless():
    """Return True if we're running without a GUI/display."""
    # 1) on UNIX, no $DISPLAY means no X11 display.
    if os.name == "posix" and not os.environ.get("DISPLAY"):
        return True
    # 2) if plt is using a non‐interactive backend (agg/pdf/svg, etc.)
    if matplotlib.get_backend().lower().startswith(("agg", "pdf", "svg", "ps")):
        return True

    return False

def get_test_title(input_log: str) -> str:
    """Generate a plot title based on the input log filename."""
    return os.path.basename(os.path.dirname(input_log)).replace("out_", "")

def smarter_eng_formatter(places=1, unit='', sep=''):
    base_fmt = EngFormatter(unit=unit, places=places, sep=sep)
    more_fmt = EngFormatter(unit=unit, places=places + 1, sep=sep)

    def _fmt(x, pos):
        s = base_fmt(x, pos)
        try:
            val = float(s.split(".")[0])
            #val = float(s.split(" ")[0].split(".")[0])
        except ValueError:
            return s
        # use extra precision if < 10 after scaling
        s = more_fmt(x, pos) if abs(val) < 10 and val != 0 else s
        # strip trailing .0 or zeros
        #val_p = s.split(" ")
        #val_n = val_p[0] # numeric part
        #val_s = val_p[1] if len(val_p) > 1 else '' # suffix part
        #val_n = val_n.rstrip('0').rstrip('.') if '.' in val_n else val_n
        #s = f"{val_n} {val_s}".strip()
        #return s
        return s.rstrip('0').rstrip('.') if '.' in s else s

    return FuncFormatter(_fmt)

FMT_AXIS = EngFormatter(unit='', places=0, sep='')

def reformat_json(json_data, max_depth=2, indent=4):
    dumped_json = json.dumps(json_data, indent=indent)
    deeper_indent = " " * (indent * (max_depth + 1))
    parent_indent = " " * (indent * max_depth)
    out_json = re.sub(rf"\n{deeper_indent}", " ", dumped_json)
    out_json = re.sub(rf"\n{parent_indent}}},", " },", out_json)
    out_json = re.sub(rf"\n{parent_indent}}}", " }", out_json)

    # replace all consecutive whitespaces NOT at beginning of line
    def normalize_internal_whitespace(txt):
        lines = txt.splitlines(keepends=True) # keep \n at the end of each line
        processed = []
        for line in lines:
            # preserve leading whitespace (spaces or tabs)
            match = re.match(r'^(\s*)', line)
            leading = match.group(1)
            rest = line[len(leading):]
            # replace consecutive whitespace in the rest of the line
            rest = re.sub(r'\s{2,}', ' ', rest)
            processed.append(leading + rest)

        return ''.join(processed)

    out_json = normalize_internal_whitespace(out_json)

    return out_json
