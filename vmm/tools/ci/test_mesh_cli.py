#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# End-to-end fragmentation warnings, quiet output, rejection and exit status.
import pathlib
import re
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix="vmm-cli-") as directory:
    config = pathlib.Path(directory) / "river.cfg"
    text = ("dimension = 2\n[region river]\nshape = polygon\n"
            "xy = 0 0 10 0 10 2.1 0 2.1 0 1.1 9 1.1 9 1.0 0 1.0\n"
            "sites = grid\nsites.spacing = 0.75\nsites.origin = 0.375 0.375\n")
    config.write_text(text)
    normal = subprocess.run([sys.argv[1], str(config)], text=True, capture_output=True)
    assert normal.returncode == 0, normal.stderr
    count = re.search(r"fragmented cells (\d+)", normal.stdout)
    assert count and int(count[1]) > 0, normal.stdout
    assert "VMM-" in normal.stderr, normal.stderr
    quiet = subprocess.run([sys.argv[1], "--quiet", str(config)], text=True, capture_output=True)
    assert quiet.returncode == 0 and not quiet.stdout and not quiet.stderr, quiet
    config.write_text("reject_fragmented = true\noutput = rejected\n" + text)
    rejected = subprocess.run([sys.argv[1], "--quiet", str(config)], text=True, capture_output=True)
    assert rejected.returncode == 1 and "VMM-404" in rejected.stderr, rejected
    assert not (config.parent / "rejected.vmesh").exists()
    print("CLI: warnings, count=" + count[1] + ", quiet and rejection passed")
