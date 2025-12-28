#!/usr/bin/env python3

import argparse
import concurrent.futures
import glob
import json
import os
import subprocess

import numpy as np

GIT_ROOT = (subprocess.check_output(['git', 'rev-parse', '--show-toplevel'])
            .strip().decode('utf-8'))
TEST_DIR = os.path.join(GIT_ROOT, 'sw/baremetal')
ISA_TEST_DIR = os.path.join(GIT_ROOT, 'sw/riscv-isa-tests')
WORK_DIR = os.getcwd()

CODEGEN_APPS = ["aapg", "sorting", "vector_ew_basic", "matmul"]

def file_exists(filename):
    if not os.path.exists(filename):
        raise argparse.ArgumentTypeError(f"File '{filename}' not found")
    return filename

def parse_args():
    parser = argparse.ArgumentParser(description="Prepare RISC-V tests for Google Test")
    parser.add_argument("-t", "--testlist", required=True, type=file_exists, help="JSON file with tests to prepare")
    parser.add_argument("--isa_tests", default=False, action='store_true', help="Also prepare ISA tests")
    parser.add_argument("--clean_only", default=False, action='store_true', help="Clean all targets and exit")
    parser.add_argument("--clean_first", default=False, action='store_true', help="Clean all targets before building. Otherwise, just run make to build")
    parser.add_argument("--rebuild_codegen", default=False, action='store_true', help="Rebuild apps that use codegen, otherwise use existing codegen outputs")
    parser.add_argument("--hex", default=True, action='store_true', help="Also prepare hex files for RTL simulation")
    parser.add_argument("--aapg_seeds", default=1, type=int, help="Number of different AAPG seeds to build")
    return parser.parse_args()

def run_make(make_cmd, cwd=None):
    make_status = subprocess.run(
        make_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=cwd)
    check_make_status(make_status, f"compile {make_cmd}. CWD='{cwd}'")

def check_make_status(make_status, msg: str):
    if make_status.returncode != 0:
        print("Makefile steps:")
        print(make_status.stdout.decode('utf-8'))
        if make_status.stderr:
            print("Makefile stderr:")
            print(make_status.stderr.decode('utf-8'))
        raise RuntimeError(f"Error: Makefile failed to {msg}.")

def build_test(test):
    t_dir, t_entry = test
    test_list, test_opts = t_entry if len(t_entry) == 2 else (t_entry, [])
    test_dir = os.path.join(TEST_DIR, t_dir)
    if args.clean_first or args.clean_only:
        run_make(["make", "cleanall"], cwd=test_dir) # cleans app and common
    if args.clean_only:
        return

    aapg_run = t_dir.startswith("aapg")
    if aapg_run:
        # always clean codegen for aapg to remove old seeds
        run_make(["make", "clean_codegen"], cwd=test_dir)

    if args.rebuild_codegen:
        # if any of the CODEGEN_APPS is in the test directory, run codegen first
        if any(t_dir.startswith(app) for app in CODEGEN_APPS):
            run_make(["make", "clean_codegen"], cwd=test_dir)
            run_make(["make", "codegen"], cwd=test_dir)

    make_all = (test_list == ["all"])
    all_targets = test_list if make_all else [f"{t}.elf" for t in test_list]
    make_cmd = ["make", "-j", "build_common"] + test_opts
    run_make(make_cmd, cwd=test_dir)
    make_cmd = ["make", "-j"] + hex_gen + all_targets + test_opts

    if not aapg_run:
        run_make(make_cmd, cwd=test_dir)
    else:
        run_make(make_cmd, cwd=test_dir) # build with default seed first
        for _ in range(args.aapg_seeds-1):
            # randomize seed for each run
            seed_rnd = np.random.randint(0, 2**32 - 1)
            run_make(make_cmd + [f"SEED={seed_rnd}"], cwd=test_dir)

    return glob.glob(f"{test_dir}/*.elf")

args = parse_args()
with open(args.testlist, 'r') as f:
    tests = json.load(f)

print(f"Tests directories in json input config: {len(tests)}")

out_txt = []
hex_gen = ["HEX=1"] if args.hex else ["HEX=0"]
# spawning processes is currently slower than threads; 'make' is called with -j
#with concurrent.futures.ProcessPoolExecutor() as executor:
with concurrent.futures.ThreadPoolExecutor() as executor:
    futures = {executor.submit(build_test,item): item for item in tests.items()}
    print(f"Building RISC-V tests with {executor._max_workers} threads")
    for future in concurrent.futures.as_completed(futures):
        test_item = futures[future]
        out_txt.extend(future.result()) # aggregate all elf files

isa_out_txt = []
if args.isa_tests:
    print("Preparing ISA tests also")
    os.chdir(ISA_TEST_DIR)
    run_make(["make", "clean"])
    make_cmd = ["make", "-j"] + hex_gen
    if not args.clean_only:
        run_make(make_cmd + ["DIR=riscv-tests/isa/rv32ui"]) # RV32I
        run_make(make_cmd + ["DIR=modified_riscv-tests/isa/rv32mi/"]) # Zicsr
        run_make(make_cmd + ["DIR=riscv-tests/isa/rv32um/",
                             "MARCH=rv32im_zicsr_zifencei"]) # RV32M
        run_make(make_cmd + ["DIR=riscv-tests/isa/rv32uc/",
                             "MARCH=rv32ic_zicsr_zifencei"]) # RV32C
        isa_out_txt = glob.glob(f"{ISA_TEST_DIR}/*.elf")

if args.clean_only:
    print("Cleaning done. Exiting.")
    exit(0)

os.chdir(WORK_DIR)
all_out_txt = isa_out_txt + out_txt
print(f"Total RISC-V tests prepared: {len(all_out_txt)}")
with open("gtest_testlist.txt", 'w') as f:
    f.write("\n".join(all_out_txt))
