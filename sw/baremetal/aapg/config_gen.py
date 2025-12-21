#!/usr/bin/env python3

import argparse
import os
import shutil
import subprocess
import sys

import regex
from ruamel.yaml import YAML

yaml = YAML()
yaml.preserve_quotes = True
yaml.indent(mapping=2, sequence=4, offset=2)

DEFAULT_CONFIG = "./config_meta.yaml"
TEMPLATE_DIR = "./template/"

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate AAPG config files and optionally build them.")
    parser.add_argument("-c", "--config", type=str, default=DEFAULT_CONFIG, help="Path to the configuration YAML file.")
    parser.add_argument("-m", "--make", action="store_true", help="Run make to build the generated projects.")
    parser.add_argument("-f", "--filter", type=str, nargs='*', help="Filter to only generate specific tests.")
    return parser.parse_args()

args = parse_args()
print(f"Using config file: {args.config}\n")

if not os.path.exists(args.config):
    print(f"Error: Config file {args.config} does not exist.")
    sys.exit(1)

with open(args.config, 'r') as f:
    config = yaml.load(f)

for test, config_updates in config.items():
    # if filter is provided, and any fiter entry is not in test, skip
    if args.filter and not any(regex.search(f, test) for f in args.filter):
        #print(f"Skipping test: {test}")
        continue

    out_dir = os.path.join(".", f"aapg_{test}")
    if os.path.exists(out_dir):
        shutil.rmtree(out_dir)
    shutil.copytree(TEMPLATE_DIR, out_dir)

    new_config_file = os.path.join(out_dir, "config.yaml")

    # merge config into new config file
    with open(new_config_file, 'r') as nf:
        new_config = yaml.load(nf)

    # for each key in value, update new_config
    for group, entry in config_updates.items():
        for key, value in entry.items():
            new_config[group][key] = value

    with open(new_config_file, 'w') as nf:
        yaml.dump(new_config, nf)
    print(f"Generated config file: {new_config_file}")

    if args.make:
        # make codegen -B && make -j
        subprocess.run(["make", "codegen", "-B"], cwd=out_dir, check=True)
        subprocess.run(["make", "-j"], cwd=out_dir, check=True)
        print(f"Built project in: {out_dir}")
        print("")

if not args.make:
    print("")
