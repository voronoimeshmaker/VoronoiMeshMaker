// SPDX-License-Identifier: BSD-3-Clause
#pragma once
//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <limits>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/geometry/horizons.hpp>
#include <vmm/mesh/mesh.hpp>
namespace vmm {
inline constexpr std::size_t lateral_face = std::numeric_limits<std::size_t>::max();
/// Serializable geometry and provenance. Layers are numbered bottom to top.
/// face_level identifies a horizontal subdivision surface, or lateral_face.
struct LayeredData {
    Mesh3D mesh;
    HorizonGrid horizons;
    std::size_t column_count = 0;
    std::vector<std::vector<Real>> fractions;
    std::vector<std::size_t> column;
    std::vector<std::size_t> layer;
    std::vector<std::size_t> face_level;
};
/// Immutable mesh with explicit columns; identifiers are not physical properties.
class LayeredMesh {
public:
    [[nodiscard]] static Result<LayeredMesh> from_data(LayeredData data);
    [[nodiscard]] const Mesh3D& mesh() const noexcept { return data_.mesh; }
    [[nodiscard]] const LayeredData& data() const noexcept { return data_; }
    [[nodiscard]] std::vector<CellId> cells_in_column(std::size_t column) const;
    /// Returns all neighbours across horizontal subdivision surfaces.
    [[nodiscard]] std::vector<CellId> vertical_neighbours(CellId cell) const;
private:
    LayeredData data_;
};
} // namespace vmm
