// ============================================================================
// File: app_support.hpp
// Description: P05a prototype - helpers shared by the proof executables:
//              portable pseudo-random numbers and PASS/FAIL bookkeeping.
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <numbers>
#include <print>
#include <random>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <p05a/backend.hpp>
#include <p05a/invariants.hpp>
#include <p05a/mesh.hpp>
#include <p05a/types.hpp>

namespace vmm::p05a::app {

/// std::mt19937_64 is fully specified by the standard; the mapping to [0, 1)
/// is done by hand because std::uniform_real_distribution is not portable.
class Random {
public:
    explicit Random(std::uint64_t seed) : engine_(seed) {}
    Real uniform() { return static_cast<Real>(engine_() >> 11) * 0x1.0p-53; }
    std::uint64_t next() { return engine_(); }

private:
    std::mt19937_64 engine_;
};

/// Counts failed checks; the executable returns failures() != 0.
class Checks {
public:
    bool expect(bool ok, std::string_view what) {
        std::println("  [{}] {}", ok ? "PASS" : "FAIL", what);
        if (!ok) ++failures_;
        return ok;
    }
    [[nodiscard]] int failures() const noexcept { return failures_; }
    [[nodiscard]] int exit_code() const noexcept { return failures_ == 0 ? 0 : 1; }

private:
    int failures_ = 0;
};

inline void print_backend() {
    const BackendInfo info = backend_info();
    std::println("backend: CGAL {} | Boost {} | exact FT {}", info.cgal_version, info.boost_version,
                 info.exact_number_type);
}

/// Prints one invariant report and records the DEC-011 checks.
inline void expect_invariants(Checks& checks, const InvariantReport& r, const InvariantReference& ref,
                              std::string_view label) {
    constexpr Real deg = 180.0 / std::numbers::pi;
    std::println("  {}: cells {} | faces internal {} interface {} boundary {}", label, r.cells, r.internal_faces,
                 r.interface_faces, r.boundary_faces);
    std::println("    measure rel. error: total {:.3e} | worst region {:.3e} | worst cell (vs polygon) {:.3e}",
                 r.total_relative_error, r.max_region_relative_error, r.max_cell_measure_error);
    std::println("    boundary rel. error {:.3e} | interface rel. error {:.3e}", r.boundary_relative_error,
                 r.interface_relative_error);
    std::println("    closure max |sum S_f| {:.3e} (tol {:.3e}) | non-orthogonality max internal {:.3e} rad, "
                 "interface {:.3e} rad ({:.2f} deg)",
                 r.max_closure, r.closure_tolerance, r.max_nonortho_internal, r.max_nonortho_interface,
                 r.max_nonortho_interface * deg);
    const Real tol = ref.relative_tolerance;
    checks.expect(r.measures_ok(tol), std::format("{}: measures (total, regions, cells, boundary, interface) <= {:.0e}",
                                                  label, tol));
    checks.expect(r.max_closure <= r.closure_tolerance, std::format("{}: closure of every cell", label));
    checks.expect(r.bad_owner_neighbour == 0 && r.nonpositive_cells == 0 && r.adjacency_symmetric,
                  std::format("{}: owner/neighbour consistent, every cell positive, adjacency symmetric", label));
    checks.expect(r.max_nonortho_internal <= ref.nonorthogonality_limit,
                  std::format("{}: internal non-orthogonality <= {:.0e} rad", label, ref.nonorthogonality_limit));
}

/// Checks the internal/boundary views: face views against the report, cell
/// views as a partition, and the faces reached through each cell.
template <std::size_t D>
void expect_iteration(Checks& checks, const PolyMesh<D>& m, const InvariantReport& r, std::string_view label) {
    std::size_t n_internal_faces = 0;
    for (const FaceId f : internal_faces(m)) n_internal_faces += m.neighbour[f.index()].valid() ? 1 : 0;
    std::size_t n_boundary_faces = 0;
    for (const FaceId f : boundary_faces(m)) n_boundary_faces += m.patch[f.index()].valid() ? 1 : 0;

    const CellFaceIndex index = index_cell_faces(m);
    std::size_t n_internal_cells = 0;
    bool internal_ok = true;
    for (const CellId c : index.internal_cells()) {
        ++n_internal_cells;
        for (const FaceId f : index.faces_of(c)) internal_ok = internal_ok && m.neighbour[f.index()].valid();
    }
    std::size_t n_boundary_cells = 0;
    bool boundary_ok = true;
    for (const CellId c : index.boundary_cells()) {
        ++n_boundary_cells;
        const auto faces = index.faces_of(c);
        boundary_ok = boundary_ok && std::ranges::any_of(faces, [&](FaceId f) { return !m.neighbour[f.index()].valid(); });
    }
    std::println("    iteration: internal faces {} | boundary faces {} | internal cells {} | boundary cells {}",
                 n_internal_faces, n_boundary_faces, n_internal_cells, n_boundary_cells);
    checks.expect(n_internal_faces == r.internal_faces + r.interface_faces && n_boundary_faces == r.boundary_faces,
                  std::format("{}: face views cover internal and boundary faces exactly", label));
    checks.expect(n_internal_cells + n_boundary_cells == m.cell_count() && internal_ok && boundary_ok &&
                      index.faces.values.size() == 2 * n_internal_faces + n_boundary_faces,
                  std::format("{}: cell views partition the cells; faces reached through each cell", label));
}

using Signature = std::vector<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>>;

/// Faces as (key(owner), key(neighbour) or max, patch or max), where key is
/// the rank of the site in lexicographic order: independent of the input
/// order and of a positive scaling.
template <std::size_t D>
[[nodiscard]] Signature signature(const PolyMesh<D>& m) {
    std::vector<std::uint32_t> order(m.cell_count());
    for (std::uint32_t c = 0; c < order.size(); ++c) order[c] = c;
    std::ranges::sort(order, [&](std::uint32_t a, std::uint32_t b) { return m.sites[a] < m.sites[b]; });
    std::vector<std::uint32_t> key(m.cell_count());
    for (std::uint32_t r = 0; r < order.size(); ++r) key[order[r]] = r;
    constexpr std::uint32_t none = std::numeric_limits<std::uint32_t>::max();
    Signature sig;
    for (std::size_t f = 0; f < m.face_count(); ++f) {
        const std::uint32_t o = key[m.owner[f].index()];
        if (!m.neighbour[f].valid()) {
            sig.emplace_back(o, none, m.patch[f].value);
            continue;
        }
        const std::uint32_t n = key[m.neighbour[f].index()];
        sig.emplace_back(std::min(o, n), std::max(o, n), none);
    }
    std::ranges::sort(sig);
    return sig;
}

/// Insertion order `variant`: 0 identity, 1 reversed, >= 2 seeded shuffles.
inline std::vector<std::uint32_t> permutation(std::size_t n, int variant) {
    std::vector<std::uint32_t> p(n);
    for (std::uint32_t k = 0; k < n; ++k) p[k] = k;
    if (variant == 1) std::ranges::reverse(p);
    if (variant >= 2) {  // Fisher-Yates with the portable generator
        app::Random rng(1000 + static_cast<std::uint64_t>(variant));
        for (std::size_t k = n - 1; k > 0; --k) std::swap(p[k], p[rng.next() % (k + 1)]);
    }
    return p;
}

}  // namespace vmm::p05a::app
