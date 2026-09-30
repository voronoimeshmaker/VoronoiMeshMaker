// ============================================================================
// File: error_code.hpp
// Description: Stable error codes (data enum, DEC-024), categories and
//              severities. A code is never reused or renumbered.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstdint>

namespace vmm {

enum class ErrorCode : std::uint32_t {
    // core (1xx)
    InvalidArgument = 100,
    InvalidLengthScale = 101,
    IndexOutOfRange = 102,
    // domain (2xx)
    EmptyDeclaration = 200,
    DegenerateShape = 201,
    InvalidPolygon = 202,
    DomainVoid = 203,
    RegionEmptied = 204,
    RegionFragmented = 205,
    Sliver = 206,
    UnknownShape = 207,
    InvalidShapeParameter = 208,
    UnknownMedium = 209,
    DuplicateName = 210,
    // sites (3xx)
    SiteOutsideRegion = 300,
    DuplicateSite = 301,
    RegionWithoutSites = 302,
    SiteGenerationFailed = 303,
    InvalidSpacing = 304,
    // mesh (4xx)
    InvariantViolated = 400,
    FragmentWithoutNeighbour = 401,
    InterfaceNotConforming = 402,
    UnresolvedLabel = 403,
    // io (5xx)
    FileOpenFailed = 500,
    ParseError = 501,
    UnsupportedVersion = 502,
    InconsistentData = 503,
    // backend (6xx)
    BackendFailure = 600,
    // internal (9xx)
    InternalError = 900,
};

/// Category of a code: its hundreds digit.
enum class ErrorCategory : std::uint32_t {
    Core = 1,
    Domain = 2,
    Sites = 3,
    Mesh = 4,
    IO = 5,
    Backend = 6,
    Internal = 9,
};

enum class Severity : std::uint8_t { Info, Warning, Error, Fatal };

[[nodiscard]] constexpr ErrorCategory category_of(ErrorCode code) noexcept {
    return static_cast<ErrorCategory>(static_cast<std::uint32_t>(code) / 100);
}

}  // namespace vmm
