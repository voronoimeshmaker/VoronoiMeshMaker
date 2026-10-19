// SPDX-License-Identifier: BSD-3-Clause
#pragma once
//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/csr.hpp>
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
    std::vector<Vec2> base_generator = {};
    std::vector<std::uint8_t> site_is_centroid = {};
    /// Optional constant subdivision elevations per column (flat-column geometry).
    std::vector<std::vector<Real>> column_levels = {};
    /// First nonempty nominal layer per column; layer count denotes an empty column.
    std::vector<std::size_t> kfloor = {};
};
/// Immutable mesh with explicit columns; identifiers are not physical properties.
class LayeredMesh {
public:
    /// @brief Validates metadata, sampled support, closure and cell orientation.
    /// @param data Geometry and provenance to freeze.
    /// @return Immutable mesh or an inconsistency error.
    /// @note This is not a general proof for arbitrary transformed external faces.
    /// @par Level
    /// Intermediate
    /// @par Location
    /// vmm/mesh/layered.hpp
    /// @par Examples
    /// ex_horizons.cpp (through generate_layered_mesh)
    /// @sa generate_layered_mesh
    [[nodiscard]] static Result<LayeredMesh> from_data(LayeredData data);
    /// Read-only volumetric geometry.
    [[nodiscard]] const Mesh3D& mesh() const noexcept { return data_.mesh; }
    /// Read-only reference and provenance.
    [[nodiscard]] const LayeredData& data() const noexcept { return data_; }
    /// Cells sorted bottom to top; empty for an absent or unknown column.
    [[nodiscard]] std::vector<CellId> cells_in_column(std::size_t column) const;
    /// Returns all neighbours across horizontal subdivision surfaces.
    [[nodiscard]] std::vector<CellId> vertical_neighbours(CellId cell) const;
    /// Constant-time view, valid until this immutable object is destroyed or assigned.
    [[nodiscard]] std::span<const CellId> cells_in_column_view(std::size_t column) const noexcept;
    /// Constant-time view of unique vertical neighbours, sorted by cell ID.
    [[nodiscard]] std::span<const CellId> vertical_neighbours_view(CellId cell) const noexcept;
private:
    LayeredData data_;
    Csr<CellId> columns_;
    Csr<CellId> vertical_;

};
/// Derived geometry for one column; empty columns have zero minimum thickness.
struct ColumnQuality {
    Real min_thickness = 0;
    Real max_interface_slope = 0; ///< Angle above the xy plane, radians.
    Real max_vertical_skew = 0; ///< Lateral-face site-vector inclination, radians.
    std::size_t absent_layers = 0; ///< Nominal layers without a cell, regardless of cause.
};
/// Computes quality from present geometry; does not infer missing generation history.
[[nodiscard]] std::vector<ColumnQuality> column_quality_report(const LayeredMesh& layered);
} // namespace vmm
