import re
import sys

def prepare_file(file_path):
    inst = r'^[0-9a-fA-F]{8}:' # 8-digit hex number followed by a colon
    labels = r'<.*?>' # strings enclosed in angle brackets
    comments = r'#.*' # strings starting with a hash
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
        # splitinto PC and the instruction part, using the first space as the delimiter
        parts = line.split(' ', 1)
        if len(parts) == 2:
            pc, instruction = parts
            lut[pc] = instruction
    return lut

def compare(log, lut):
    match_count = 0
    mismatch_count = 0
    missing_in_lut_count = 0

    for line in log:
        parts = line.split(' ', 1)
        if len(parts) == 2:
            pc, inst = parts

            if pc in lut:
                if inst == lut[pc]:
                    match_count += 1
                else:
                    mismatch_count += 1
                    print(f"Mismatch at PC {pc}: LUT instruction '{lut[pc]}' vs Log instruction '{inst}'")
            else:
                missing_in_lut_count += 1
                print(f"PC {pc} found in log but not in LUT.")
        else:
            print(f"Invalid line in log: {line}")

    return match_count, mismatch_count, missing_in_lut_count

dasm_file_path = sys.argv[1]
log_file_path = sys.argv[2]

cleaned_dasm = prepare_file(dasm_file_path)
cleaned_log = prepare_file(log_file_path)

lookup_table = create_lut(cleaned_dasm)
match_count, mismatch_count, missing_in_lut_count = compare(cleaned_log, lookup_table)

print(f"Matches: {match_count}, Mismatches: {mismatch_count}, Missing in LUT: {missing_in_lut_count}")
