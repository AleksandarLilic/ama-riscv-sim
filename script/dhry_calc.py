#!/usr/bin/env python3

import argparse

parser = argparse.ArgumentParser(description='Find DMIPS/MHz for a given run')
parser.add_argument('cpu_f', type=float, help='CPU frequency in MHz')
parser.add_argument('dhry_per_sec', type=float, help='Dhrystone result printed as "Dhrystones per Second:"')

args = parser.parse_args()

vax_dhry_per_sec = 1757.0 # VAX 11/780, nominally a 1 MIPS machine
dmips = args.dhry_per_sec / vax_dhry_per_sec
dmips_mhz = dmips / args.cpu_f
#print(f"DMIPS: {dmips:.2f}")
print(f"DMIPS/MHz: {dmips_mhz:.2f}")
