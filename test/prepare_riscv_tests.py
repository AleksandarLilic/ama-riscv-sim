import os
import subprocess
import glob
import json
import argparse

GIT_ROOT = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).strip().decode('utf-8')
TEST_DIR = os.path.join(GIT_ROOT, 'sw/baremetal')
ISA_TEST_DIR = os.path.join(GIT_ROOT, 'sw/riscv-isa-tests')
WORK_DIR = os.getcwd()

def file_exists(filename):
    if not os.path.exists(filename):
        raise argparse.ArgumentTypeError(f"File '{filename}' not found")
    return filename

def parse_args():
    parser = argparse.ArgumentParser(description="Prepare RISC-V tests for Google Test")
    parser.add_argument("--testlist", required=True, type=file_exists, help="JSON file with tests to prepare")
    parser.add_argument("--isa_tests", default=False, action='store_true', help="Also prepare ISA tests")
    return parser.parse_args()

args = parse_args()
with open(args.testlist, 'r') as f:
    tests = json.load(f)

print(f"Tests directories in json input config: {len(tests)}")

out_txt = []
for directory, test_list in tests.items():
    test_dir = os.path.join(TEST_DIR, directory)
    os.chdir(test_dir)
    subprocess.run(["make", "clean"], stdout=subprocess.PIPE)
    for test in test_list:
        _ = subprocess.run(["make", f"{test}.elf"], stdout=subprocess.PIPE)
        out_txt.append(os.path.join(test_dir, f"{test}.bin"))

isa_out_txt = []
if args.isa_tests:
    print("Preparing ISA tests also")
    os.chdir(ISA_TEST_DIR)
    subprocess.run(["make", "clean"], stdout=subprocess.PIPE)
    subprocess.run(["make", "-j"], stdout=subprocess.PIPE)
    subprocess.run(["make", "DIR=modified_riscv-tests/isa/rv32mi/"], 
                   stdout=subprocess.PIPE)
    isa_out_txt = glob.glob(f"{ISA_TEST_DIR}/*.bin")

os.chdir(WORK_DIR)
all_out_txt = isa_out_txt + out_txt
print(f"Total tests prepared: {len(all_out_txt)}")
with open("gtest_testlist.txt", 'w') as f:
    f.write("\n".join(all_out_txt))
