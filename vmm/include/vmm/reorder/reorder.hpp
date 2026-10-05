// ============================================================================
// File: reorder.hpp
// Description: Cell reordering (P11 task 4): explicit permutation and its
//              inverse, ordering policies (open concept) and renumbering of
//              a mesh that keeps the face conventions (DEC-029).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <concepts>
#include <cstddef>
#include <span>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/csr.hpp>
#include <vmm/core/types.hpp>
#include <vmm/error/error.hpp>
#include <vmm/mesh/mesh.hpp>

namespace vmm {

/// Bijection of cell ids: new_of_old[old] = new, old_of_new[new] = old.
class Permutation {
public:
    Permutation() = default;
    /// Builds from old_of_new (the new order listed by old ids); fails unless it is a bijection.
    [[nodiscard]] static Result<Permutation> from_order(std::vector<CellId> old_of_new);
    [[nodiscard]] static Permutation identity(std::size_t n);

    [[nodiscard]] std::size_t size() const noexcept { return old_of_new_.size(); }
    [[nodiscard]] CellId new_of(CellId old_id) const { return new_of_old_.at(old_id.index()); }
    [[nodiscard]] CellId old_of(CellId new_id) const { return old_of_new_.at(new_id.index()); }
    [[nodiscard]] std::span<const CellId> new_of_old() const noexcept { return new_of_old_; }
    [[nodiscard]] std::span<const CellId> old_of_new() const noexcept { return old_of_new_; }
    [[nodiscard]] Permutation inverse() const;

private:
    std::vector<CellId> new_of_old_;
    std::vector<CellId> old_of_new_;
};

template <class O, std::size_t D>
concept Ordering = requires(const O& o, const Mesh<D>& m) {
    { o.permutation(m) } -> std::same_as<Permutation>;
};

struct IdentityOrdering {
    template <std::size_t D>
    [[nodiscard]] Permutation permutation(const Mesh<D>& m) const { return Permutation::identity(m.cell_count()); }
};

/// Cells sorted by generator position (x, then y, ...).
struct LexicographicOrdering {
    template <std::size_t D>
    [[nodiscard]] Permutation permutation(const Mesh<D>& m) const;
};

/// Cells sorted along a Hilbert curve through the generators (2D; 3D uses Morton order).
struct HilbertOrdering {
    int bits = 16;
    template <std::size_t D>
    [[nodiscard]] Permutation permutation(const Mesh<D>& m) const;
};

/// Reverse Cuthill-McKee on the cell adjacency, from a pseudo-peripheral cell
/// of each connected component; ties broken by degree then id (deterministic).
struct RcmOrdering {
    template <std::size_t D>
    [[nodiscard]] Permutation permutation(const Mesh<D>& m) const;
};

/// Reverse Cuthill-McKee on any symmetric adjacency.
[[nodiscard]] Permutation reverse_cuthill_mckee(const Csr<CellId>& adjacency);

/// max |i - j| over the adjacency.
[[nodiscard]] std::size_t bandwidth(const Csr<CellId>& adjacency);

/// Sum over rows of (i - min column in row, if smaller).
[[nodiscard]] std::size_t profile(const Csr<CellId>& adjacency);

/// @brief Renumbers the cells of a mesh with a permutation.
/// @param mesh Mesh.
/// @param permutation New order of the cells (e.g. RcmOrdering{}.permutation(mesh)).
/// @return The renumbered mesh; internal faces stay upper-triangular and boundary faces grouped by patch.
/// @par Level
/// Intermediate
/// @sa RcmOrdering, HilbertOrdering, Permutation
/// @par Location
/// vmm/reorder/reorder.hpp
template <std::size_t D>
[[nodiscard]] Result<Mesh<D>> renumber(const Mesh<D>& mesh, const Permutation& permutation);

// Explicit instantiation declarations control linkage; the public templates above
// are the API documentation. Do not render these as additional overloads.
/// @cond VMM_EXPLICIT_INSTANTIATIONS
extern template Permutation LexicographicOrdering::permutation(const Mesh<2>&) const;
extern template Permutation LexicographicOrdering::permutation(const Mesh<3>&) const;
extern template Permutation HilbertOrdering::permutation(const Mesh<2>&) const;
extern template Permutation HilbertOrdering::permutation(const Mesh<3>&) const;
extern template Permutation RcmOrdering::permutation(const Mesh<2>&) const;
extern template Permutation RcmOrdering::permutation(const Mesh<3>&) const;
extern template Result<Mesh<2>> renumber(const Mesh<2>&, const Permutation&);
extern template Result<Mesh<3>> renumber(const Mesh<3>&, const Permutation&);
/// @endcond

}  // namespace vmm
