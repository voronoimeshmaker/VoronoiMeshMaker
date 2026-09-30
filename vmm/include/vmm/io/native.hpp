// ============================================================================
// File: native.hpp
// Description: Native VMM mesh format (DEC-019, P06 §10): versioned text,
//              exact round trip (17 significant digits), writer and reader.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <string>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/error/error.hpp>
#include <vmm/mesh/mesh.hpp>

namespace vmm {

inline constexpr int native_format_version = 1;

struct NativeWriteOptions {
    /// Free text recorded in the header (versions, flags...); one line.
    std::string generator;
};

template <std::size_t D>
[[nodiscard]] Status write_native(const Mesh<D>& mesh, std::ostream& out, const NativeWriteOptions& options = {});

template <std::size_t D>
[[nodiscard]] Status write_native(const Mesh<D>& mesh, const std::filesystem::path& path,
                                  const NativeWriteOptions& options = {});

/// Reads a mesh of dimension D; fails on a different dimension or version.
template <std::size_t D>
[[nodiscard]] Result<Mesh<D>> read_native(std::istream& in);

template <std::size_t D>
[[nodiscard]] Result<Mesh<D>> read_native(const std::filesystem::path& path);

extern template Status write_native(const Mesh<2>&, std::ostream&, const NativeWriteOptions&);
extern template Status write_native(const Mesh<3>&, std::ostream&, const NativeWriteOptions&);
extern template Status write_native(const Mesh<2>&, const std::filesystem::path&, const NativeWriteOptions&);
extern template Status write_native(const Mesh<3>&, const std::filesystem::path&, const NativeWriteOptions&);
extern template Result<Mesh<2>> read_native<2>(std::istream&);
extern template Result<Mesh<3>> read_native<3>(std::istream&);
extern template Result<Mesh<2>> read_native<2>(const std::filesystem::path&);
extern template Result<Mesh<3>> read_native<3>(const std::filesystem::path&);

}  // namespace vmm
