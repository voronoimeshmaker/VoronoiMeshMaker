// ============================================================================
// File: config.hpp
// Description: Mesh configuration files (DEC-040): a small "key = value"
//              text format with [region], [hole] and [background] sections,
//              read by the vmm-mesh executable and by run_config.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/error/error.hpp>

namespace vmm {

/// One "key = value" line of a configuration file.
struct ConfigEntry {
    std::string key;
    std::string value;
    std::size_t line = 0;
};

/// One "[kind name]" block; the global block (before the first header) has an empty kind.
struct ConfigSection {
    std::string kind;
    std::string name;
    std::size_t line = 0;
    std::vector<ConfigEntry> entries;

    /// Value of `key`, or nullopt.
    [[nodiscard]] std::optional<std::string_view> find(std::string_view key) const;
};

/// @brief A parsed mesh configuration file (DEC-040).
/// @note Format, one statement per line; `#` starts a comment:
/// @code
/// dimension = 2              # 2 or 3 (required)
/// seed = 42                  # optional: seed of the site generator
/// output = mesh              # optional: base name of the output files (default: the file's name)
/// formats = vmesh vtu        # optional: output formats (default: vmesh vtu)
///
/// [region soil]              # a region; later regions cover earlier ones
/// medium = soil              # optional: medium name (default: the region name)
/// shape = rectangle          # a shape of the registry, with its parameters
/// lo = 0 0
/// hi = 2 1
/// sites = uniform            # a site source of the registry, with its parameters
/// sites.spacing = 0.05
///
/// [hole]                     # a hole: a shape and its parameters
/// shape = circle
/// center = 1 0.5
/// radius = 0.1
/// @endcode
/// Global keys: dimension, seed, output, formats, interface_pairs (spacing of the mirrored pairs,
/// optional), tolerance (relative point tolerance, optional). Sections: region (name required), hole
/// (name optional) and, in 2D only, background (the region that fills every void).
/// @par Level
/// Beginner
/// @sa run_config, make_request_2d, make_request_3d
/// @par Location
/// vmm/app/config.hpp
class MeshConfig {
public:
    /// @brief Parses the text of a configuration file.
    /// @param text File content.
    /// @param base_directory Directory against which relative paths (STL files, output) are resolved.
    /// @return The configuration, or ParseError naming the line (syntax, repeated key, unknown global key or
    ///         section kind, missing or invalid dimension).
    [[nodiscard]] static Result<MeshConfig> parse(std::string_view text, std::filesystem::path base_directory = {});

    /// @brief Reads and parses a configuration file; relative paths are resolved from its directory, and the
    ///        default output name is the file name without extension.
    /// @return The configuration, FileOpenFailed or ParseError.
    [[nodiscard]] static Result<MeshConfig> read(const std::filesystem::path& file);

    [[nodiscard]] int dimension() const noexcept { return dimension_; }
    [[nodiscard]] std::uint64_t seed() const noexcept { return seed_; }
    /// Base path of the output files, resolved against the base directory.
    [[nodiscard]] std::filesystem::path output() const;
    /// Replaces the base path of the output files (relative paths are resolved against the base directory).
    void set_output(std::filesystem::path output) { output_ = std::move(output); }
    [[nodiscard]] const std::vector<std::string>& formats() const noexcept { return formats_; }
    [[nodiscard]] std::optional<Real> interface_pairs() const noexcept { return interface_pairs_; }
    [[nodiscard]] std::optional<Real> tolerance() const noexcept { return tolerance_; }
    [[nodiscard]] const std::filesystem::path& base_directory() const noexcept { return base_directory_; }

    /// The global block (keys before the first section header).
    [[nodiscard]] const ConfigSection& global() const noexcept { return global_; }
    /// The [region], [hole] and [background] blocks, in file order.
    [[nodiscard]] const std::vector<ConfigSection>& sections() const noexcept { return sections_; }

private:
    ConfigSection global_;
    std::vector<ConfigSection> sections_;
    std::filesystem::path base_directory_;
    std::filesystem::path output_ = "mesh";
    std::vector<std::string> formats_{"vmesh", "vtu"};
    std::uint64_t seed_ = 0;
    std::optional<Real> interface_pairs_;
    std::optional<Real> tolerance_;
    int dimension_ = 0;
};

}  // namespace vmm
