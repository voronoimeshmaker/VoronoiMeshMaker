// ============================================================================
// File: reorder.cpp
// Description: Permutations, orderings and renumbering.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/reorder/reorder.hpp>

namespace vmm {
namespace {

/// Hilbert index of (x, y) in a 2^bits grid (classic xy2d with rotations).
std::uint64_t hilbert_index(std::uint32_t x, std::uint32_t y, int bits) {
    const std::uint64_t n = std::uint64_t{1} << bits;
    std::uint64_t d = 0;
    std::uint64_t px = x;
    std::uint64_t py = y;
    for (std::uint64_t s = n / 2; s > 0; s /= 2) {
        const std::uint64_t rx = (px & s) ? 1 : 0;
        const std::uint64_t ry = (py & s) ? 1 : 0;
        d += s * s * ((3 * rx) ^ ry);
        if (ry == 0) {
            if (rx == 1) {
                px = n - 1 - px;
                py = n - 1 - py;
            }
            std::swap(px, py);
        }
    }
    return d;
}

std::uint64_t morton_index(const std::vector<std::uint32_t>& q, int bits) {
    std::uint64_t d = 0;
    for (int b = bits - 1; b >= 0; --b) {
        for (const std::uint32_t v : q) d = (d << 1) | ((v >> b) & 1u);
    }
    return d;
}

Permutation from_keys(std::vector<std::pair<std::uint64_t, std::size_t>> keys) {
    std::ranges::sort(keys);
    std::vector<CellId> order;
    order.reserve(keys.size());
    for (const auto& k : keys) order.push_back(CellId::from_index(k.second));
    return *Permutation::from_order(std::move(order));
}

}  // namespace

Result<Permutation> Permutation::from_order(std::vector<CellId> old_of_new) {
    Permutation p;
    p.new_of_old_.assign(old_of_new.size(), CellId::invalid());
    for (std::size_t k = 0; k < old_of_new.size(); ++k) {
        const CellId o = old_of_new[k];
        if (!o.valid() || o.index() >= old_of_new.size() || p.new_of_old_[o.index()].valid()) {
            return fail(ErrorCode::InvalidArgument, "not a permutation", o);
        }
        p.new_of_old_[o.index()] = CellId::from_index(k);
    }
    p.old_of_new_ = std::move(old_of_new);
    return p;
}

Permutation Permutation::identity(std::size_t n) {
    std::vector<CellId> order(n);
    for (std::size_t k = 0; k < n; ++k) order[k] = CellId::from_index(k);
    return *from_order(std::move(order));
}

Permutation Permutation::inverse() const {
    Permutation p;
    p.new_of_old_ = old_of_new_;
    p.old_of_new_ = new_of_old_;
    return p;
}

template <std::size_t D>
Permutation LexicographicOrdering::permutation(const Mesh<D>& m) const {
    std::vector<CellId> order(m.cell_count());
    for (std::size_t k = 0; k < order.size(); ++k) order[k] = CellId::from_index(k);
    std::ranges::stable_sort(order, [&](CellId a, CellId b) { return m.site(a) < m.site(b); });
    return *Permutation::from_order(std::move(order));
}

template <std::size_t D>
Permutation HilbertOrdering::permutation(const Mesh<D>& m) const {
    Vec<D> lo = m.sites().empty() ? Vec<D>{} : m.sites()[0];
    Vec<D> hi = lo;
    for (const auto& x : m.sites()) {
        for (std::size_t k = 0; k < D; ++k) {
            lo[k] = std::min(lo[k], x[k]);
            hi[k] = std::max(hi[k], x[k]);
        }
    }
    const int b = std::clamp(bits, 1, D == 2 ? 31 : 21);
    const Real cells = static_cast<Real>((std::uint64_t{1} << b) - 1);
    std::vector<std::pair<std::uint64_t, std::size_t>> keys;
    keys.reserve(m.cell_count());
    for (std::size_t c = 0; c < m.cell_count(); ++c) {
        std::vector<std::uint32_t> q(D);
        for (std::size_t k = 0; k < D; ++k) {
            const Real w = hi[k] - lo[k];
            q[k] = static_cast<std::uint32_t>(w > 0 ? (m.sites()[c][k] - lo[k]) / w * cells : 0);
        }
        const std::uint64_t key = D == 2 ? hilbert_index(q[0], q[1], b) : morton_index(q, b);
        keys.emplace_back(key, c);
    }
    return from_keys(std::move(keys));
}

Permutation reverse_cuthill_mckee(const Csr<CellId>& adj) {
    const std::size_t n = adj.rows();
    std::vector<char> seen(n, 0);
    std::vector<CellId> order;
    order.reserve(n);
    auto degree = [&](std::size_t i) { return adj.row(i).size(); };
    auto bfs_last = [&](std::size_t start, std::vector<char>& mark) {
        // Returns the last cell reached and the eccentricity (levels) from start.
        std::deque<std::size_t> queue{start};
        std::vector<std::size_t> level(n, 0);
        mark[start] = 1;
        std::size_t last = start;
        std::size_t depth = 0;
        std::vector<std::size_t> touched{start};
        while (!queue.empty()) {
            const std::size_t u = queue.front();
            queue.pop_front();
            if (level[u] > depth || (level[u] == depth && degree(u) < degree(last))) {
                depth = level[u];
                last = u;
            }
            for (const CellId v : adj.row(u)) {
                if (!mark[v.index()]) {
                    mark[v.index()] = 1;
                    level[v.index()] = level[u] + 1;
                    queue.push_back(v.index());
                    touched.push_back(v.index());
                }
            }
        }
        for (const std::size_t t : touched) mark[t] = 0;
        return std::pair(last, depth);
    };
    std::vector<char> scratch(n, 0);
    for (std::size_t root = 0; root < n; ++root) {
        if (seen[root]) continue;
        // Pseudo-peripheral start (George-Liu): repeat BFS while the depth grows.
        std::size_t start = root;
        for (auto [far, depth] = bfs_last(start, scratch);;) {
            const auto [far2, depth2] = bfs_last(far, scratch);
            if (depth2 <= depth) {
                start = far;
                break;
            }
            start = far;
            far = far2;
            depth = depth2;
        }
        std::deque<std::size_t> queue{start};
        seen[start] = 1;
        while (!queue.empty()) {
            const std::size_t u = queue.front();
            queue.pop_front();
            order.push_back(CellId::from_index(u));
            std::vector<std::size_t> next;
            for (const CellId v : adj.row(u)) {
                if (!seen[v.index()]) {
                    seen[v.index()] = 1;
                    next.push_back(v.index());
                }
            }
            std::ranges::sort(next, [&](std::size_t a, std::size_t b) { return std::pair(degree(a), a) < std::pair(degree(b), b); });
            for (const std::size_t v : next) queue.push_back(v);
        }
    }
    std::ranges::reverse(order);
    return *Permutation::from_order(std::move(order));
}

template <std::size_t D>
Permutation RcmOrdering::permutation(const Mesh<D>& m) const {
    return reverse_cuthill_mckee(cell_adjacency(m));
}

std::size_t bandwidth(const Csr<CellId>& adj) {
    std::size_t b = 0;
    for (std::size_t i = 0; i < adj.rows(); ++i) {
        for (const CellId j : adj.row(i)) b = std::max(b, i > j.index() ? i - j.index() : j.index() - i);
    }
    return b;
}

std::size_t profile(const Csr<CellId>& adj) {
    std::size_t p = 0;
    for (std::size_t i = 0; i < adj.rows(); ++i) {
        std::size_t lowest = i;
        for (const CellId j : adj.row(i)) lowest = std::min(lowest, j.index());
        p += i - lowest;
    }
    return p;
}

template <std::size_t D>
Result<Mesh<D>> renumber(const Mesh<D>& mesh, const Permutation& perm) {
    if (perm.size() != mesh.cell_count()) return fail(ErrorCode::InvalidArgument, "permutation size differs from cells");
    const MeshData<D>& d = mesh.data();
    MeshData<D> out;
    out.points = d.points;
    out.regions = d.regions;
    out.media = d.media;
    const std::size_t nc = mesh.cell_count();
    out.sites.resize(nc);
    out.cell_region.resize(nc);
    out.cell_input_site.resize(nc);
    if (!d.site_weight.empty()) out.site_weight.resize(nc);
    for (std::size_t c = 0; c < nc; ++c) {
        const std::size_t o = perm.old_of(CellId::from_index(c)).index();
        out.sites[c] = d.sites[o];
        out.cell_region[c] = d.cell_region[o];
        out.cell_input_site[c] = d.cell_input_site[o];
        if (!d.site_weight.empty()) out.site_weight[c] = d.site_weight[o];
    }
    struct Face {
        std::uint32_t patch;  // 0 for internal (sorted first), 1 + patch for boundary
        CellId owner;
        CellId neighbour;
        std::size_t old;
        bool flip;
    };
    std::vector<Face> faces;
    faces.reserve(mesh.face_count());
    for (const FaceId f : mesh.faces()) {
        CellId o = perm.new_of(mesh.owner(f));
        CellId n = mesh.is_internal(f) ? perm.new_of(mesh.neighbour(f)) : CellId::invalid();
        bool flip = false;
        if (n.valid() && n < o) {
            std::swap(o, n);
            flip = true;
        }
        const std::uint32_t patch = n.valid() ? 0u : 1u + mesh.patch(f).value;
        faces.push_back({patch, o, n, f.index(), flip});
    }
    std::ranges::stable_sort(faces, [](const Face& a, const Face& b) {
        return std::tuple(a.patch, a.owner, a.neighbour) < std::tuple(b.patch, b.owner, b.neighbour);
    });
    if (!d.face_periodic_offset.empty()) out.face_periodic_offset.reserve(faces.size());
    for (const auto& f : faces) {
        std::vector<VertexId> verts(d.face_vertices.row(f.old).begin(), d.face_vertices.row(f.old).end());
        if (f.flip) std::ranges::reverse(verts);
        out.face_vertices.push_row(verts);
        out.owner.push_back(f.owner);
        if (f.neighbour.valid()) out.neighbour.push_back(f.neighbour);
        if (!d.face_periodic_offset.empty()) {
            const Vec<D> off = d.face_periodic_offset[f.old];
            out.face_periodic_offset.push_back(f.flip ? Vec<D>{} - off : off);
        }
    }
    std::size_t start = out.neighbour.size();
    for (const auto& p : d.patches) {
        out.patches.push_back({p.name, start, p.count});
        start += p.count;
    }
    return Mesh<D>::from_data(std::move(out));
}

template Permutation LexicographicOrdering::permutation(const Mesh<2>&) const;
template Permutation LexicographicOrdering::permutation(const Mesh<3>&) const;
template Permutation HilbertOrdering::permutation(const Mesh<2>&) const;
template Permutation HilbertOrdering::permutation(const Mesh<3>&) const;
template Permutation RcmOrdering::permutation(const Mesh<2>&) const;
template Permutation RcmOrdering::permutation(const Mesh<3>&) const;
template Result<Mesh<2>> renumber(const Mesh<2>&, const Permutation&);
template Result<Mesh<3>> renumber(const Mesh<3>&, const Permutation&);

}  // namespace vmm
