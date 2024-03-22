import re
import sys

def prepare_file(file_path):
    # instructions: 8-digit hex number followed by a colon at the beginning of the line
    inst = r'^[0-9a-fA-F]{8}:'
    # labels:  strings enclosed in angle brackets
    labels = r'<.*?>'
    # comments: strings starting with a hash
    comments = r'#.*'

    # compile regular expression for performance
    compiled_inst = re.compile(inst)
    compiled_labels = re.compile(labels)
    compiled_comments = re.compile(comments)

    lines = []
    with open(file_path, 'r') as file:
        for line in file:
            if compiled_inst.match(line):
                line = line.replace(':', '')
                line = compiled_labels.sub('', line)
                line = compiled_comments.sub('', line)
                line = ' '.join(line.strip().split())
                lines.append(line)

    return lines

def create_lut(cleaned_lines):
    lut = {}
    for line in cleaned_lines:
        parts = line.split(' ', 1)
        if len(parts) == 2:
            pc, instruction = parts
            lut[pc] = instruction
    return lut

def compare(log, lut):
    match_count = 0
    mismatch_count = 0
    missing_in_dasm_count = 0

    for line in log:
        parts = line.split(' ', 1)
        if len(parts) == 2:
            pc, inst = parts

            if pc in lut:
                if inst == lut[pc]:
                    match_count += 1
                else:
                    mismatch_count += 1
                    print(f"Mismatch at PC {pc}: DASM '{lut[pc]}' vs LOG '{inst}'")
            else:
                missing_in_dasm_count += 1
                print(f"PC {pc} found in LOG but not in DASM.")
        else:
            print(f"Invalid line in LOG: {line}")

    return match_count, mismatch_count, missing_in_dasm_count

dasm_file_path = sys.argv[1]
log_file_path = sys.argv[2]

cleaned_dasm = prepare_file(dasm_file_path)
cleaned_log = prepare_file(log_file_path)

lookup_table = create_lut(cleaned_dasm)
match_count, mismatch_count, missing_in_dasm_count = compare(cleaned_log, lookup_table)

print(f"Matches: {match_count}, Mismatches: {mismatch_count}, Missing in DASM: {missing_in_dasm_count}")
