#!/usr/bin/env python3

import argparse
import sys
from itertools import zip_longest

# itertools grouper recipe; aligned with SiFive freedom-bin2hex.
def grouper(iterable, n, fillvalue=None):
    """Collect data into fixed-length chunks or blocks."""
    args = [iter(iterable)] * n
    return zip_longest(*args, fillvalue=fillvalue)

def parse_verilog_address(spec):
    """Resolve --address: decimal / 0x hex / hex digits fallback."""
    spec = spec.strip()
    try:
        return int(spec, 0)
    except ValueError:
        pass
    return int(spec, 16)

def parse_args():
    parser = argparse.ArgumentParser(description="Convert a binary file to a format that can be read in verilog via $readmemh(). By default read from stdin and write to stdout.")
    parser.add_argument("infile", nargs="?", type=argparse.FileType("rb"), default=sys.stdin.buffer, help="Input binary file (default: stdin)")
    parser.add_argument("outfile", nargs="?", type=argparse.FileType("w"), default=sys.stdout, help="Output .mem text file (default: stdout)")
    parser.add_argument("--bit-width", "-w", type=int, required=True, help="How many bits per row.")
    parser.add_argument("-m", "--memory", type=int, default=None, metavar="BYTES", help="Pad image to this byte size with trailing zeros (multiple of bit_width/8); error if binary exceeds it.")
    parser.add_argument("-a", "--address", type=str, default=None, metavar="ADDR", help="If set, leading @ADDR line ($readmemh): decimal, 0x hex, or hex digits.")
    return parser.parse_args()

def convert_rows(data, bit_width):
    byte_width = bit_width // 8
    for row in grouper(data, byte_width, fillvalue=0):
        # Verilog packs MSB of the vector first.
        hex_row = "".join("{:02x}".format(b) for b in reversed(row))
        yield hex_row

def main():
    args = parse_args()
    if args.bit_width % 8 != 0:
        sys.exit("bit_width must be a multiple of 8 bits (i.e. one byte)")

    byte_width = args.bit_width // 8

    data = args.infile.read()

    if args.memory is not None:
        if args.memory % byte_width != 0:
            sys.exit(
                f"--memory ({args.memory}) must be a multiple of "
                f"bit_width/8 ({byte_width})"
            )
        if len(data) > args.memory:
            sys.exit(
                f"Binary size ({len(data)} bytes) exceeds "
                f"--memory ({args.memory})"
            )
        pad = args.memory - len(data)
        if pad:
            data = data + b"\x00" * pad

    out = args.outfile
    if args.address is not None:
        try:
            addr = parse_verilog_address(args.address)
        except ValueError as e:
            sys.exit(f"Invalid --address {args.address!r}: {e}")
        out.write("@{:x}\n".format(addr))

    for hex_row in convert_rows(data, args.bit_width):
        out.write(hex_row + "\n")

if __name__ == "__main__":
    main()
