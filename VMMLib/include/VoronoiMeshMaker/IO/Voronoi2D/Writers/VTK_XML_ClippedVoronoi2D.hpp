// SPDX-License-Identifier: GPL-3.0-or-later
/** @file VTK_XML_ClippedVoronoi2D.hpp
 * @brief Shared-vertex, polygon-only XML unstructured grid export.
 * @ingroup io_voronoi2d
 */
#pragma once
#include <filesystem>
#include <span>
#include <string_view>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiDiagram2D.hpp>
namespace vmm::io {
struct VtkXmlClippedOptions2D {
    bool ascii{false}; ///< Otherwise raw appended data with lossless zlib compression.
};
struct VtkXmlClippedSummary2D {
    std::size_t point_count{};
    std::size_t cell_count{};
    std::uintmax_t file_bytes{};
};
/** Borrowed, interleaved tuples in diagram.cells order. The name and values must
 * remain valid until the writer returns. NaN is permitted for unavailable fields.
 * Names must be printable ASCII and unique, including the built-in cell arrays.
 */
struct VtkXmlCellDataArray2D {
    std::string_view name;
    std::span<const s2d::Real> values;
    int components{1};
};
/** Cell classes: 0 internal, 1 one boundary face, 2 multiple boundary faces.
 * TPFA offset is the maximum |crossing_parameter - 0.5| on internal faces.
 * TPFA crossing is true only if every internal generator segment crosses its face.
 * All floating-point arrays retain the precision of s2d::Real.
 */
[[nodiscard]] VtkXmlClippedSummary2D write_clipped_voronoi_vtu(
    const vd2d::ClippedVoronoiDiagram2D& diagram,
    const std::filesystem::path& path, VtkXmlClippedOptions2D options = {});
/** Add fields without replacing built-in geometry diagnostics. Invalid names,
 * component counts or tuple counts throw before opening the output file.
 * Extra fields do not alter the shared points or cell connectivity blocks.
 */
[[nodiscard]] VtkXmlClippedSummary2D write_clipped_voronoi_vtu(
    const vd2d::ClippedVoronoiDiagram2D& diagram,
    const std::filesystem::path& path, VtkXmlClippedOptions2D options,
    std::span<const VtkXmlCellDataArray2D> extra_cell_arrays);
}
