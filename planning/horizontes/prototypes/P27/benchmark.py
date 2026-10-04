#!/usr/bin/env python3
"""Synthetic CLI timing including generation and serialization; no physical fields."""
import json
from pathlib import Path
import re
import subprocess
import time

root = Path(__file__).resolve().parents[4]
work = root / "build/horizons-bench"
work.mkdir(parents=True, exist_ok=True)
results = []
for n in (16, 32, 64):
    for layers in (2, 8):
        stem = f"n{n}_l{layers}"
        fractions = " ".join(str(k/layers) for k in range(layers+1))
        (work/f"{stem}.hgrid").write_text(f'VMM_HORIZONS 1\n2 0 1\n2 0 1\n2\n"bottom"\n4 0 0 0 0\n"top"\n4 1 1 1 1\n1\n{layers+1} {fractions}\n')
        config = work/f"{stem}.cfg"
        config.write_text(f"dimension = 2\nhorizons = {stem}.hgrid\noutput = {stem}\nformats = vmesh\n[region base]\nshape = rectangle\nlo = 0 0\nhi = 1 1\nsites = grid\nsites.spacing = {1/n}\n")
        start = time.perf_counter()
        run = subprocess.run(["/usr/bin/time", "-f", "peak_kib=%M", str(root/"build/bin/vmm-mesh"), str(config)], capture_output=True, text=True, check=True)
        elapsed = time.perf_counter()-start
        (work/f"{stem}.log").write_text(run.stdout+run.stderr)
        cells, internal, boundary = map(int, re.search(r"cells (\d+) \| internal faces (\d+) \| boundary faces (\d+)", run.stdout).groups())
        points = int(re.search(r"^points (\d+)$", (work/f"{stem}.vmesh").read_text(), re.M).group(1))
        results.append(dict(columns=cells//layers, layers=layers, cells=cells, faces=internal+boundary, points=points, seconds=elapsed, peak_kib=int(re.search(r"peak_kib=(\d+)",run.stderr).group(1)), microseconds_per_cell=elapsed*1e6/cells))
        print(results[-1], flush=True)
(work/"results.json").write_text(json.dumps(results,indent=2)+"\n")
