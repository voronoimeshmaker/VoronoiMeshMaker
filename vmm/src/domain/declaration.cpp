// ============================================================================
// File: declaration.cpp
// Description: Domain declaration and medium registry.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/declaration.hpp>

namespace vmm {

Result<MediumId> MediumRegistry::add(std::string name) {
    if (name.empty()) return fail(ErrorCode::InvalidArgument, "medium name is empty");
    if (find(name)) return fail(ErrorCode::DuplicateName, name);
    names_.push_back(std::move(name));
    return MediumId::from_index(names_.size() - 1);
}

std::optional<MediumId> MediumRegistry::find(std::string_view name) const {
    const auto it = std::ranges::find(names_, name);
    if (it == names_.end()) return std::nullopt;
    return MediumId::from_index(static_cast<std::size_t>(it - names_.begin()));
}

Result<RegionId> Declaration2D::new_region(std::string name, MediumId medium) {
    if (name.empty()) return fail(ErrorCode::InvalidArgument, "region name is empty");
    if (!medium.valid() || medium.index() >= media_.size()) return fail(ErrorCode::UnknownMedium, name);
    if (std::ranges::any_of(regions_, [&](const RegionInfo& r) { return r.name == name; })) {
        return fail(ErrorCode::DuplicateName, name);
    }
    regions_.push_back({std::move(name), medium});
    return RegionId::from_index(regions_.size() - 1);
}

Result<RegionId> Declaration2D::add_region_outline(std::string name, MediumId medium, ShapeOutline outline) {
    auto id = new_region(std::move(name), medium);
    if (!id) return id;
    layers_.push_back({*id, std::move(outline)});
    return id;
}

Status Declaration2D::add_hole_outline(ShapeOutline outline) {
    layers_.push_back({RegionId::invalid(), std::move(outline)});
    return {};
}

Result<RegionId> Declaration2D::set_background(std::string name, MediumId medium) {
    if (background_.valid()) return fail(ErrorCode::DuplicateName, "background region already declared");
    auto id = new_region(std::move(name), medium);
    if (id) background_ = *id;
    return id;
}

}  // namespace vmm
