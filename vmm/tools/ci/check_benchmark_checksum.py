#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Compare an exact B1 topology checksum only for a registered build platform.
import argparse
import pathlib
import re
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--benchmark", required=True)
parser.add_argument("--records", required=True)
parser.add_argument("--platform", required=True)
args = parser.parse_args()
expected = None
for line in pathlib.Path(args.records).read_text().splitlines():
    if not line or line.startswith("#"):
        continue
    platform, case, checksum = line.split()
    if platform == args.platform and case == "B1-100000":
        expected = checksum
if expected is None:
    print("SKIP: no B1-100000 checksum registered for " + args.platform)
    sys.exit(77)
run = subprocess.run([args.benchmark, "B1", "100000"], text=True, capture_output=True)
print(run.stdout, end="")
print(run.stderr, end="", file=sys.stderr)
match = re.search(r"checksum ([0-9a-f]{16})", run.stdout)
if run.returncode or not match or "PASS" not in run.stdout or match[1] != expected:
    sys.exit("B1 checksum/invariants mismatch; expected " + expected)
