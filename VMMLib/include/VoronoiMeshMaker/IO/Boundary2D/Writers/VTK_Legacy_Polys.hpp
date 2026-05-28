#pragma once
//==============================================================================
// Name        : VTK_Legacy_Polys.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : IO / Boundary2D / Writers
// Description : Writer VTK Legacy (.vtk) POLYDATA + POLYGONS (triangulado).
// License     : GNU GPL v3
// Version     : 1.0.0
//==============================================================================

/**
 * @file VTK_Legacy_Polys.hpp
 * @brief Serializa uma fronteira triangulada como VTK Legacy (.vtk) com células POLYGONS.
 */

#include <string>
#include <sstream>
#include <iomanip>

// ✅ INTEGRAÇÃO: Inclui os componentes de I/O necessários.
#include <VoronoiMeshMaker/Core/namespace.h>
#include <VoronoiMeshMaker/IO/Options.hpp>
#include <VoronoiMeshMaker/IO/Sinks.hpp>
// ✅ DEPENDÊNCIA PRINCIPAL: A visão/política de triangulação.
#include <VoronoiMeshMaker/IO/Boundary2D/Topology/TriangulatePolicy.hpp>

#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>

VORMAKER_NAMESPACE_OPEN
IO_NAMESPACE_OPEN

/**
 * @brief Functor que escreve uma fronteira triangulada no formato VTK Legacy (ASCII).
 */
struct VTK_Legacy_PolysWriter {
    template <class Sink>
    void operator()( const TriangulatedView& view,
                      const VtkOptions& opt,
                      Sink& sink) const
    {
        using namespace ::vmm::b2d;
        using ::vmm::error::CoreErr;

        // Acessa os dados originais (coordenadas dos pontos) através da 'view'.
        const auto& b2d = view.data();

        if (opt.encoding != Encoding::ASCII) {
            VMM_THROW(CoreErr::NotImplemented, {{"where","VTK_Legacy_PolysWriter(non-ASCII)"}});
        }

        // --- Header VTK ---
        sink.write("# vtk DataFile Version 4.2\n");
        sink.write("VMM Boundary2D (Triangulated Polygons)\n");
        sink.write("ASCII\n");
        sink.write("DATASET POLYDATA\n");

        std::stringstream ss;

        // --- POINTS ---
        // Os pontos são os mesmos do Boundary2DData original.
        ss << "POINTS " << b2d.vertex_count() << " double\n";
        ss << std::fixed << std::setprecision(opt.precision);
        for (const auto& p : b2d.points) {
            ss << static_cast<double>(p.x) << " "
               << static_cast<double>(p.y) << " 0.0\n";
        }
        sink.write(ss.str());
        ss.str(""); // Limpa o stream

        // --- POLYGONS ---
        // Esta é a principal diferença: escrevemos a lista de triângulos.
        ss << "POLYGONS " << view.num_triangles() << " " << view.total_connectivity_size() << "\n";
        for (const auto& tri_indices : view.triangles()) {
            // Cada polígono (triângulo) é definido por '3' seguido dos 3 índices.
            ss << "3 " << tri_indices[0] << " " << tri_indices[1] << " " << tri_indices[2] << "\n";
        }
        sink.write(ss.str());
        ss.str("");

        // --- CELL_DATA ---
        // Os dados agora são por triângulo, não por anel.
        if (opt.cell_data && view.num_triangles() > 0) {
            ss << "CELL_DATA " << view.num_triangles() << "\n";
            ss << "SCALARS region_id int 1\n";
            ss << "LOOKUP_TABLE default\n";
            for (const auto& region : view.triangle_regions()) {
                ss << static_cast<int>(region.value) << "\n";
            }
            sink.write(ss.str());
        }
        sink.flush();
    }
};

IO_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
