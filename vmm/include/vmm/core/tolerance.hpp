// ============================================================================
// File: tolerance.hpp
// Description: Tolerances relative to the length scale L, the diagonal of the
//              domain's bounding box (R17, DEC-020). No absolute constants.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cmath>
#include <cstddef>
#include <optional>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>

namespace vmm {

class Tolerance {
public:
    static constexpr Real default_relative = 1e-12;

    /// nullopt unless L and the relative tolerance are finite and positive.
    [[nodiscard]] static std::optional<Tolerance> from_length(Real length_scale,
                                                              Real relative = default_relative) noexcept {
        if (!(std::isfinite(length_scale) && length_scale > 0 && std::isfinite(relative) && relative > 0)) {
            return std::nullopt;
        }
        return Tolerance(length_scale, relative);
    }

    [[nodiscard]] Real length_scale() const noexcept { return length_; }
    [[nodiscard]] Real relative() const noexcept { return relative_; }

    /// Distance below which two points are the same point: relative * L.
    [[nodiscard]] Real point() const noexcept { return relative_ * length_; }

    /// Tolerance for a quantity of dimension L^k (length k = 1, area k = 2, ...).
    [[nodiscard]] Real measure(int k) const noexcept { return relative_ * std::pow(length_, k); }

    template <std::size_t D>
    [[nodiscard]] bool same_point(const Vec<D>& a, const Vec<D>& b) const noexcept {
        return norm(a - b) <= point();
    }

private:
    Tolerance(Real length, Real relative) noexcept : length_(length), relative_(relative) {}

    Real length_;
    Real relative_;
};

}  // namespace vmm
