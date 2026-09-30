// ============================================================================
// File: random.hpp
// Description: Portable pseudo-random numbers. std::mt19937_64 is fully
//              specified by the standard; the distributions are written here
//              because std::uniform_*_distribution is implementation-defined.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <utility>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>

namespace vmm {

class Random {
public:
    explicit Random(std::uint64_t seed) noexcept : engine_(seed) {}

    /// Next raw 64-bit value.
    [[nodiscard]] std::uint64_t next() noexcept { return engine_(); }

    /// Uniform in [0, 1) with 53 random bits.
    [[nodiscard]] Real uniform() noexcept { return static_cast<Real>(engine_() >> 11) * 0x1.0p-53; }

    /// Uniform in [a, b).
    [[nodiscard]] Real uniform(Real a, Real b) noexcept { return a + (b - a) * uniform(); }

    /// Uniform integer in [0, n), unbiased (rejection). n must be > 0.
    [[nodiscard]] std::uint64_t below(std::uint64_t n) noexcept {
        const std::uint64_t limit = std::uint64_t(0) - (std::uint64_t(0) - n) % n;  // largest multiple of n
        for (;;) {
            const std::uint64_t x = engine_();
            if (limit == 0 || x < limit) return x % n;
        }
    }

    /// Fisher-Yates shuffle, identical on every platform.
    template <class T>
    void shuffle(std::span<T> values) noexcept {
        for (std::size_t k = values.size(); k > 1; --k) {
            std::swap(values[k - 1], values[static_cast<std::size_t>(below(k))]);
        }
    }

private:
    std::mt19937_64 engine_;
};

}  // namespace vmm
