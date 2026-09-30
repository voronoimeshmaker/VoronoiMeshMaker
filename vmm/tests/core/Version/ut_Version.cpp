// ============================================================================
// File: ut_Version.cpp
// Description: vmm/core/version.hpp equals the version of the CMake project
//              (DEC-041), and the string matches the numbers.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <format>
#include <string_view>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/version.hpp>

namespace {

TEST(Version, EqualsTheCMakeProject) { EXPECT_EQ(vmm::version_string, std::string_view(VMM_PROJECT_VERSION)); }

TEST(Version, StringMatchesTheNumbers) {
    EXPECT_EQ(vmm::version_string,
              std::format("{}.{}.{}", vmm::version_major, vmm::version_minor, vmm::version_patch));
}

}  // namespace
