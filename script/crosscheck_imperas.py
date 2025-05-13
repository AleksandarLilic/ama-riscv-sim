#!/usr/bin/env python3

import os
import sys
import subprocess
import glob
from typing import Dict, Any, List
from itertools import islice
from elftools.elf.elffile import ELFFile
from utils import get_reporoot
from hw_model_sweep import run_sim

reporoot = get_reporoot()
SIM = os.path.join(reporoot, "src", "ama-riscv-sim")
PASS_STRING = "    0x051e tohost    : 0x00000001"

def get_symbol_address(elf_path, symbol_name):
    with open(elf_path, 'rb') as f:
        elf = ELFFile(f)

        symtab = elf.get_section_by_name('.symtab')
        if symtab is None:
            raise RuntimeError("No symbol table found in ELF")

        for sym in symtab.iter_symbols():
            if sym.name == symbol_name:
                # st_value is the symbol’s virtual address
                return sym['st_value']

    raise KeyError(f"Symbol '{symbol_name}' not found in {elf_path}")

def is_end_of_cmp(ref_str, cmp_str):
    return ref_str == "00000000" and cmp_str == "a5a5a5a5"

def sim_matches(
        ref_path, cmp_path, ref_start=0, cmp_start=0, dc_extra_cmp=False):
    """
    Compare two text files line-by-line, beginning at line ref_start in ref_path
    and cmp_start in cmp_path (0-based).
    Prints any mismatches with their line numbers.
    """
    with open(ref_path, 'r') as f1, open(cmp_path, 'r') as f2:
        # skip the first lines, if any
        it1 = islice(f1, ref_start, None)
        it2 = islice(f2, cmp_start, None)

        for offset, (line1, line2) in enumerate(zip(it1, it2), start=1):
            # strip trailing newlines for cleaner comparison
            if line1.rstrip('\n') != line2.rstrip('\n'):
                if is_end_of_cmp(line1.rstrip('\n'), line2.rstrip('\n')):
                    return True
                ln1 = ref_start + offset
                ln2 = cmp_start + offset
                print(f"Mismatch: file1@{ln1} != file2@{ln2}")
                print(f" - {ref_path}:{ln1}: {line1!r}")
                print(f" - {cmp_path}:{ln2}: {line2!r}")
                return False

        else:
            # after zip completes, check if one file has extra lines
            extra_ref = list(it1)
            extra_cmp = list(it2)

            if extra_ref:
                print(f"...{len(extra_ref)} extra lines in {ref_path}" +
                      f" starting at line {ref_start + offset + 1}")
                return False

            if extra_cmp:
                # likely harmless as 720 bytes are always dumped
                if not dc_extra_cmp:
                    print(f"...{len(extra_cmp)} extra lines in {cmp_path}" +
                          f" starting at line {cmp_start + offset + 1}")
                return True

            return False

if __name__ == '__main__':
    test_loc = os.path.join(reporoot, "sw", "baremetal", "imperas-riscv-tests")
    refs_path = os.path.join(test_loc, "references")
    ref = glob.glob(refs_path)[0]
    elfs = glob.glob(os.path.join(test_loc, "*", "*.elf"))
    elfs.sort()

    for elf in elfs:
        test_dirname = os.path.basename(os.path.dirname(elf))

        if test_dirname.startswith("I-ENDIANESS"):
            addr = get_symbol_address(elf, "test_A_res")
        elif test_dirname.startswith("I-"):
            addr = get_symbol_address(elf, "test_A1_res")
        else:
            addr = get_symbol_address(elf, "signature_1_0")

        mem_dump_args = ["--mem_dump_start", f"{addr:x}",
                         "--mem_dump_size", "720"]
        cmd = [SIM] + [elf] + mem_dump_args
        run_sim(cmd)

        sim_dump = os.path.join(f"out_{test_dirname}_test", "mem_dump.log")
        if not os.path.exists(sim_dump):
            print("Can't find dumped memory log")
            sys.exit(1)

        ref_name = f"{test_dirname}-01.reference_output"
        match_ref = glob.glob(os.path.join(refs_path, ref_name))[0]

        if not sim_matches(match_ref, sim_dump, 0, 1, True):
            print("Compare Failed")
            sys.exit(1)

    print(f"Ran {len(elfs)} tests. All results match.")
