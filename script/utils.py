
import os
import subprocess

import matplotlib
import matplotlib.pyplot as plt


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
        return None

def is_headless():
    """Return True if we're running without a GUI/display."""
    # 1) on UNIX, no $DISPLAY means no X11 display.
    if os.name == "posix" and not os.environ.get("DISPLAY"):
        return True
    # 2) if plt is using a non‐interactive backend (agg/pdf/svg, etc.)
    if matplotlib.get_backend().lower().startswith(("agg", "pdf", "svg", "ps")):
        return True

    return False
