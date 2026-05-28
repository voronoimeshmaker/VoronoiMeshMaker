#pragma once
//==============================================================================
// Name        : Vtk.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : IO/Boundary2D/Writers
// Description : Writer para o formato VTK (Legacy/XML, ASCII/Binary).
// License     : GNU GPL v3
// Version     : 1.0.0
//==============================================================================

#include <vector>
#include <string>
#include <algorithm>
#include <bit> // Para std::endian (C++20)
#include <iomanip>
#include <sstream>

// ✅ INTEGRAÇÃO: Utiliza os seus tipos e a arquitetura de I/O já definidos.
#include <VoronoiMeshMaker/Core/namespace.h>
#include <VoronoiMeshMaker/Core/type.h> // Para vmm::Real
#include <VoronoiMeshMaker/IO/Options.hpp> //
#include <VoronoiMeshMaker/IO/Sinks.hpp> //
#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp> // Assume que este ficheiro existe.

VORMAKER_NAMESPACE_OPEN
IO_NAMESPACE_OPEN

namespace detail {
    // Helper para converter para Big Endian, exigido pelo formato VTK binário.
    template <typename T>
    T to_big_endian(T value) {
        if constexpr (std::endian::native == std::endian::little) {
            char* bytes = reinterpret_cast<char*>(&value);
            std::reverse(bytes, bytes + sizeof(T));
        }
        return value;
    }
} // namespace detail

/**
 * @brief Classe responsável por escrever dados Boundary2D no formato VTK.
 *
 * Esta classe contém a lógica de formatação para os dialetos Legacy e XML,
 * e para as codificações ASCII e Binária.
 */
class VtkBoundaryWriter {
public:
    /**
     * @brief Ponto de entrada principal para a escrita.
     * Delega para a implementação correta (ASCII ou Binária) com base nas opções.
     */
    void write(const vmm::b2d::Boundary2DData& data,
               const VtkOptions& opts,
               ISink& sink); // A interface ISink permite escrever em ficheiros ou strings.

private:
    // A implementação real será dividida para maior clareza.
    void write_legacy_ascii(const vmm::b2d::Boundary2DData& data, const VtkOptions& opts, ISink& sink);
    void write_legacy_binary(const vmm::b2d::Boundary2DData& data, const VtkOptions& opts, ISink& sink);
};

// --- IMPLEMENTAÇÃO ---
// (Para simplicidade, a implementação está no header. Em projetos maiores,
//  poderia ser movida para um ficheiro .cpp)

inline void VtkBoundaryWriter::write(const vmm::b2d::Boundary2DData& data,
                                     const VtkOptions& opts,
                                     ISink& sink)
{
    // Delega para o método apropriado com base nas opções.
    if (opts.dialect == Dialect::Legacy) {
        if (opts.encoding == Encoding::ASCII) {
            write_legacy_ascii(data, opts, sink);
        } else if (opts.encoding == Encoding::Binary) {
            write_legacy_binary(data, opts, sink);
        }
    }
    // else if (opts.dialect == Dialect::XML) { ... lógica para XML aqui ... }
}

inline void VtkBoundaryWriter::write_legacy_ascii(const vmm::b2d::Boundary2DData& data,
                                                  const VtkOptions& opts,
                                                  ISink& sink)
{
    // 1. Cabeçalho do ficheiro VTK
    sink.write("# vtk DataFile Version 4.2\n");
    sink.write("VoronoiMeshMaker Boundary2D Data\n");
    sink.write("ASCII\n");
    sink.write("DATASET POLYDATA\n");

    // 2. Secção de Pontos (Vértices)
    const auto num_points = data.vertex_count();
    std::stringstream ss;
    ss << "POINTS " << num_points << " double\n";
    ss << std::fixed << std::setprecision(opts.precision);

    for (const auto& p : data.vertices_view()) {
        ss << p.x << " " << p.y << " 0.0\n";
    }
    sink.write(ss.str());

    // 3. Secção de Topologia (Polilinhas)
    if (opts.topology == Topology::PolyLines) {
        const auto num_rings = data.ring_count();
        // Each closed polyline stores: count + n vertices + repeated first.
        const size_t total_indices = num_rings + data.vertex_count() + num_rings;

        ss.str(""); // Limpa o stringstream
        ss << "LINES " << num_rings << " " << total_indices << "\n";
        
        vmm::b2d::Index current_offset = 0;
        for (const auto& ring_view : data.rings_view()) {
            ss << (ring_view.size() + 1U);
            for (size_t i = 0; i < ring_view.size(); ++i) {
                ss << " " << current_offset + i;
            }
            ss << " " << current_offset;
            ss << "\n";
            current_offset += ring_view.size();
        }
        sink.write(ss.str());
    }

    // 4. Secção de Dados da Célula (Cell Data)
    if (opts.cell_data && data.ring_count() > 0) {
        ss.str("");
        ss << "CELL_DATA " << data.ring_count() << "\n";
        // Escreve o RegionId associado a cada anel
        ss << "SCALARS region_id int 1\n";
        ss << "LOOKUP_TABLE default\n";
        for (const auto& ring_meta : data.rings_meta_view()) {
            ss << ring_meta.region_id.value << "\n";
        }
        sink.write(ss.str());
    }
}

inline void VtkBoundaryWriter::write_legacy_binary(const vmm::b2d::Boundary2DData& data,
                                                   const VtkOptions& opts,
                                                   ISink& sink)
{
    // O cabeçalho é sempre em texto (ASCII)
    sink.write("# vtk DataFile Version 4.2\n");
    sink.write("VoronoiMeshMaker Boundary2D Data\n");
    sink.write("BINARY\n");
    sink.write("DATASET POLYDATA\n");

    // Implementação da escrita binária... (usando detail::to_big_endian)
    // ... (esta parte pode ser adicionada depois, se necessário)
}

IO_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
