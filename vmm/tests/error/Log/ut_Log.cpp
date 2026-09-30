// ============================================================================
// File: ut_Log.cpp
// Description: Logging facade (callback sink).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/error/log.hpp>

namespace {

TEST(Log, SinkReceivesDiagnosticsUntilRemoved) {
    std::vector<vmm::ErrorCode> seen;
    vmm::set_log_sink([&](const vmm::Error& e) { seen.push_back(e.code()); });
    vmm::log(vmm::Error(vmm::ErrorCode::RegionFragmented, "", {}, vmm::Severity::Warning));
    vmm::set_log_sink({});
    vmm::log(vmm::Error(vmm::ErrorCode::Sliver));
    ASSERT_EQ(seen.size(), 1u);
    EXPECT_EQ(seen[0], vmm::ErrorCode::RegionFragmented);
}

}  // namespace
