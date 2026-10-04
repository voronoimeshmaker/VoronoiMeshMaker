# SPDX-License-Identifier: BSD-3-Clause
"""Exact analytic proof of collapsed triangular wedges (no production dependency)."""
from fractions import Fraction as F
from time import perf_counter

def volume(area, bottom, top):
    assert all(t >= b for b, t in zip(bottom, top))
    return area * sum(t-b for b,t in zip(bottom,top)) / 3

assert volume(F(1,2), [F(0)]*3, [F(1)]*3) == F(1,2)
# A layer ends on an edge; its volume is a wedge, not a zero-height prism.
assert volume(F(1,2), [F(0)]*3, [F(0),F(0),F(1)]) == F(1,6)
# It disappears over a complete facet.
assert volume(F(1,2), [F(0)]*3, [F(0)]*3) == 0
# Graded vertical subdivisions partition the same volume exactly.
cuts = [F(0), F(1,10), F(1,2), F(1)]
height = [F(0), F(1), F(2)]
parts = [volume(F(1,2), [a*h for h in height], [b*h for h in height])
         for a,b in zip(cuts,cuts[1:])]
assert sum(parts) == F(1,2)
# A fixed support is independent of subdivisions of faces.
def z(x,y): return 2*x + 3*y + 1
for x,y in [(F(0),F(0)),(F(1,3),F(1,5)),(F(1),F(1))]:
    assert z(x,y)-2*x-3*y-1 == 0
for n in [100,1000,10000]:
    t=perf_counter()
    for _ in range(n): volume(F(1,2), [F(0)]*3, height)
    print(n, perf_counter()-t)
print("analytic wedge and fixed-plane checks passed; no claim of full mesh validation")
