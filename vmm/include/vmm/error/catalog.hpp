// ============================================================================
// File: catalog.hpp
// Description: Message catalogue: every error code has a Portuguese and an
//              English text (R10). The display language is process-wide.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/error/error_code.hpp>

namespace vmm {

enum class Language : std::uint8_t { Portuguese, English };

struct CatalogEntry {
    std::string_view pt;
    std::string_view en;

    [[nodiscard]] constexpr std::string_view text(Language l) const noexcept {
        return l == Language::Portuguese ? pt : en;
    }
};

/// Entry of a code, or nullopt for a value outside the catalogue.
[[nodiscard]] std::optional<CatalogEntry> catalog_entry(ErrorCode code) noexcept;

/// Every code of the catalogue, in increasing order.
[[nodiscard]] std::span<const ErrorCode> all_error_codes() noexcept;

void set_language(Language language) noexcept;
[[nodiscard]] Language current_language() noexcept;

}  // namespace vmm
