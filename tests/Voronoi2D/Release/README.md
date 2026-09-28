# Release Audit

All tests use Ubuntu-26.04-Test. They do not use Git or the solver's paper directory.
The Cartesian known answers are derived from Nx=256, Ny=85 and the 30 by 10 domain.
Random tests record seed 8675309 and use 21760 generators. Hexagonal tests use 256
sites along the longest side, clipped to the same rectangular domain.

Build in a separate binary directory with VMM_BUILD_EXAMPLES=OFF,
VMM_BUILD_PAPER=OFF, VMM_ENABLE_FAST_MATH=OFF and VMM_TEST_SANITIZERS=ON.
Sanitizers are opt-in and disable LTO for the diagnostic build. Their compile/link
flags propagate to build-tree consumers of the instrumented library. Use a separate
build with VMM_TEST_SANITIZERS=OFF for installation and performance measurements;
existing CMake caches retain their old value until explicitly reconfigured.
Set VMM_AUDIT_DIR to a new directory for each run. The Release test executable writes
diagnostic CSV files and ASCII/binary VTU meshes there. Run verify_xml.py with that
directory to independently parse both formats and check polygon connectivity.

The native bandwidth expectation 256 denotes max(abs(i-j)). The existing
matrix_bandwidth member includes the diagonal and therefore returns 257.
The Cartesian shared vertex count is (256+1)(85+1)=22102, also obtained from
Euler's V-E+F=1 with E=43861 and F=21760. The specification's <22000 requirement
is inconsistent with its own Cartesian reference; tests use the derived count.
delaunay_neighbours() is a span over the legacy neighbor_ids storage; it is not a
second mutable vector. face_neighbours() derives its entries from internal faces.

T2 samples the centroid and interior interpolations towards every vertex and face
midpoint. A violation is not ignored merely because it is close to an edge. CSV
distances provide evidence for a precision-artifact diagnosis. The diagnostic
1e-6 distance threshold never changes the local nearest-owner acceptance threshold.

T9 uses graph invariants for random intermediate meshes, not Cartesian graph counts.
Rectangularly clipped hexagonal boundary cells need not be centroidal: an unrestricted
claim that Lloyd is a no-op on them is not a mathematically valid release condition.
The reference Cartesian mesh, by contrast, is centroidal throughout.
