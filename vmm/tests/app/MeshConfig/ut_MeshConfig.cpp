// ============================================================================
// File: ut_MeshConfig.cpp
// Description: MeshConfig: parsing of the configuration format, errors with
//              line numbers, reading from a file.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/app/config.hpp>

namespace {

using vmm::MeshConfig;

constexpr std::string_view kMinimal = "dimension = 2\n[region a]\nshape = rectangle\n";

/// Parses and expects a ParseError whose context contains `what`.
void expect_error(std::string_view text, std::string_view what) {
    const auto c = MeshConfig::parse(text);
    ASSERT_FALSE(c) << text;
    EXPECT_EQ(c.error().code(), vmm::ErrorCode::ParseError) << text;
    EXPECT_NE(c.error().context().find(what), std::string::npos) << c.error().context();
}

TEST(MeshConfig, ParsesGlobalsSectionsAndComments) {
    const auto c = MeshConfig::parse("# a comment\r\n"
                                     "dimension = 3   # trailing comment\r\n"
                                     "seed = 42\n"
                                     "output = out/mesh\n"
                                     "formats = vtu\n"
                                     "interface_pairs = 0.1\n"
                                     "tolerance = 1e-10\n"
                                     "\n"
                                     "[ region   rock layer ]\n"
                                     "  shape =  cuboid \n"
                                     "lo = 0 0 0\n"
                                     "[hole]\n"
                                     "shape = sphere\n"
                                     "[background air]\n",
                                     "/base");
    ASSERT_TRUE(c) << c.error().message();
    EXPECT_EQ(c->dimension(), 3);
    EXPECT_EQ(c->seed(), 42u);
    EXPECT_EQ(c->output(), std::filesystem::path("/base/out/mesh"));
    EXPECT_EQ(c->formats(), std::vector<std::string>{"vtu"});
    EXPECT_EQ(c->interface_pairs(), 0.1);
    EXPECT_EQ(c->tolerance(), 1e-10);
    EXPECT_EQ(c->base_directory(), std::filesystem::path("/base"));
    EXPECT_EQ(c->global().entries.size(), 6u);
    ASSERT_EQ(c->sections().size(), 3u);
    const auto& rock = c->sections()[0];
    EXPECT_EQ(rock.kind, "region");
    EXPECT_EQ(rock.name, "rock layer");
    EXPECT_EQ(rock.line, 9u);
    ASSERT_EQ(rock.entries.size(), 2u);
    EXPECT_EQ(rock.entries[0].key, "shape");
    EXPECT_EQ(rock.entries[0].value, "cuboid");
    EXPECT_EQ(rock.entries[1].line, 11u);
    EXPECT_EQ(c->sections()[1].kind, "hole");
    EXPECT_TRUE(c->sections()[1].name.empty());
    EXPECT_EQ(c->sections()[2].name, "air");
}

TEST(MeshConfig, Defaults) {
    auto c = MeshConfig::parse(kMinimal);
    ASSERT_TRUE(c) << c.error().message();
    EXPECT_EQ(c->dimension(), 2);
    EXPECT_EQ(c->seed(), 0u);
    EXPECT_EQ(c->output(), std::filesystem::path("mesh"));
    EXPECT_EQ(c->formats(), (std::vector<std::string>{"vmesh", "vtu"}));
    EXPECT_FALSE(c->interface_pairs());
    EXPECT_FALSE(c->tolerance());
    c->set_output("elsewhere/m");
    EXPECT_EQ(c->output(), std::filesystem::path("elsewhere/m"));
}

TEST(MeshConfig, SyntaxErrorsNameTheLine) {
    expect_error("dimension = 2\n[region a\n", "line 2: a section header ends with ']'");
    expect_error("dimension = 2\n[domain a]\n", "line 2: unknown section 'domain'");
    expect_error("dimension = 2\n[region]\n", "line 2: [region] needs a name");
    expect_error("dimension = 2\n[background]\n", "[background] needs a name");
    expect_error("dimension = 2\njust words\n", "line 2: expected 'key = value'");
    expect_error("dimension = 2\n = 3\n", "line 2: a key and a value");
    expect_error("dimension = 2\nseed =\n", "line 2: a key and a value");
    expect_error("dimension = 2\n[region a]\nshape = x\nshape = y\n", "line 4: 'shape' is repeated");
}

TEST(MeshConfig, GlobalKeysAreChecked) {
    expect_error("dimension = 4\n[region a]\n", "line 1: dimension is 2 or 3");
    expect_error("dimension = 2\nseed = -1\n[region a]\n", "line 2: seed is a non-negative integer");
    expect_error("dimension = 2\nseed = 3x\n[region a]\n", "seed is a non-negative integer");
    expect_error("dimension = 2\ntolerance = 0\n[region a]\n", "tolerance is a positive number");
    expect_error("dimension = 2\ninterface_pairs = abc\n[region a]\n", "interface_pairs is a positive number");
    expect_error("dimension = 2\ninterface_pairs = 1 2\n[region a]\n", "interface_pairs is a positive number");
    expect_error("dimension = 2\nsed = 1\n[region a]\n", "line 2: unknown key 'sed'");
    expect_error("[region a]\n", "'dimension = 2' or 'dimension = 3' is required");
    expect_error("dimension = 3\n[hole]\nshape = sphere\n", "no [region] section");
}

TEST(MeshConfig, ReadsAFileAndResolvesPathsFromItsDirectory) {
    const auto dir = std::filesystem::temp_directory_path() / "vmm_ut_meshconfig";
    std::filesystem::create_directories(dir);
    const auto file = dir / "block.cfg";
    std::ofstream(file) << kMinimal;
    const auto c = MeshConfig::read(file);
    ASSERT_TRUE(c) << c.error().message();
    EXPECT_EQ(c->base_directory(), dir);
    EXPECT_EQ(c->output(), dir / "block");  // default: the file name
    std::ofstream(file) << kMinimal << "[region a]\n";
    const auto repeated = MeshConfig::read(file);
    std::filesystem::remove_all(dir);
    ASSERT_TRUE(repeated);  // repeated names are checked by the declaration, not by the parser
    const auto missing = MeshConfig::read(dir / "none.cfg");
    ASSERT_FALSE(missing);
    EXPECT_EQ(missing.error().code(), vmm::ErrorCode::FileOpenFailed);
}

TEST(MeshConfig, ReadErrorsNameTheFile) {
    const auto file = std::filesystem::temp_directory_path() / "vmm_ut_meshconfig_bad.cfg";
    std::ofstream(file) << "dimension = 5\n";
    const auto c = MeshConfig::read(file);
    std::filesystem::remove(file);
    ASSERT_FALSE(c);
    EXPECT_EQ(c.error().code(), vmm::ErrorCode::ParseError);
    EXPECT_EQ(c.error().context(), file.string() + ": line 1: dimension is 2 or 3");
}

TEST(MeshConfig, OutputKeyWinsOverTheFileName) {
    const auto file = std::filesystem::temp_directory_path() / "vmm_ut_meshconfig_out.cfg";
    std::ofstream(file) << "output = named\n" << kMinimal;
    const auto c = MeshConfig::read(file);
    std::filesystem::remove(file);
    ASSERT_TRUE(c) << c.error().message();
    EXPECT_EQ(c->output(), file.parent_path() / "named");
}

}  // namespace
