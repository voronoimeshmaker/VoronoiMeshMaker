#pragma once
//==============================================================================
// Name        : VTK_Legacy_PolyLines.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : IO / Boundary2D / Writers
// Description : Writer VTK Legacy (.vtk) POLYDATA + LINES (ASCII).
// License     : GNU GPL v3
// Version     : 0.2.0 (Refined)
//==============================================================================

#include <string>
#include <sstream>
#include <iomanip>

#include <VoronoiMeshMaker/Core/namespace.h>
#include <VoronoiMeshMaker/IO/Options.hpp>
#include <VoronoiMeshMaker/IO/Sinks.hpp>
// ✅ MODIFICADO: Agora depende da View, não diretamente dos dados.
#include <VoronoiMeshMaker/IO/Boundary2D/Topology/PolyLinesView.hpp>

#include <VoronoiMeshMaker/ErrorHandling/Macros.h>
#include <VoronoiMeshMaker/ErrorHandling/CoreErrors.h>

VORMAKER_NAMESPACE_OPEN
IO_NAMESPACE_OPEN

/**
 * @brief Functor writer: VTK Legacy (.vtk) POLYDATA with closed LINES (ASCII).
 */
struct VTK_Legacy_PolyLinesWriter {
    template <class Sink>
    void operator()( const ::vmm::b2d::Boundary2DData& data,
                     const VtkOptions& opt,
                     Sink& sink) const
    {
        // Compatibility path while the IO module is being migrated to topology
        // views. Future code should prefer constructing PolyLinesView explicitly.
        (*this)(PolyLinesView{data}, opt, sink);
    }

    template <class Sink>
    void operator()( const PolyLinesView& view, // ✅ MODIFICADO: Aceita a View
                      const VtkOptions& opt,
                      Sink& sink) const
    {
        using namespace ::vmm::b2d;
        using ::vmm::error::CoreErr;

        // Acesso aos dados através da view
        const auto& b2d = view.data();

        if (!b2d.invariant_ok()) {
            VMM_THROW(CoreErr::AssertFailed, {{"where","b2d.invariant_ok"}});
        }

        if (opt.encoding != Encoding::ASCII) {
            VMM_THROW(CoreErr::NotImplemented, {{"where","VTK_Legacy_Writer(non-ASCII)"}});
        }

        // --- Header VTK ---
        sink.write("# vtk DataFile Version 4.2\n");
        sink.write("VMM Boundary2D (LINES)\n");
        sink.write("ASCII\n");
        sink.write("DATASET POLYDATA\n");

        std::stringstream ss;

        // --- POINTS ---
        ss << "POINTS " << b2d.vertex_count() << " double\n";
        ss << std::fixed << std::setprecision(opt.precision);
        for (const auto& p : b2d.points) {
            ss << static_cast<double>(p.x) << " "
               << static_cast<double>(p.y) << " 0.0\n";
        }
        sink.write(ss.str());
        ss.str(""); // Limpa o stream

        // --- LINES ---
        // VTK Legacy POLYDATA uses the keyword LINES for polyline cells.
        ss << "LINES " << view.num_lines() << " "
           << view.total_connectivity_size() << "\n";
        for (std::size_t r = 0; r < view.num_lines(); ++r) {
            const auto ring_view = b2d.ring(static_cast<Index>(r));
            const auto n = ring_view.size();
            const auto base_index = b2d.ring_off[r];

            if (n < 3) {
                VMM_THROW(CoreErr::InvalidArgument, {{"ring","<3_vertices"}});
            }

            ss << (n + 1); // n + 1 para fechar o anel
            for (std::size_t i = 0; i < n; ++i) ss << " " << (base_index + i);
            ss << " " << base_index << "\n"; // Repete o primeiro índice para fechar
        }
        sink.write(ss.str());
        ss.str("");

        // --- CELL_DATA ---
        if (opt.cell_data && view.num_lines() > 0) {
            ss << "CELL_DATA " << view.num_lines() << "\n";

            ss << "SCALARS loop_kind int 1\nLOOKUP_TABLE default\n";
            for (std::size_t r = 0; r < view.num_lines(); ++r) {
                ss << ((b2d.kinds[r] == LoopKind::Hole) ? 1 : 0) << "\n";
            }

            ss << "SCALARS region_id int 1\nLOOKUP_TABLE default\n";
            for (std::size_t r = 0; r < view.num_lines(); ++r) {
                ss << static_cast<int>(b2d.regions[r].value) << "\n";
            }
            sink.write(ss.str());
        }
        sink.flush();
    }
};

IO_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
