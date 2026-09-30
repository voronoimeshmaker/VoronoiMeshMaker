// ============================================================================
// File: ut_Exception.cpp
// Description: vmm::Exception (no base class) and raise().
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <exception>
#include <string>
#include <type_traits>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/error/exception.hpp>

namespace {

static_assert(!std::is_base_of_v<std::exception, vmm::Exception>, "DEC-016: no std::exception base");
static_assert(std::is_empty_v<vmm::Exception> == false);

TEST(Exception, HoldsTheErrorAndEnglishText) {
    const vmm::Exception ex(vmm::Error(vmm::ErrorCode::InternalError, "broken"));
    EXPECT_EQ(ex.error().code(), vmm::ErrorCode::InternalError);
    EXPECT_STREQ(ex.what(), "[VMM-900] internal error (invariant violated; please report): broken");
}

TEST(Exception, RaiseThrowsFatalWithLocation) {
    try {
        vmm::raise(vmm::ErrorCode::InternalError, "bug", vmm::CellId::from_index(9));
        FAIL() << "raise() returned";
    } catch (const vmm::Exception& ex) {
        EXPECT_EQ(ex.error().severity(), vmm::Severity::Fatal);
        EXPECT_EQ(ex.error().context(), "bug");
        EXPECT_NE(std::string(ex.error().where().file_name()).find("ut_Exception.cpp"), std::string::npos);
    }
}

}  // namespace
