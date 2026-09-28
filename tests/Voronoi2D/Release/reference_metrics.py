# SPDX-License-Identifier: GPL-3.0-or-later
"""Extract the reference table independently from the XML output."""
from collections import Counter
import json
from pathlib import Path
import sys
from verify_xml import read

arrays, summary = read(Path(sys.argv[1]) / 'cartesian_binary.vtu')
fv = arrays['n_neighbours_fv']
dt = arrays['n_neighbours_delaunay']
boundary_faces = [a-b for a, b in zip(arrays['n_faces'], fv)]
summary.update({
    'internal_faces': sum(fv)//2,
    'boundary_faces': sum(boundary_faces),
    'boundary_cells': sum(arrays['is_boundary_cell']),
    'nnz_fv': len(fv)+sum(fv),
    'nnz_delaunay': len(dt)+sum(dt),
    'delaunay_only_ties': (sum(dt)-sum(fv))//2,
    'fv_histogram': dict(Counter(fv)),
    'delaunay_histogram': dict(Counter(dt)),
    'corner_delaunay_histogram': dict(Counter(d for d, b in zip(dt, boundary_faces) if b == 2)),
    'area_min': min(arrays['V_P']),
    'area_max': max(arrays['V_P']),
    'compactness_min': min(arrays['q_compactness']),
    'compactness_max': max(arrays['q_compactness']),
    'd_gc_max': max(arrays['d_gc']),
    'tpfa_crossing_all': all(arrays['tpfa_crossing_ok']),
})
print(json.dumps(summary, indent=2))
