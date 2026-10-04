// SPDX-License-Identifier: BSD-3-Clause
#pragma once
//==============================================================================
//  C++ standard library
//==============================================================================
#include <functional>
#include <span>
#include <string>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/error/error.hpp>
namespace vmm {
/// Fixed, piecewise affine horizons. Each rectangle has its SW--NE diagonal.
/// Elevations use x-fast node order and increase from bottom to top. Equality
/// is allowed (pinch-out); inversion is not. The grid is independent of sites.
class HorizonGrid {
public:
    using Surface = std::function<Real(const Vec2&)>;
    /// Parameters: increasing axes, unique names, one elevation array per horizon.
    /// Notes: freezes the geometry; resampling after moving sites is not allowed.
    /// Level: Intermediate. See Also: generate_layered_mesh.
    /// Location: vmm/geometry/horizons.hpp.
    [[nodiscard]] static Result<HorizonGrid> make(std::vector<Real> x, std::vector<Real> y,
        std::vector<std::string> names, std::vector<std::vector<Real>> elevations);
    /// Samples functions once at grid nodes, then uses the same affine representation.
    [[nodiscard]] static Result<HorizonGrid> sample(std::vector<Real> x, std::vector<Real> y,
        std::vector<std::string> names, std::span<const Surface> surfaces);
    [[nodiscard]] const std::vector<Real>& x() const noexcept { return x_; }
    [[nodiscard]] const std::vector<Real>& y() const noexcept { return y_; }
    [[nodiscard]] const std::vector<std::string>& names() const noexcept { return names_; }
    [[nodiscard]] const std::vector<std::vector<Real>>& elevations() const noexcept { return z_; }
    [[nodiscard]] std::size_t horizon_count() const noexcept { return z_.size(); }
private:
    std::vector<Real> x_, y_;
    std::vector<std::string> names_;
    std::vector<std::vector<Real>> z_;
};
} // namespace vmm
