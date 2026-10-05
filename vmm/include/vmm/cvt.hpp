// SPDX-License-Identifier: BSD-3-Clause
#pragma once
//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <string>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/partition.hpp>
#include <vmm/domain/partition3d.hpp>
#include <vmm/mesh/layered.hpp>
#include <vmm/mesh/mesh.hpp>
#include <vmm/sites/site_set.hpp>
#include <vmm/sites/sources3d.hpp>
namespace vmm {
/// Uniform-density constrained Lloyd controls; tolerances use partition length.
struct CvtOptions {
    std::size_t max_iterations=50;
    std::size_t max_backtracks=24;
    Real relaxation=1;
    Real relative_tolerance=1e-6;
};
/// Accepted iterates only; energies include the initial reconstructed mesh.
struct CvtReport {
    std::vector<Real> energy;
    std::vector<Real> relative_displacement;
    std::size_t rejected_steps=0;
    bool converged=false;
    bool stalled=false;
    std::string last_rejection;
};
template<std::size_t D>
struct CvtResult { Mesh<D> mesh; CvtReport report; };
/// @brief Integrates squared distance to the generating sites, with uniform density.
/// @note The mesh must contain oriented planar faces and actual Voronoi generators.
template<std::size_t D>
[[nodiscard]] Real cvt_energy(const Mesh<D>& mesh);
/// @brief Runs constrained Lloyd on an immutable 2D regional partition.
/// @param partition Fixed boundary/interface geometry, reused at every iteration.
/// @param sites Initial generators and fixed region membership; weights unsupported.
/// @param options Iteration, backtracking and convergence controls.
/// @return Last accepted mesh and an explicit convergence/stagnation report.
/// @par Level
/// Intermediate
/// @par Location
/// vmm/cvt.hpp
[[nodiscard]] Result<CvtResult<2>> optimize_cvt(const Partition2D& partition,
    const SiteSet& sites,const CvtOptions& options={});
/// 3D counterpart; does not preserve a column arrangement.
[[nodiscard]] Result<CvtResult<3>> optimize_cvt(const Partition3D& partition,
    const SiteSet3D& sites,const CvtOptions& options={});
/// Fixed volumetric regions extracted from a generated layered mesh.
/// source_region maps compact partition regions to the original mesh region IDs.
struct LayeredCvtDomain {
    Partition3D partition;
    std::vector<RegionId> source_region;
};
/// Extracts only regional interfaces and external boundaries, not layer subdivisions.
[[nodiscard]] Result<LayeredCvtDomain> cvt_domain(const LayeredMesh& mesh);
} // namespace vmm
