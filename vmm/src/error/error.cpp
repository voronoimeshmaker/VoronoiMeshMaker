// ============================================================================
// File: error.cpp
// Description: Error message rendering.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstdint>
#include <format>
#include <string>
#include <type_traits>
#include <variant>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/error/error.hpp>

namespace vmm {
namespace {

std::string entity_text(const EntityRef& entity, Language language) {
    const bool pt = language == Language::Portuguese;
    return std::visit(
        [pt](const auto& id) -> std::string {
            using T = std::decay_t<decltype(id)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return {};
            } else {
                const char* name = std::is_same_v<T, SiteId>   ? (pt ? "sítio" : "site")
                                   : std::is_same_v<T, CellId> ? (pt ? "célula" : "cell")
                                   : std::is_same_v<T, FaceId> ? (pt ? "face" : "face")
                                   : std::is_same_v<T, RegionId> ? (pt ? "região" : "region")
                                                                 : "patch";
                return std::format(" ({} {})", name, id.value);
            }
        },
        entity);
}

}  // namespace

std::string Error::message(Language language) const {
    const auto entry = catalog_entry(code_);
    const std::string_view text = entry ? entry->text(language) : std::string_view("?");
    std::string out = std::format("[VMM-{}] {}", static_cast<std::uint32_t>(code_), text);
    if (!context_.empty()) out += ": " + context_;
    out += entity_text(entity_, language);
    return out;
}

}  // namespace vmm
