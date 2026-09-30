// ============================================================================
// File: ut_Exception.cpp
// Description: vmm::Exception (no base class), raise() and value_or_throw().
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <exception>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

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

vmm::Result<int> twice(int x) {
    if (x < 0) return vmm::fail(vmm::ErrorCode::InvalidArgument, "negative");
    return 2 * x;
}

TEST(ValueOrThrow, ReturnsTheValueOfAPrvalue) {
    EXPECT_EQ(vmm::value_or_throw(twice(4)), 8);
    static_assert(std::is_same_v<decltype(vmm::value_or_throw(twice(1))), int>);
}

TEST(ValueOrThrow, ThrowsTheErrorOfAFailure) {
    try {
        (void)vmm::value_or_throw(twice(-1));
        FAIL() << "value_or_throw() returned";
    } catch (const vmm::Exception& ex) {
        EXPECT_EQ(ex.error().code(), vmm::ErrorCode::InvalidArgument);
        EXPECT_EQ(ex.error().context(), "negative");
        EXPECT_STREQ(ex.what(), ex.error().message(vmm::Language::English).c_str());
    }
}

TEST(ValueOrThrow, LvaluesGiveReferencesWithoutCopy) {
    vmm::Result<std::string> r = std::string("mesh");
    std::string& ref = vmm::value_or_throw(r);
    EXPECT_EQ(&ref, &*r);
    const auto& cr = r;
    EXPECT_EQ(&vmm::value_or_throw(cr), &*r);
    const vmm::Result<std::string> bad = vmm::fail(vmm::ErrorCode::ParseError);
    EXPECT_THROW((void)vmm::value_or_throw(bad), vmm::Exception);
    vmm::Result<std::string> bad_mutable = vmm::fail(vmm::ErrorCode::ParseError);
    EXPECT_THROW((void)vmm::value_or_throw(bad_mutable), vmm::Exception);
}

TEST(ValueOrThrow, MovesMoveOnlyValues) {
    vmm::Result<std::unique_ptr<int>> r = std::make_unique<int>(7);
    const std::unique_ptr<int> p = vmm::value_or_throw(std::move(r));
    EXPECT_EQ(*p, 7);
}

TEST(ValueOrThrow, ChecksAStatus) {
    EXPECT_NO_THROW(vmm::value_or_throw(vmm::Status{}));
    const vmm::Status bad = vmm::fail(vmm::ErrorCode::FileOpenFailed, "x.vtu");
    EXPECT_THROW(vmm::value_or_throw(bad), vmm::Exception);
}

}  // namespace
