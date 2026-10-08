#!/usr/bin/env python3
"""Reproducible end-to-end CVT measurements; run in Ubuntu-26.04-Test.

python3 planning/cvt/benchmark.py --build build --repeats 3
Includes partition construction, optimization, validation and native-file IO.
Each sample runs in a separate process; GNU time supplies peak RSS in KiB.
"""
# SPDX-License-Identifier: BSD-3-Clause
import argparse
import csv
import pathlib
import re
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=pathlib.Path, required=True)
    parser.add_argument("--repeats", type=int, default=3)
    args = parser.parse_args()
    if args.repeats < 1:
        parser.error("--repeats must be positive")
    build = args.build.resolve()
    output = build / "cvt_benchmark"
    output.mkdir(exist_ok=True)
    root = pathlib.Path(__file__).resolve().parents[2]
    horizon = root / "vmm/examples/config/cvt_horizons.hgrid"
    rows = []
    for kind, counts in (("2d", (16, 64, 256)), ("3d", (8, 27, 64)),
                         ("horizons", (4, 9, 16))):
        for count in counts:
            dimension = 3 if kind == "3d" else 2
            shape = "cuboid" if dimension == 3 else "rectangle"
            lo = "0 0 0" if dimension == 3 else "0 0"
            hi = "1 1 1" if dimension == 3 else "2 1"
            cfg = output / f"{kind}_{count}.cfg"
            text = f"dimension = {dimension}\nseed = 7\nformats = vmesh\ncvt_iterations = 5\n"
            if kind == "horizons":
                text += f"horizons = {horizon}\n"
            text += (f"[region domain]\nshape = {shape}\nlo = {lo}\nhi = {hi}\n"
                     f"sites = count\nsites.count = {count}\n")
            cfg.write_text(text)
            for repeat in range(args.repeats):
                stem = output / f"{kind}_{count}_{repeat}"
                timing = stem.with_suffix(".time")
                run = subprocess.run(["/usr/bin/time", "-f", "%e %M", "-o", str(timing),
                                      str(build / "bin/vmm-mesh"), "--output", str(stem), str(cfg)],
                                     text=True, capture_output=True, check=True)
                stem.with_suffix(".log").write_text(run.stdout + run.stderr)
                cells = re.search(r"cells (\d+)", run.stdout).group(1)
                cvt = re.search(r"CVT: (\d+) accepted iterations \| converged (\w+) \| stalled (\w+)"
                                r" \| energy (\S+) -> (\S+)", run.stdout)
                if cvt is None:
                    raise RuntimeError("Missing CVT report")
                accepted, converged, stalled, initial, final = cvt.groups()
                if float(final) > float(initial):
                    raise RuntimeError("CVT energy increased")
                seconds, rss = timing.read_text().split()
                rows.append(dict(case=kind, input_sites=count, cells=cells, repeat=repeat,
                                 seconds=seconds, peak_rss_kib=rss, accepted=accepted,
                                 converged=converged, stalled=stalled,
                                 initial_energy=initial, final_energy=final))
                print(rows[-1], flush=True)
    with (output / "results.csv").open("w", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)


if __name__ == "__main__":
    main()
