
import os
import subprocess

import matplotlib
from matplotlib.ticker import EngFormatter, FuncFormatter

INDENT = " " * 4
DELIM = f"\n{INDENT}"

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
        except ValueError:
            return s
        # use extra precision if < 10 after scaling
        s = more_fmt(x, pos) if abs(val) < 10 and val != 0 else s
        # strip trailing .0 or zeros
        return s.rstrip('0').rstrip('.') if '.' in s else s

    return FuncFormatter(_fmt)

FMT_AXIS = EngFormatter(unit='', places=0, sep='')
