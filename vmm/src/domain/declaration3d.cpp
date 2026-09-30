// ============================================================================
// File: declaration3d.cpp
// Description: Declaration3D.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <string>
#include <utility>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/declaration3d.hpp>

namespace vmm {

Result<RegionId> Declaration3D::add_region_surface(std::string name, MediumId medium, TriangleSurface surface) {
    if (name.empty()) return fail(ErrorCode::InvalidArgument, "region name is empty");
    if (!medium.valid() || medium.index() >= media_.size()) return fail(ErrorCode::UnknownMedium, name);
    if (std::ranges::any_of(regions_, [&](const RegionInfo& r) { return r.name == name; })) {
        return fail(ErrorCode::DuplicateName, name);
    }
    if (surface.triangle_count() == 0) return fail(ErrorCode::InvalidSurface, "empty surface");
    regions_.push_back({std::move(name), medium});
    const RegionId id = RegionId::from_index(regions_.size() - 1);
    layers_.push_back({id, std::move(surface)});
    return id;
}

Status Declaration3D::add_hole_surface(TriangleSurface surface) {
    if (surface.triangle_count() == 0) return fail(ErrorCode::InvalidSurface, "empty surface");
    layers_.push_back({RegionId::invalid(), std::move(surface)});
    return {};
}

}  // namespace vmm
