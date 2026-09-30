// ============================================================================
// File: repair.cpp
// Description: repair_surface.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/geometry/repair.hpp>

namespace vmm {
namespace {

std::uint64_t cell_key(std::int64_t a, std::int64_t b, std::int64_t c) {
    const auto u = [](std::int64_t v) { return static_cast<std::uint64_t>(v) & 0x1fffffULL; };
    return (u(a) << 42) | (u(b) << 21) | u(c);
}

std::uint64_t edge_key(std::uint32_t a, std::uint32_t b) {
    return (std::uint64_t{std::min(a, b)} << 32) | std::max(a, b);
}

/// True when the triangle traverses a -> b (in this direction).
bool goes(const Triangle& t, std::uint32_t a, std::uint32_t b) {
    for (std::size_t k = 0; k < 3; ++k) {
        if (t[k] == a && t[(k + 1) % 3] == b) return true;
    }
    return false;
}

}  // namespace

Result<TriangleSurface> repair_surface(const TriangleSoup& soup, const SurfaceRepairOptions& options,
                                       SurfaceRepairReport* report) {
    SurfaceRepairReport rep;
    if (soup.triangles.empty()) return fail(ErrorCode::InvalidSurface, "no triangles");
    if (soup.triangle_patch.size() != soup.triangles.size()) {
        return fail(ErrorCode::InvalidArgument, "one patch per triangle expected");
    }
    for (std::size_t t = 0; t < soup.triangles.size(); ++t) {
        if (std::ranges::any_of(soup.triangles[t], [&](std::uint32_t v) { return v >= soup.points.size(); })) {
            return fail(ErrorCode::InvalidArgument, std::format("triangle {}: vertex index", t));
        }
        if (soup.triangle_patch[t] >= soup.patches.size()) {
            return fail(ErrorCode::InvalidArgument, std::format("triangle {}: patch index", t));
        }
    }
    for (const Vec3& p : soup.points) {
        if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2])) {
            return fail(ErrorCode::InvalidSurface, "non-finite point");
        }
    }
    const Real L = Box3::of(soup.points).diagonal();
    if (!(L > 0) || !(options.weld_relative_tolerance >= 0)) return fail(ErrorCode::InvalidSurface, "zero extent or bad tolerance");
    const Real tol = options.weld_relative_tolerance * L;

    // 1. Weld points within the tolerance (the first point seen represents the others).
    std::vector<std::uint32_t> rep_of(soup.points.size());
    {
        const Real cell = tol > 0 ? 4 * tol : L * 1e-12;
        std::unordered_map<std::uint64_t, std::vector<std::uint32_t>> grid;
        for (std::uint32_t i = 0; i < soup.points.size(); ++i) {
            const Vec3& p = soup.points[i];
            const auto q = [&](std::size_t k) { return static_cast<std::int64_t>(std::floor(p[k] / cell)); };
            rep_of[i] = i;
            bool found = false;
            for (std::int64_t da = -1; da <= 1 && !found; ++da) {
                for (std::int64_t db = -1; db <= 1 && !found; ++db) {
                    for (std::int64_t dc = -1; dc <= 1 && !found; ++dc) {
                        const auto it = grid.find(cell_key(q(0) + da, q(1) + db, q(2) + dc));
                        if (it == grid.end()) continue;
                        for (const std::uint32_t r : it->second) {
                            if (norm(soup.points[r] - p) <= tol) {
                                rep_of[i] = r;
                                found = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (found) {
                ++rep.welded_points;
            } else {
                grid[cell_key(q(0), q(1), q(2))].push_back(i);
            }
        }
    }

    // 2. Collapsed triangles; 3. duplicates (opposite copies cancel).
    std::vector<Triangle> tris;
    std::vector<std::uint32_t> patch;
    std::map<std::array<std::uint32_t, 3>, std::vector<std::size_t>> by_key;
    for (std::size_t t = 0; t < soup.triangles.size(); ++t) {
        Triangle x{rep_of[soup.triangles[t][0]], rep_of[soup.triangles[t][1]], rep_of[soup.triangles[t][2]]};
        if (x[0] == x[1] || x[1] == x[2] || x[0] == x[2]) {
            ++rep.collapsed_triangles;
            continue;
        }
        std::array<std::uint32_t, 3> key = x;
        std::ranges::sort(key);
        by_key[key].push_back(tris.size());
        tris.push_back(x);
        patch.push_back(soup.triangle_patch[t]);
    }
    std::vector<char> keep(tris.size(), 1);
    for (const auto& [key, copies] : by_key) {
        if (copies.size() < 2) continue;
        // Orientation of each copy relative to the first one.
        std::vector<std::size_t> same;
        std::vector<std::size_t> opposite;
        const Triangle& first = tris[copies.front()];
        for (const std::size_t c : copies) (goes(tris[c], first[0], first[1]) ? same : opposite).push_back(c);
        for (const std::size_t c : copies) keep[c] = 0;
        rep.duplicate_triangles += copies.size();
        if (same.size() != opposite.size()) {
            const std::size_t survivor = same.size() > opposite.size() ? same.front() : opposite.front();
            keep[survivor] = 1;
            --rep.duplicate_triangles;
        }
    }
    std::vector<Triangle> kept;
    std::vector<std::uint32_t> kept_patch;
    for (std::size_t t = 0; t < tris.size(); ++t) {
        if (keep[t]) {
            kept.push_back(tris[t]);
            kept_patch.push_back(patch[t]);
        }
    }
    if (kept.empty()) return fail(ErrorCode::InvalidSurface, "no triangle left after the repair");

    // 4. Edge adjacency: two triangles per edge (manifold and closed).
    std::unordered_map<std::uint64_t, std::vector<std::uint32_t>> edges;
    for (std::uint32_t t = 0; t < kept.size(); ++t) {
        for (std::size_t k = 0; k < 3; ++k) edges[edge_key(kept[t][k], kept[t][(k + 1) % 3])].push_back(t);
    }
    std::size_t holes = 0;
    for (const auto& [key, ts] : edges) {
        if (ts.size() > 2) return fail(ErrorCode::InvalidSurface, std::format("edge shared by {} triangles", ts.size()));
        if (ts.size() == 1) ++holes;
    }
    if (holes > 0) return fail(ErrorCode::InvalidSurface, std::format("{} boundary edges (the surface has holes)", holes));

    // 5. Consistent orientation per component (breadth-first), with the fewest flips.
    std::vector<int> flip(kept.size(), -1);
    for (std::uint32_t seed = 0; seed < kept.size(); ++seed) {
        if (flip[seed] != -1) continue;
        ++rep.components;
        std::vector<std::uint32_t> component{seed};
        flip[seed] = 0;
        for (std::size_t head = 0; head < component.size(); ++head) {
            const std::uint32_t t = component[head];
            for (std::size_t k = 0; k < 3; ++k) {
                const std::uint32_t a = kept[t][k];
                const std::uint32_t b = kept[t][(k + 1) % 3];
                const bool t_forward = (flip[t] == 0);  // t traverses a -> b when not flipped
                for (const std::uint32_t u : edges[edge_key(a, b)]) {
                    if (u == t) continue;
                    // u must traverse the edge opposite to t.
                    const bool u_same_as_raw = goes(kept[u], a, b);
                    const int wanted = (u_same_as_raw == t_forward) ? 1 : 0;
                    if (flip[u] == -1) {
                        flip[u] = wanted;
                        component.push_back(u);
                    } else if (flip[u] != wanted) {
                        return fail(ErrorCode::InvalidSurface, "non-orientable surface");
                    }
                }
            }
        }
        const auto flipped = static_cast<std::size_t>(std::ranges::count_if(component, [&](std::uint32_t t) { return flip[t] == 1; }));
        const bool invert = 2 * flipped > component.size();
        for (const std::uint32_t t : component) {
            if (invert) flip[t] = 1 - flip[t];
            if (flip[t] == 1) {
                std::swap(kept[t][1], kept[t][2]);
                ++rep.flipped_triangles;
            }
        }
    }

    // 6. Compact the points and build the checked surface.
    std::vector<std::uint32_t> local(soup.points.size(), UINT32_MAX);
    std::vector<Vec3> points;
    for (Triangle& t : kept) {
        for (std::uint32_t& v : t) {
            if (local[v] == UINT32_MAX) {
                local[v] = static_cast<std::uint32_t>(points.size());
                points.push_back(soup.points[v]);
            }
            v = local[v];
        }
    }
    if (report != nullptr) *report = rep;
    return TriangleSurface::make(std::move(points), std::move(kept), std::move(kept_patch), soup.patches);
}

}  // namespace vmm
