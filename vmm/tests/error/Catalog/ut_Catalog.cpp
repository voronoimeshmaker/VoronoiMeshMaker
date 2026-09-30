// ============================================================================
// File: ut_Catalog.cpp
// Description: Message catalogue: every code has pt and en text (R10);
//              codes sorted and unique; language switch.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstdint>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/error/catalog.hpp>

namespace {

TEST(Catalog, EveryCodeHasBothTexts) {
    const auto codes = vmm::all_error_codes();
    ASSERT_FALSE(codes.empty());
    for (const auto code : codes) {
        const auto entry = vmm::catalog_entry(code);
        ASSERT_TRUE(entry) << static_cast<std::uint32_t>(code);
        EXPECT_FALSE(entry->pt.empty()) << static_cast<std::uint32_t>(code);
        EXPECT_FALSE(entry->en.empty()) << static_cast<std::uint32_t>(code);
        EXPECT_NE(entry->pt, entry->en) << static_cast<std::uint32_t>(code);
    }
}

TEST(Catalog, CodesAreSortedAndUnique) {
    const auto codes = vmm::all_error_codes();
    EXPECT_TRUE(std::ranges::is_sorted(codes));
    EXPECT_EQ(std::ranges::adjacent_find(codes), codes.end());
}

TEST(Catalog, UnknownCodeHasNoEntry) {
    EXPECT_FALSE(vmm::catalog_entry(static_cast<vmm::ErrorCode>(1)));
    EXPECT_FALSE(vmm::catalog_entry(static_cast<vmm::ErrorCode>(99999)));
}

TEST(Catalog, CategoryIsTheHundreds) {
    EXPECT_EQ(vmm::category_of(vmm::ErrorCode::InvalidArgument), vmm::ErrorCategory::Core);
    EXPECT_EQ(vmm::category_of(vmm::ErrorCode::BackendFailure), vmm::ErrorCategory::Backend);
    EXPECT_EQ(vmm::category_of(vmm::ErrorCode::InternalError), vmm::ErrorCategory::Internal);
}

TEST(Catalog, LanguageSwitch) {
    vmm::set_language(vmm::Language::English);
    EXPECT_EQ(vmm::current_language(), vmm::Language::English);
    EXPECT_EQ(vmm::catalog_entry(vmm::ErrorCode::DuplicateName)->text(vmm::current_language()), "duplicate name");
    vmm::set_language(vmm::Language::Portuguese);
    EXPECT_EQ(vmm::catalog_entry(vmm::ErrorCode::DuplicateName)->text(vmm::current_language()), "nome repetido");
}

}  // namespace
