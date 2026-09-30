// ============================================================================
// File: types.hpp
// Description: Scalar type, strong ids and fixed-size vectors of the data model.
// SPDX-License-Identifier: BSD-3-Clause
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

namespace vmm {

/// Floating-point type of the whole library (R20). Never a CGAL number type.
using Real = double;

/// Strong 32-bit index. The tag makes ids of different entities
/// non-interchangeable; the maximum value is reserved for "invalid".
template <class Tag>
struct Id {
    using value_type = std::uint32_t;
    value_type value = std::numeric_limits<value_type>::max();

    [[nodiscard]] static constexpr Id invalid() noexcept { return Id{}; }
    [[nodiscard]] static constexpr Id from_index(std::size_t i) noexcept { return Id{static_cast<value_type>(i)}; }
    [[nodiscard]] constexpr bool valid() const noexcept { return value != std::numeric_limits<value_type>::max(); }
    [[nodiscard]] constexpr std::size_t index() const noexcept { return value; }
    friend constexpr auto operator<=>(Id, Id) = default;
};

struct CellTag {};
struct FaceTag {};
struct VertexTag {};
struct RegionTag {};
struct MediumTag {};
struct PatchTag {};
struct SiteTag {};

using CellId = Id<CellTag>;
using FaceId = Id<FaceTag>;
using VertexId = Id<VertexTag>;
using RegionId = Id<RegionTag>;
using MediumId = Id<MediumTag>;
using PatchId = Id<PatchTag>;
using SiteId = Id<SiteTag>;

/// Fixed-size vector. A distinct type (not an alias of std::array) so that the
/// arithmetic operators below are found by argument-dependent lookup.
template <std::size_t D>
struct Vec {
    std::array<Real, D> data{};

    [[nodiscard]] constexpr Real& operator[](std::size_t k) noexcept { return data[k]; }
    [[nodiscard]] constexpr const Real& operator[](std::size_t k) const noexcept { return data[k]; }
    [[nodiscard]] static constexpr std::size_t size() noexcept { return D; }
    [[nodiscard]] constexpr auto begin() noexcept { return data.begin(); }
    [[nodiscard]] constexpr auto end() noexcept { return data.end(); }
    [[nodiscard]] constexpr auto begin() const noexcept { return data.begin(); }
    [[nodiscard]] constexpr auto end() const noexcept { return data.end(); }
    friend constexpr bool operator==(const Vec&, const Vec&) = default;
    friend constexpr auto operator<=>(const Vec&, const Vec&) = default;
};

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

[[nodiscard]] constexpr Real cross(const Vec2& a, const Vec2& b) noexcept { return a[0] * b[1] - a[1] * b[0]; }

[[nodiscard]] constexpr Vec3 cross(const Vec3& a, const Vec3& b) noexcept {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

/// Angle between two vectors in [0, pi], accurate for nearly parallel vectors.
template <std::size_t D>
[[nodiscard]] inline Real angle_between(const Vec<D>& a, const Vec<D>& b) noexcept {
    if constexpr (D == 2) {
        return std::atan2(std::abs(cross(a, b)), dot(a, b));
    } else {
        return std::atan2(norm(cross(a, b)), dot(a, b));
    }
}

}  // namespace vmm
