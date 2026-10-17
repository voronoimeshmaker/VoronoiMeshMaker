// ============================================================================
// File: stl.cpp
// Description: STL reader and writer.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <format>
#include <fstream>
#include <iterator>
#include <limits>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/io/stl.hpp>

namespace vmm {
namespace {

static_assert(std::endian::native == std::endian::little, "binary STL is little-endian");

std::uint32_t patch_id(TriangleSoup& soup, const std::string& name) {
    const auto it = std::ranges::find(soup.patches, name);
    if (it != soup.patches.end()) return static_cast<std::uint32_t>(it - soup.patches.begin());
    soup.patches.push_back(name);
    return static_cast<std::uint32_t>(soup.patches.size() - 1);
}

void add_triangle(TriangleSoup& soup, const std::array<Vec3, 3>& v, std::uint32_t patch) {
    const auto base = static_cast<std::uint32_t>(soup.points.size());
    soup.points.insert(soup.points.end(), v.begin(), v.end());
    soup.triangles.push_back({base, base + 1, base + 2});
    soup.triangle_patch.push_back(patch);
}

Result<TriangleSoup> read_binary(const std::string& bytes, const StlReadOptions& options) {
    std::uint32_t count = 0;
    std::memcpy(&count, bytes.data() + 80, 4);
    if (count == 0 || count > (std::numeric_limits<std::uint32_t>::max() - 1) / 3)
        return fail(ErrorCode::ParseError, "triangle count exceeds ID range");
    TriangleSoup soup;
    const std::uint32_t patch = patch_id(soup, options.patch_name);
    for (std::uint32_t t = 0; t < count; ++t) {
        const char* record = bytes.data() + 84 + std::size_t{50} * t;
        std::array<float, 12> f{};
        std::memcpy(f.data(), record, sizeof(f));  // normal, then three vertices
        if (!std::ranges::all_of(f, [](float x) { return std::isfinite(x); }))
            return fail(ErrorCode::ParseError, "non-finite binary STL value");
        std::array<Vec3, 3> v;
        for (std::size_t k = 0; k < 3; ++k) v[k] = {f[3 + 3 * k], f[4 + 3 * k], f[5 + 3 * k]};
        add_triangle(soup, v, patch);
    }
    return soup;
}

Result<TriangleSoup> read_ascii(const std::string& text, const StlReadOptions& options) {
    TriangleSoup soup;
    std::istringstream in(text);
    std::string line;
    std::size_t number = 0;
    std::uint32_t patch = 0;
    bool in_solid = false;
    bool in_facet = false;
    bool in_loop = false;
    bool closed_loop = false;
    std::vector<Vec3> loop;
    const auto error = [&](const std::string& what) { return fail(ErrorCode::ParseError, std::format("line {}: {}", number, what)); };
    while (std::getline(in, line)) {
        ++number;
        std::istringstream words(line);
        std::string word;
        if (!(words >> word)) continue;
        if (word == "solid") {
            if (in_solid) return error("solid inside a solid");
            std::string name;
            std::getline(words >> std::ws, name);
            while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) name.pop_back();
            patch = patch_id(soup, name.empty() ? options.patch_name : name);
            in_solid = true;
        } else if (word == "endsolid") {
            if (!in_solid || in_facet) return error("endsolid without solid or unfinished facet");
            in_solid = false;
        } else if (word == "facet") {
            Vec3 n;
            std::string normal, extra;
            if (!in_solid || in_facet || !(words >> normal >> n[0] >> n[1] >> n[2]) || normal != "normal" ||
                !std::ranges::all_of(n, [](Real x) { return std::isfinite(x); }) || (words >> extra))
                return error("invalid facet normal");
            in_facet = true;
            closed_loop = false;
        } else if (word == "outer") {
            std::string keyword, extra;
            if (!in_facet || in_loop || closed_loop || !(words >> keyword) || keyword != "loop" || (words >> extra))
                return error("invalid outer loop");
            in_loop = true;
        } else if (word == "endloop") {
            std::string extra;
            if (!in_loop || loop.size() != 3 || (words >> extra)) return error("invalid endloop");
            in_loop = false;
            closed_loop = true;
        } else if (word == "vertex") {
            Vec3 p;
            std::string extra;
            if (!in_loop || loop.size() >= 3 || !(words >> p[0] >> p[1] >> p[2]) ||
                !std::ranges::all_of(p, [](Real x) { return std::isfinite(x); }) || (words >> extra))
                return error("vertex needs three finite numbers inside a loop");
            loop.push_back(p);
        } else if (word == "endfacet") {
            if (!in_facet || !closed_loop || in_loop || loop.size() != 3) return error(std::format("facet with {} vertices", loop.size()));
            std::string extra;
            if (words >> extra) return error("extra endfacet data");
            if (soup.points.size() > std::numeric_limits<std::uint32_t>::max() - 4)
                return error("vertex ID range exhausted");
            add_triangle(soup, {loop[0], loop[1], loop[2]}, patch);
            in_facet = false;
            loop.clear();
        } else {
            return error("unknown keyword '" + word + "'");
        }
    }
    if (in_solid || in_facet || in_loop) return error("unfinished solid or facet");
    if (soup.triangles.empty()) return fail(ErrorCode::ParseError, "no facet");
    return soup;
}

}  // namespace

Result<TriangleSoup> read_stl(std::istream& in, const StlReadOptions& options) try {
    const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) return fail(ErrorCode::ParseError, "STL stream failure");
    if (bytes.size() >= 84) {
        std::uint32_t count = 0;
        std::memcpy(&count, bytes.data() + 80, 4);
        if (bytes.size() == 84 + std::size_t{50} * count) return read_binary(bytes, options);
    }
    const auto first = bytes.find_first_not_of(" \t\r\n");
    if (first != std::string::npos && bytes.compare(first, 5, "solid") == 0) return read_ascii(bytes, options);
    return fail(ErrorCode::ParseError, "neither ASCII (solid ...) nor binary STL");
} catch (const std::bad_alloc&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::length_error&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::exception&) {
    return fail(ErrorCode::ParseError);
}

Result<TriangleSoup> read_stl(const std::filesystem::path& path, const StlReadOptions& options) try {
    std::ifstream in(path, std::ios::binary);
    if (!in) return fail(ErrorCode::FileOpenFailed, path.string());
    return read_stl(in, options);
} catch (const std::bad_alloc&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::length_error&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::exception&) {
    return fail(ErrorCode::ParseError);
}

Result<TriangleSurface> read_stl_surface(const std::filesystem::path& path, const StlReadOptions& options,
                                         const SurfaceRepairOptions& repair, SurfaceRepairReport* report) try {
    auto soup = read_stl(path, options);
    if (!soup) return std::unexpected(soup.error());
    return repair_surface(*soup, repair, report);
} catch (const std::bad_alloc&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::length_error&) {
    return fail(ErrorCode::ResourceExhausted);
} catch (const std::exception&) {
    return fail(ErrorCode::ParseError);
}

Status write_stl(const TriangleSurface& s, std::ostream& out, const StlWriteOptions& options) {
    const auto& p = s.points();
    const auto normal = [&](const Triangle& t) {
        const Vec3 n = cross(p[t[1]] - p[t[0]], p[t[2]] - p[t[0]]);
        return (1 / norm(n)) * n;
    };
    if (options.binary) {
        std::array<char, 80> header{};
        std::memcpy(header.data(), "VoronoiMeshMaker binary STL", 27);
        out.write(header.data(), 80);
        const auto count = static_cast<std::uint32_t>(s.triangle_count());
        out.write(reinterpret_cast<const char*>(&count), 4);
        for (const Triangle& t : s.triangles()) {
            const Vec3 n = normal(t);
            std::array<float, 12> f{static_cast<float>(n[0]), static_cast<float>(n[1]), static_cast<float>(n[2])};
            for (std::size_t k = 0; k < 3; ++k) {
                for (std::size_t c = 0; c < 3; ++c) f[3 + 3 * k + c] = static_cast<float>(p[t[k]][c]);
            }
            out.write(reinterpret_cast<const char*>(f.data()), sizeof(f));
            const std::uint16_t attribute = 0;
            out.write(reinterpret_cast<const char*>(&attribute), 2);
        }
    } else {
        for (std::uint32_t patch = 0; patch < s.patches().size(); ++patch) {
            out << "solid " << s.patches()[patch] << "\n";
            for (std::size_t t = 0; t < s.triangle_count(); ++t) {
                if (s.triangle_patch()[t] != patch) continue;
                const Triangle& x = s.triangles()[t];
                const Vec3 n = normal(x);
                out << std::format("  facet normal {:.17g} {:.17g} {:.17g}\n    outer loop\n", n[0], n[1], n[2]);
                for (const std::uint32_t v : x) out << std::format("      vertex {:.17g} {:.17g} {:.17g}\n", p[v][0], p[v][1], p[v][2]);
                out << "    endloop\n  endfacet\n";
            }
            out << "endsolid " << s.patches()[patch] << "\n";
        }
    }
    if (!out) return fail(ErrorCode::FileOpenFailed, "write error");
    return {};
}

Status write_stl(const TriangleSurface& s, const std::filesystem::path& path, const StlWriteOptions& options) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return fail(ErrorCode::FileOpenFailed, path.string());
    return write_stl(s, out, options);
}

}  // namespace vmm
