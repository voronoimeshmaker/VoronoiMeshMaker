// ============================================================================
// File: partition3d.cpp
// Description: Partition3D measures and region surfaces.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/partition3d.hpp>

namespace vmm {
namespace {

Real tetra_volume(const std::vector<Vec3>& p, const Triangle& t) {
    return dot(p[t[0]], cross(p[t[1]], p[t[2]])) / 6;
}

Real triangle_area(const std::vector<Vec3>& p, const Triangle& t) {
    return 0.5 * norm(cross(p[t[1]] - p[t[0]], p[t[2]] - p[t[0]]));
}

}  // namespace

Partition3D::Partition3D(std::vector<Vec3> vertices, std::vector<PartitionTriangle> triangles,
                         std::vector<RegionInfo> regions, std::vector<std::string> media, std::vector<std::string> patches)
    : vertices_(std::move(vertices)),
      triangles_(std::move(triangles)),
      regions_(std::move(regions)),
      media_(std::move(media)),
      patches_(std::move(patches)) {}

Result<TriangleSurface> Partition3D::region_surface(RegionId r) const {
    if (!r.valid() || r.index() >= regions_.size()) return fail(ErrorCode::InvalidArgument, "region id");
    std::vector<std::string> names = patches_;
    const auto interface = static_cast<std::uint32_t>(names.size());
    names.emplace_back("interface");
    std::unordered_map<std::uint32_t, std::uint32_t> local;
    std::vector<Vec3> points;
    std::vector<Triangle> tris;
    std::vector<std::uint32_t> tri_patch;
    const auto vertex = [&](std::uint32_t v) {
        const auto [it, added] = local.emplace(v, static_cast<std::uint32_t>(points.size()));
        if (added) points.push_back(vertices_[v]);
        return it->second;
    };
    for (const PartitionTriangle& t : triangles_) {
        if (t.inside == r) {
            tris.push_back({vertex(t.v[0]), vertex(t.v[1]), vertex(t.v[2])});
        } else if (t.outside == r) {
            tris.push_back({vertex(t.v[0]), vertex(t.v[2]), vertex(t.v[1])});
        } else {
            continue;
        }
        tri_patch.push_back(t.inside.valid() && t.outside.valid() ? interface : t.patch.value);
    }
    return TriangleSurface::make(std::move(points), std::move(tris), std::move(tri_patch), std::move(names));
}

Real Partition3D::region_volume(RegionId r) const {
    Real v = 0;
    for (const PartitionTriangle& t : triangles_) {
        if (t.inside == r) v += tetra_volume(vertices_, t.v);
        if (t.outside == r) v -= tetra_volume(vertices_, t.v);
    }
    return v;
}

Real Partition3D::total_volume() const {
    Real v = 0;
    for (std::size_t r = 0; r < regions_.size(); ++r) v += region_volume(RegionId::from_index(r));
    return v;
}

Real Partition3D::boundary_area() const {
    Real a = 0;
    for (const PartitionTriangle& t : triangles_) {
        if (t.inside.valid() != t.outside.valid()) a += triangle_area(vertices_, t.v);
    }
    return a;
}

Real Partition3D::interface_area(RegionId a, RegionId b) const {
    Real s = 0;
    for (const PartitionTriangle& t : triangles_) {
        if ((t.inside == a && t.outside == b) || (t.inside == b && t.outside == a)) s += triangle_area(vertices_, t.v);
    }
    return s;
}

}  // namespace vmm
