#!/bin/bash
set -e

# run on produced elfs to get .mem files for RTL sim, e.g.
# sw/arch-tests
# $ ./elf2mem.sh riscv-arch-test/work/ama-riscv-rv32im/elfs/rv32i/

ELF_DIR="$1"

REPO_ROOT=$(git rev-parse --show-toplevel)

OBJCOPY=${RV_GNU_DEV}/riscv32-unknown-elf-objcopy
BIN2HEX=${REPO_ROOT}/sw/bin2hex.py

find "$ELF_DIR" -type f -name '*.elf' | sort | while read -r f; do
  echo "$f"
  "$OBJCOPY" -O binary "$f" "${f%.elf}.bin"
  "$BIN2HEX" -w 128 -a 0 "${f%.elf}.bin" "${f%.elf}.mem"
done
