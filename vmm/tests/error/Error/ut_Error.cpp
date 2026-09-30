// ============================================================================
// File: ut_Error.cpp
// Description: Error value, Result<T> and fail().
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <string>
#include <variant>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/error/error.hpp>

namespace {

vmm::Result<int> half(int x) {
    if (x % 2 != 0) return vmm::fail(vmm::ErrorCode::InvalidArgument, "odd value", vmm::SiteId::from_index(7));
    return x / 2;
}

TEST(Error, CarriesCodeCategoryContextEntityAndLocation) {
    const vmm::Error e(vmm::ErrorCode::DuplicateSite, "x = 1", vmm::SiteId::from_index(3), vmm::Severity::Warning);
    EXPECT_EQ(e.code(), vmm::ErrorCode::DuplicateSite);
    EXPECT_EQ(e.category(), vmm::ErrorCategory::Sites);
    EXPECT_EQ(e.severity(), vmm::Severity::Warning);
    EXPECT_EQ(e.context(), "x = 1");
    ASSERT_TRUE(std::holds_alternative<vmm::SiteId>(e.entity()));
    EXPECT_EQ(std::get<vmm::SiteId>(e.entity()).value, 3u);
    EXPECT_NE(std::string(e.where().file_name()).find("ut_Error.cpp"), std::string::npos);
}

TEST(Error, MessageInBothLanguages) {
    const vmm::Error e(vmm::ErrorCode::DuplicateSite, "x = 1", vmm::SiteId::from_index(3));
    EXPECT_EQ(e.message(vmm::Language::English), "[VMM-301] duplicate site: x = 1 (site 3)");
    EXPECT_EQ(e.message(vmm::Language::Portuguese), "[VMM-301] sítio repetido: x = 1 (sítio 3)");
}

TEST(Error, MessageEntities) {
    using vmm::Language;
    EXPECT_EQ(vmm::Error(vmm::ErrorCode::InternalError).message(Language::English),
              "[VMM-900] internal error (invariant violated; please report)");
    EXPECT_NE(vmm::Error(vmm::ErrorCode::InvariantViolated, "", vmm::CellId::from_index(1)).message(Language::English).find("(cell 1)"), std::string::npos);
    EXPECT_NE(vmm::Error(vmm::ErrorCode::InvariantViolated, "", vmm::FaceId::from_index(2)).message(Language::Portuguese).find("(face 2)"), std::string::npos);
    EXPECT_NE(vmm::Error(vmm::ErrorCode::RegionEmptied, "", vmm::RegionId::from_index(4)).message(Language::Portuguese).find("(região 4)"), std::string::npos);
    EXPECT_NE(vmm::Error(vmm::ErrorCode::Sliver, "", vmm::PatchId::from_index(5)).message(Language::English).find("(patch 5)"), std::string::npos);
    EXPECT_NE(vmm::Error(vmm::ErrorCode::Sliver, "", vmm::CellId::from_index(1)).message(Language::Portuguese).find("(célula 1)"), std::string::npos);
    EXPECT_NE(vmm::Error(vmm::ErrorCode::Sliver, "", vmm::RegionId::from_index(1)).message(Language::English).find("(region 1)"), std::string::npos);
    EXPECT_NE(vmm::Error(vmm::ErrorCode::Sliver, "", vmm::SiteId::from_index(1)).message(Language::Portuguese).find("(sítio 1)"), std::string::npos);
    EXPECT_NE(vmm::Error(static_cast<vmm::ErrorCode>(12345)).message(Language::English).find("?"), std::string::npos);
}

TEST(Error, DefaultMessageUsesCurrentLanguage) {
    vmm::set_language(vmm::Language::English);
    EXPECT_EQ(vmm::Error(vmm::ErrorCode::ParseError).message(), "[VMM-501] file parse error");
    vmm::set_language(vmm::Language::Portuguese);
    EXPECT_EQ(vmm::Error(vmm::ErrorCode::ParseError).message(), "[VMM-501] erro de leitura do arquivo");
}

TEST(Error, ResultCarriesValueOrError) {
    const auto ok = half(4);
    ASSERT_TRUE(ok);
    EXPECT_EQ(*ok, 2);
    const auto bad = half(3);
    ASSERT_FALSE(bad);
    EXPECT_EQ(bad.error().code(), vmm::ErrorCode::InvalidArgument);
    EXPECT_EQ(std::get<vmm::SiteId>(bad.error().entity()).value, 7u);
    const vmm::Status done{};
    EXPECT_TRUE(done);
}

}  // namespace
