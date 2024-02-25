import sys
import os
import re
import subprocess

def update_run_cycles(exec_log_file, target_file):
    # get cycle count from exec log
    spike_cycles_offset = 4
    with open(exec_log_file, 'r') as f:
        cycles = int(f.readline().strip()) + spike_cycles_offset
    
    # replace first occurrence of run parameter for spike
    with open(target_file, 'r') as f:
        lines = f.readlines()
    pattern = re.compile(r"^rs \d+")
    for i, line in enumerate(lines):
        if pattern.match(line):
            lines[i] = f"rs {cycles}\n"
            break
    with open(target_file, 'w') as f:
        f.writelines(lines)

def extract_hex_values(file_path):
    with open(file_path, 'r') as file:
        content = file.read()
    hex_values = re.findall(r'0x[0-9A-Fa-f]+', content)
    return hex_values

def compare_hex_values(exec_log_file, output_file):
    # extract hex values from both files
    exec_log_hex_values = extract_hex_values(exec_log_file)
    output_file_hex_values = extract_hex_values(output_file)
    
    # check if both files have the same number of hex values (34 expected)
    if len(exec_log_hex_values) < 34:
        print(f"Execution log file has {len(exec_log_hex_values)} hex values, expected 34")
        return False
    if len(output_file_hex_values) < 34:
        print(f"Output file has {len(output_file_hex_values)} hex values, expected 34")
        return False

    # compare the hex values
    for i in range(34):
        if exec_log_hex_values[i] != output_file_hex_values[i]:
            if i == 0:
                print("PC does not match:", end=" ")
            elif i == 33:
                print("CSR (mscratch) does not match:", end=" ")
            else:
                print(f"Register x{i-1} does not match:", end=" ")
            print(f"{exec_log_hex_values[i]} != {output_file_hex_values[i]}")
            return False
    
    return True

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python script.py <execution_log_file> <target_file> <spike_elf>")
        sys.exit(1)

    exec_log_file = sys.argv[1]
    spike_dbg_file = sys.argv[2]
    update_run_cycles(exec_log_file, spike_dbg_file)
    spike_elf = sys.argv[3]

    spike_out_path = os.path.dirname(spike_dbg_file)
    spike_checker = os.path.join(spike_out_path, "spike.check")
    spike_log = os.path.join(spike_out_path, "spike.log")
    with open(spike_checker, "w") as f:
        subprocess.run(["spike", "-l", f"--log={spike_log}", "-d",
                        f"--debug-cmd={spike_dbg_file}", "--isa=RV32I_zicsr",
                        spike_elf], stdout=f, stderr=f)

    status = compare_hex_values(exec_log_file, spike_checker)
    if status:
        print("PASS")
    else:
        print("FAIL")
