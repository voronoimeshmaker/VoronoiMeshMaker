// ============================================================================
// File: version.hpp
// Description: Library version (semantic versioning, DEC-041). A test keeps
//              it equal to the version of the CMake project.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <string_view>

namespace vmm {

inline constexpr int version_major = 1;
inline constexpr int version_minor = 0;
inline constexpr int version_patch = 0;
inline constexpr std::string_view version_string = "1.0.0";

}  // namespace vmm
