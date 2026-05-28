#pragma once
//==============================================================================
// Name        : constants.h
// Project     : VoronoiMeshMaker (VMM)
// Author      : Joao Flavio Vieira de Vasconcellos
// Version     : 1.3
// Description : Numerical constants (thresholds and defaults) for the core of
//               VoronoiMeshMaker.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file constants.h
 * @brief Numerical constants used across the library.
 *
 * All constants live in `vmm::constants` and carry a `k` prefix.
 * The legacy all-caps names (`ZERO`, `LIMIT`, `EPSILON`, `LSIZE`, `PI`) are
 * kept for backward compatibility but are **deprecated** — prefer the
 * `vmm::constants::k*` forms in all new code.
 *
 * This header intentionally contains no logic or mutable state.
 *
 * @ingroup core
 */

 
//==============================================================================
//  C++ standard library
//==============================================================================
#include <limits>   // std::numeric_limits
#include <numbers>  // std::numbers::pi_v

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <VoronoiMeshMaker/Core/type.h>  // for Real

VORMAKER_NAMESPACE_OPEN

/**
 * @namespace vmm::constants
 * @brief Internal namespace for numerical constants.
 * @ingroup core
 *
 * Keeping constants in a dedicated namespace avoids polluting the main API
 * while preserving simple, short names for frequent use inside the library.
 */
CONSTANTS_NAMESPACE_OPEN

//------------------------------------------------------------------------------
// Typed constants  — prefer these in all new code
//------------------------------------------------------------------------------

/**
 * @brief Numerical zero tolerance (very small threshold).
 *
 * Used as the default lower bound when testing whether a floating-point value
 * is "effectively zero". For IEEE double, 1e-12 is well above the rounding
 * floor of typical CGAL inexact constructions while still representing a
 * physically negligible length.
 */
inline constexpr Real kZeroTol = static_cast<Real>(1e-12);

/**
 * @brief Small limit threshold for conservative comparisons.
 *
 * Intended for clamping borderline-negative areas or volumes to zero without
 * treating genuine errors as valid results (e.g. `area = std::max(kLimit, raw_area)`).
 */
inline constexpr Real kLimit = static_cast<Real>(1e-30);

/**
 * @brief Epsilon for equality checks in non-critical geometric predicates.
 *
 * Use tighter or looser values where the algorithm warrants it; this default
 * is suitable for containment and clipping tests at unit-scale geometry.
 */
inline constexpr Real kEpsilon = static_cast<Real>(1e-6);

/**
 * @brief Default small container/buffer size used in utilities.
 *
 * The hex value 0x50 == 80.  Chosen as a round number that fits typical
 * Voronoi cell neighbour counts with room to spare.
 */
inline constexpr int kDefaultSize = 0x50;

/**
 * @brief Compile-time constant for π (pi).
 *
 * Derived from `std::numbers::pi_v<long double>` for maximum precision,
 * then narrowed to `Real`.
 */
inline constexpr Real kPi =
    static_cast<Real>(std::numbers::pi_v<long double>);

CONSTANTS_NAMESPACE_CLOSE

//==============================================================================
//  Backward-compatible aliases
//  These exist only to avoid breaking existing code that uses the old names.
//  They will be removed in a future major version — migrate to vmm::constants::k*.
//==============================================================================

/** @deprecated Use `vmm::constants::kZeroTol`. */
[[deprecated("Use vmm::constants::kZeroTol")]]
inline constexpr Real ZERO    = constants::kZeroTol;

/** @deprecated Use `vmm::constants::kLimit`. */
[[deprecated("Use vmm::constants::kLimit")]]
inline constexpr Real LIMIT   = constants::kLimit;

/** @deprecated Use `vmm::constants::kEpsilon`. */
[[deprecated("Use vmm::constants::kEpsilon")]]
inline constexpr Real EPSILON = constants::kEpsilon;

/** @deprecated Use `vmm::constants::kDefaultSize`. */
[[deprecated("Use vmm::constants::kDefaultSize")]]
inline constexpr int  LSIZE   = constants::kDefaultSize;

/** @deprecated Use `vmm::constants::kPi`. */
[[deprecated("Use vmm::constants::kPi")]]
inline constexpr Real PI      = constants::kPi;

//==============================================================================
//  Compile-time sanity checks
//==============================================================================

static_assert(
    std::numeric_limits<Real>::is_iec559 ||
    std::numeric_limits<Real>::is_specialized,
    "vmm::Real must be a floating-point-like type.");

static_assert(
    constants::kZeroTol > Real{0},
    "kZeroTol must be strictly positive.");

static_assert(
    constants::kEpsilon > Real{0},
    "kEpsilon must be strictly positive.");

static_assert(
    constants::kLimit <= constants::kZeroTol,
    "kLimit should be <= kZeroTol (kLimit is the tighter threshold).");

VORMAKER_NAMESPACE_CLOSE
