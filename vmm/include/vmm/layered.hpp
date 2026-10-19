// SPDX-License-Identifier: BSD-3-Clause
#pragma once
//==============================================================================
//  C++ standard library
//==============================================================================
#include <functional>
#include <span>
#include <utility>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/mesh/layered.hpp>
namespace vmm {
/// Generates columns over an existing 2D mesh using a frozen horizon grid.
/// Parameters: fractions per horizon interval, including 0 and 1; empty means
/// one cell per interval. Region identity is (base region, horizon interval).
/// Notes: pinch-outs omit absent cells; all faces are planar. Functions sampled
/// by HorizonGrid::sample are approximated once, not during reconstruction.
/// @par Level
/// Intermediate
/// @sa HorizonGrid, LayeredMesh
/// @par Location
/// vmm/layered.hpp
/// @par Examples
/// ex_horizons.cpp
[[nodiscard]] Result<LayeredMesh> generate_layered_mesh(const Mesh2D& base,
    const HorizonGrid& horizons, std::vector<std::vector<Real>> fractions = {});
/// Samples a horizon at the original base generator.
[[nodiscard]] inline Result<Real> sample_at_base_generator(const Mesh2D& base,
    const HorizonGrid& horizons,CellId column,std::size_t horizon) {
    return horizons.elevation(horizon,base.site(column));
}
/// Area mean of the frozen piecewise affine surface, integrated by local clipping.
/// The referenced base must outlive this sampler and must not be reassigned.
class PolygonMeanSampler {
public:
    explicit PolygonMeanSampler(const Mesh2D& base) : base_(&base), index_(base) {}
    [[nodiscard]] Result<Real> operator()(const Mesh2D& base,const HorizonGrid& horizons,
        CellId column,std::size_t horizon) const;
private:
    const Mesh2D* base_;
    CellFaceIndex index_;
};
/// Merges positive thin layers into adjacent cells of the same horizon interval.
[[nodiscard]] Result<void> merge_thin_layers(std::vector<Real>& levels,
    std::span<const std::size_t> intervals,Real minimum);
/// Without a thickness policy, positive thin layers are rejected; zero layers remain absent.
struct FlatColumnOptions {
    Real min_thickness_absolute = 0;
    Real min_thickness_relative = 0; ///< Fraction of the whole column thickness, in [0,1].
    std::function<Result<void>(std::vector<Real>&,std::span<const std::size_t>,Real)> thickness_policy = {};
    Real min_bottom_thickness = 0; ///< Raises the bottom cell top within its horizon interval.
};
/// Open sampling policy; custom callables have the same signature as the default.
template<class Sampler=decltype(&sample_at_base_generator)>
struct FlatColumns { Sampler sampler=&sample_at_base_generator; FlatColumnOptions options={}; };
namespace detail {
[[nodiscard]] Result<LayeredMesh> generate_flat_columns(const Mesh2D& base,
    const HorizonGrid& horizons,std::vector<std::vector<Real>> fractions,
    std::vector<std::vector<Real>> elevations,FlatColumnOptions options);
}
/// Generates constant-elevation prisms directly, without a terrain triangulation.
template<class Sampler>
[[nodiscard]] Result<LayeredMesh> generate_layered_mesh(const Mesh2D& base,
    const HorizonGrid& horizons,std::vector<std::vector<Real>> fractions,FlatColumns<Sampler> policy) {
    std::vector<std::vector<Real>> elevations(base.cell_count());
    for(CellId column:base.cells()) for(std::size_t h=0;h<horizons.horizon_count();++h) {
        auto z=policy.sampler(base,horizons,column,h);
        if(!z) return std::unexpected(z.error());
        elevations[column.index()].push_back(*z);
    }
    return detail::generate_flat_columns(base,horizons,std::move(fractions),std::move(elevations),policy.options);
}
} // namespace vmm
