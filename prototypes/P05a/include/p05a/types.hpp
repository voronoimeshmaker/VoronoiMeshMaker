// ============================================================================
// File: types.hpp
// Description: P05a prototype - scalar, strong ids and fixed-size vectors.
//              Throwaway proof-of-concept code (DEC-027); no stable API.
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <array>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace vmm::p05a {

/// Floating-point type of the data model. Never a CGAL number type (DEC-007).
using Real = double;

/// Strong index: the tag makes CellId and FaceId non-interchangeable.
template <class Tag>
struct Id {
    std::uint32_t value = std::numeric_limits<std::uint32_t>::max();

    [[nodiscard]] static constexpr Id invalid() noexcept { return Id{}; }
    [[nodiscard]] constexpr bool valid() const noexcept {
        return value != std::numeric_limits<std::uint32_t>::max();
    }
    [[nodiscard]] constexpr std::size_t index() const noexcept { return value; }
    friend constexpr auto operator<=>(Id, Id) = default;
};

struct CellTag {};
struct FaceTag {};
struct RegionTag {};
struct PatchTag {};

using CellId = Id<CellTag>;
using FaceId = Id<FaceTag>;
using RegionId = Id<RegionTag>;
using PatchId = Id<PatchTag>;

template <std::size_t D>
using Vec = std::array<Real, D>;

using Vec2 = Vec<2>;
using Vec3 = Vec<3>;

template <std::size_t D>
[[nodiscard]] constexpr Vec<D> operator+(const Vec<D>& a, const Vec<D>& b) noexcept {
    Vec<D> r{};
    for (std::size_t k = 0; k < D; ++k) r[k] = a[k] + b[k];
    return r;
}

template <std::size_t D>
[[nodiscard]] constexpr Vec<D> operator-(const Vec<D>& a, const Vec<D>& b) noexcept {
    Vec<D> r{};
    for (std::size_t k = 0; k < D; ++k) r[k] = a[k] - b[k];
    return r;
}

template <std::size_t D>
[[nodiscard]] constexpr Vec<D> operator*(Real s, const Vec<D>& a) noexcept {
    Vec<D> r{};
    for (std::size_t k = 0; k < D; ++k) r[k] = s * a[k];
    return r;
}

template <std::size_t D>
[[nodiscard]] constexpr Real dot(const Vec<D>& a, const Vec<D>& b) noexcept {
    Real r = 0;
    for (std::size_t k = 0; k < D; ++k) r += a[k] * b[k];
    return r;
}

template <std::size_t D>
[[nodiscard]] inline Real norm(const Vec<D>& a) noexcept {
    return std::sqrt(dot(a, a));
}

[[nodiscard]] constexpr Real cross(const Vec2& a, const Vec2& b) noexcept {
    return a[0] * b[1] - a[1] * b[0];
}

[[nodiscard]] constexpr Vec3 cross(const Vec3& a, const Vec3& b) noexcept {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

/// Angle between two vectors in [0, pi], robust for nearly parallel vectors.
template <std::size_t D>
[[nodiscard]] inline Real angle_between(const Vec<D>& a, const Vec<D>& b) noexcept {
    const Real c = dot(a, b);
    Real s = 0;
    if constexpr (D == 2) {
        s = std::abs(cross(a, b));
    } else {
        s = norm(cross(a, b));
    }
    return std::atan2(s, c);
}

}  // namespace vmm::p05a
