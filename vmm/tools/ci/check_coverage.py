#!/usr/bin/env python3
# ============================================================================
# File: check_coverage.py
# Description: Runs gcovr on vmm/ and enforces R25 per file: at least 90% of
#              lines and 80% of branches (exceptions listed with a reason in
#              coverage_exceptions.txt). Writes a text and a JSON summary.
# SPDX-License-Identifier: BSD-3-Clause
# ============================================================================
import argparse
import json
import pathlib
import subprocess
import sys

LINES, BRANCHES = 90.0, 80.0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True, help="repository root")
    ap.add_argument("--build", required=True, help="build directory")
    ap.add_argument("--output", required=True, help="output prefix")
    args = ap.parse_args()
    root = pathlib.Path(args.root)
    exceptions = {}
    exc_file = root / "vmm/tools/ci/coverage_exceptions.txt"
    if exc_file.exists():
        for line in exc_file.read_text().splitlines():
            if line.strip() and not line.startswith("#"):
                path, reason = line.split("|", 1)
                exceptions[path.strip()] = reason.strip()
    summary = pathlib.Path(args.output + ".json")
    cmd = ["gcovr", "-r", str(root), "--object-directory", args.build,
           "--filter", "vmm/src/", "--filter", "vmm/include/",
           "--exclude-throw-branches", "--exclude-unreachable-branches",
           "--gcov-ignore-parse-errors=negative_hits.warn_once_per_file",
           "--json-summary", str(summary), "--txt", args.output + ".txt", "--print-summary"]
    print(" ".join(cmd))
    subprocess.run(cmd, check=True)
    data = json.loads(summary.read_text())
    failures = []
    for f in data["files"]:
        name = f["filename"]
        line = f["line_percent"] if f["line_total"] else 100.0
        branch = f["branch_percent"] if f["branch_total"] else 100.0
        status = "ok"
        if line < LINES or branch < BRANCHES:
            status = f"EXCEPTION ({exceptions[name]})" if name in exceptions else "FAIL"
            if name not in exceptions:
                failures.append(name)
        print(f"{name:70s} lines {line:6.1f}% ({f['line_covered']}/{f['line_total']})  "
              f"branches {branch:6.1f}% ({f['branch_covered']}/{f['branch_total']})  {status}")
    print(f"total: lines {data['line_percent']}% branches {data['branch_percent']}% functions {data['function_percent']}%")
    print(f"R25: {len(failures)} file(s) below 90% lines / 80% branches")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
