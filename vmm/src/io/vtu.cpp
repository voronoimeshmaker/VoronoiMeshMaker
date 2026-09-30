// ============================================================================
// File: vtu.cpp
// Description: VTK XML writer (ASCII).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstddef>
#include <format>
#include <fstream>
#include <map>
#include <ostream>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/geometry/polygon.hpp>
#include <vmm/io/vtu.hpp>
#include <vmm/mesh/metrics.hpp>

namespace vmm {

Result<std::vector<std::vector<std::vector<VertexId>>>> cell_loops(const Mesh2D& m) {
    // Directed edges of each cell, oriented with the cell on the left.
    std::vector<std::multimap<VertexId, VertexId>> next(m.cell_count());
    for (const FaceId f : m.faces()) {
        const auto v = m.face_vertices(f);
        next[m.owner(f).index()].emplace(v[0], v[1]);
        if (m.is_internal(f)) next[m.neighbour(f).index()].emplace(v[1], v[0]);
    }
    std::vector<std::vector<std::vector<VertexId>>> out(m.cell_count());
    for (const CellId c : m.cells()) {
        auto& edges = next[c.index()];
        while (!edges.empty()) {
            std::vector<VertexId> loop;
            const VertexId start = edges.begin()->first;
            VertexId at = start;
            do {
                const auto it = edges.find(at);
                if (it == edges.end()) {
                    return fail(ErrorCode::InvariantViolated, "open cell boundary", c);
                }
                loop.push_back(at);
                at = it->second;
                edges.erase(it);
            } while (at != start);
            out[c.index()].push_back(std::move(loop));
        }
        auto area = [&](const std::vector<VertexId>& l) {
            std::vector<Vec2> pts;
            for (const VertexId v : l) pts.push_back(m.point(v));
            return signed_area(pts);
        };
        std::ranges::stable_sort(out[c.index()], [&](const auto& a, const auto& b) { return area(a) > area(b); });
    }
    return out;
}

Status write_vtu(const Mesh2D& m, std::ostream& out, const VtuOptions& options) {
    auto loops = cell_loops(m);
    if (!loops) return std::unexpected(loops.error());
    std::size_t connectivity = 0;
    for (const auto& l : *loops) connectivity += l.empty() ? 0 : l.front().size();
    auto real = [](Real x) { return std::format("{:.17g}", x); };

    out << "<?xml version=\"1.0\"?>\n"
        << "<VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n"
        << "  <UnstructuredGrid>\n"
        << std::format("    <Piece NumberOfPoints=\"{}\" NumberOfCells=\"{}\">\n", m.point_count(), m.cell_count())
        << "      <Points>\n        <DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">\n";
    for (const auto& p : m.points()) out << "          " << real(p[0]) << " " << real(p[1]) << " 0\n";
    out << "        </DataArray>\n      </Points>\n      <Cells>\n"
        << "        <DataArray type=\"Int64\" Name=\"connectivity\" format=\"ascii\">\n";
    for (const auto& l : *loops) {
        out << "         ";
        if (!l.empty()) {
            for (const VertexId v : l.front()) out << " " << v.value;
        }
        out << "\n";
    }
    out << "        </DataArray>\n        <DataArray type=\"Int64\" Name=\"offsets\" format=\"ascii\">\n";
    std::size_t offset = 0;
    for (const auto& l : *loops) {
        offset += l.empty() ? 0 : l.front().size();
        out << "          " << offset << "\n";
    }
    out << "        </DataArray>\n        <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">\n";
    for (std::size_t c = 0; c < m.cell_count(); ++c) out << "          7\n";  // VTK_POLYGON
    out << "        </DataArray>\n      </Cells>\n      <CellData Scalars=\"region\">\n";

    auto int_array = [&](const char* name, auto value) {
        out << std::format("        <DataArray type=\"Int64\" Name=\"{}\" format=\"ascii\">\n", name);
        for (const CellId c : m.cells()) out << "          " << value(c) << "\n";
        out << "        </DataArray>\n";
    };
    auto real_array = [&](const char* name, const std::vector<Real>& v) {
        out << std::format("        <DataArray type=\"Float64\" Name=\"{}\" format=\"ascii\">\n", name);
        for (const Real x : v) out << "          " << real(x) << "\n";
        out << "        </DataArray>\n";
    };
    int_array("region", [&](CellId c) { return m.region(c).value; });
    int_array("medium", [&](CellId c) { return m.regions()[m.region(c).index()].medium.value; });
    int_array("input_site", [&](CellId c) { return m.cell_input_sites()[c.index()].value; });
    int_array("loops", [&](CellId c) { return (*loops)[c.index()].size(); });
    if (options.include_metrics) {
        const auto x = compute_metrics(m);
        std::vector<Real> nonortho(m.cell_count(), 0);
        for (const FaceId f : m.internal_faces()) {
            for (const CellId c : {m.owner(f), m.neighbour(f)}) {
                nonortho[c.index()] = std::max(nonortho[c.index()], x.nonorthogonality[f.index()]);
            }
        }
        real_array("area", x.cell_measure);
        real_array("aspect_ratio", x.cell_aspect_ratio);
        real_array("max_nonorthogonality", nonortho);
    }
    out << "      </CellData>\n    </Piece>\n  </UnstructuredGrid>\n</VTKFile>\n";
    if (!out) return fail(ErrorCode::FileOpenFailed, "write error");
    (void)connectivity;
    return {};
}

Status write_vtu(const Mesh2D& m, const std::filesystem::path& path, const VtuOptions& options) {
    std::ofstream out(path);
    if (!out) return fail(ErrorCode::FileOpenFailed, path.string());
    return write_vtu(m, out, options);
}

Status write_vtu(const Mesh3D& m, std::ostream& out, const VtuOptions& options) {
    const CellFaceIndex index(m);
    auto real = [](Real x) { return std::format("{:.17g}", x); };
    out << "<?xml version=\"1.0\"?>\n"
        << "<VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n"
        << "  <UnstructuredGrid>\n"
        << std::format("    <Piece NumberOfPoints=\"{}\" NumberOfCells=\"{}\">\n", m.point_count(), m.cell_count())
        << "      <Points>\n        <DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">\n";
    for (const auto& p : m.points()) out << "          " << real(p[0]) << " " << real(p[1]) << " " << real(p[2]) << "\n";
    out << "        </DataArray>\n      </Points>\n      <Cells>\n";

    // Distinct points of every cell (connectivity) and its faces with outward normals.
    std::vector<std::vector<VertexId>> cell_points(m.cell_count());
    for (const CellId c : m.cells()) {
        auto& pts = cell_points[c.index()];
        for (const FaceId f : index.faces_of(c)) {
            const auto v = m.face_vertices(f);
            pts.insert(pts.end(), v.begin(), v.end());
        }
        std::ranges::sort(pts);
        pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
    }
    out << "        <DataArray type=\"Int64\" Name=\"connectivity\" format=\"ascii\">\n";
    for (const auto& pts : cell_points) {
        out << "         ";
        for (const VertexId v : pts) out << " " << v.value;
        out << "\n";
    }
    out << "        </DataArray>\n        <DataArray type=\"Int64\" Name=\"offsets\" format=\"ascii\">\n";
    std::size_t offset = 0;
    for (const auto& pts : cell_points) {
        offset += pts.size();
        out << "          " << offset << "\n";
    }
    out << "        </DataArray>\n        <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">\n";
    for (std::size_t c = 0; c < m.cell_count(); ++c) out << "          42\n";  // VTK_POLYHEDRON
    out << "        </DataArray>\n        <DataArray type=\"Int64\" Name=\"faces\" format=\"ascii\">\n";
    std::vector<std::size_t> face_offsets;
    std::size_t face_offset = 0;
    for (const CellId c : m.cells()) {
        const auto faces = index.faces_of(c);
        out << "          " << faces.size();
        face_offset += 1;
        for (const FaceId f : faces) {
            auto v = std::vector<VertexId>(m.face_vertices(f).begin(), m.face_vertices(f).end());
            if (m.owner(f) != c) std::ranges::reverse(v);
            out << " " << v.size();
            for (const VertexId x : v) out << " " << x.value;
            face_offset += 1 + v.size();
        }
        out << "\n";
        face_offsets.push_back(face_offset);
    }
    out << "        </DataArray>\n        <DataArray type=\"Int64\" Name=\"faceoffsets\" format=\"ascii\">\n";
    for (const std::size_t o : face_offsets) out << "          " << o << "\n";
    out << "        </DataArray>\n      </Cells>\n      <CellData Scalars=\"region\">\n";

    auto int_array = [&](const char* name, auto value) {
        out << std::format("        <DataArray type=\"Int64\" Name=\"{}\" format=\"ascii\">\n", name);
        for (const CellId c : m.cells()) out << "          " << value(c) << "\n";
        out << "        </DataArray>\n";
    };
    auto real_array = [&](const char* name, const std::vector<Real>& v) {
        out << std::format("        <DataArray type=\"Float64\" Name=\"{}\" format=\"ascii\">\n", name);
        for (const Real x : v) out << "          " << real(x) << "\n";
        out << "        </DataArray>\n";
    };
    int_array("region", [&](CellId c) { return m.region(c).value; });
    int_array("medium", [&](CellId c) { return m.regions()[m.region(c).index()].medium.value; });
    int_array("input_site", [&](CellId c) { return m.cell_input_sites()[c.index()].value; });
    if (options.include_metrics) {
        const auto x = compute_metrics(m);
        std::vector<Real> nonortho(m.cell_count(), 0);
        for (const FaceId f : m.internal_faces()) {
            for (const CellId c : {m.owner(f), m.neighbour(f)}) {
                nonortho[c.index()] = std::max(nonortho[c.index()], x.nonorthogonality[f.index()]);
            }
        }
        real_array("volume", x.cell_measure);
        real_array("aspect_ratio", x.cell_aspect_ratio);
        real_array("max_nonorthogonality", nonortho);
    }
    out << "      </CellData>\n    </Piece>\n  </UnstructuredGrid>\n</VTKFile>\n";
    if (!out) return fail(ErrorCode::FileOpenFailed, "write error");
    return {};
}

Status write_vtu(const Mesh3D& m, const std::filesystem::path& path, const VtuOptions& options) {
    std::ofstream out(path);
    if (!out) return fail(ErrorCode::FileOpenFailed, path.string());
    return write_vtu(m, out, options);
}

}  // namespace vmm
