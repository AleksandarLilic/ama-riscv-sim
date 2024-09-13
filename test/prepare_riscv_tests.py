import os
import subprocess
import glob
import json
import argparse

GIT_ROOT = (subprocess.check_output(['git', 'rev-parse', '--show-toplevel'])
            .strip().decode('utf-8'))
TEST_DIR = os.path.join(GIT_ROOT, 'sw/baremetal')
ISA_TEST_DIR = os.path.join(GIT_ROOT, 'sw/riscv-isa-tests')
WORK_DIR = os.getcwd()

def file_exists(filename):
    if not os.path.exists(filename):
        raise argparse.ArgumentTypeError(f"File '{filename}' not found")
    return filename

def parse_args():
    parser = argparse.ArgumentParser(description="Prepare RISC-V tests for Google Test")
    parser.add_argument("-t", "--testlist", required=True, type=file_exists, help="JSON file with tests to prepare")
    parser.add_argument("--isa_tests", default=False, action='store_true', help="Also prepare ISA tests")
    return parser.parse_args()

def run_make(make_cmd):
    make_status = subprocess.run(make_cmd,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
    check_make_status(make_status, f"compile {make_cmd}")

def check_make_status(make_status, msg: str):
    if make_status.returncode != 0:
        print("Makefile steps:")
        print(make_status.stdout.decode('utf-8'))
        if make_status.stderr:
            print("Makefile stderr:")
            print(make_status.stderr.decode('utf-8'))
        raise RuntimeError(f"Error: Makefile failed to {msg}.")

args = parse_args()
with open(args.testlist, 'r') as f:
    tests = json.load(f)

print(f"Tests directories in json input config: {len(tests)}")
# get first test directory to make common object files
test_dir = os.path.join(TEST_DIR, list(tests.keys())[0])
os.chdir(test_dir)
run_make(["make", "clean", "common"])
run_make(["make", "common"])
out_txt = []
for directory, test_list in tests.items():
    test_dir = os.path.join(TEST_DIR, directory)
    os.chdir(test_dir)
    run_make(["make", "clean"])
    make_all = test_list == ["all"]
    all_targets = test_list if make_all else [f"{t}.elf" for t in test_list]
    make_cmd = ["make", "-j"] + all_targets
    run_make(make_cmd)
    if make_all:
        all_bin_files = glob.glob(f"{test_dir}/*.bin")
    else:
        all_bin_files = [os.path.join(test_dir, f"{t}.bin") for t in test_list]
    out_txt.extend(all_bin_files)

isa_out_txt = []
if args.isa_tests:
    print("Preparing ISA tests also")
    os.chdir(ISA_TEST_DIR)
    run_make(["make", "clean"])
    run_make(["make", "-j", "DIR=riscv-tests/isa/rv32ui"]) # RV32I
    run_make(["make", "-j", "DIR=modified_riscv-tests/isa/rv32mi/"]) # memory
    run_make(["make", "-j", "DIR=riscv-tests/isa/rv32um/"]) # RV32M
    isa_out_txt = glob.glob(f"{ISA_TEST_DIR}/*.bin")

os.chdir(WORK_DIR)
all_out_txt = isa_out_txt + out_txt
print(f"Total tests prepared: {len(all_out_txt)}")
with open("gtest_testlist.txt", 'w') as f:
    f.write("\n".join(all_out_txt))
