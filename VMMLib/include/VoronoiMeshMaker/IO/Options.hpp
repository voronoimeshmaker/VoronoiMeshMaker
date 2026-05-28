#pragma once
//==============================================================================
// Name        : Options.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : IO
// Description : Enums e opções para exportação (dialeto, topologia, encoding).
// License     : GNU GPL v3
// Version     : 0.2.0
//==============================================================================

/**
 * @file Options.hpp
 * @brief Define enums e opções comuns usadas pelos exportadores (VTK/VisIt).
 */

#include <VoronoiMeshMaker/Core/namespace.h>

VORMAKER_NAMESPACE_OPEN
IO_NAMESPACE_OPEN // Usa a sua macro de namespace.h

/** * @brief Dialeto/formato alvo do ecossistema VTK.
 * Define o tipo de ficheiro que será gerado.
 */
enum class Dialect : unsigned char {
    Legacy,  ///< VTK legacy (.vtk), simples e amplamente compatível.
    XML      ///< VTK XML PolyData (.vtp), mais moderno e flexível.
};

/**
 * @brief Topologia a ser escrita no ficheiro de saída.
 * Define como os dados geométricos são representados.
 */
enum class Topology : unsigned char {
    PolyLines, ///< Contornos (rings) como linhas fechadas. Ideal para visualizar fronteiras.
    Polys      ///< Superfícies preenchidas (requer triangulação). Marcado como futuro.
};

/**
 * @brief Codificação dos dados numéricos.
 * Permite escolher entre ficheiros legíveis por humanos ou binários compactos.
 */
enum class Encoding : unsigned char {
    ASCII,       ///< Texto legível, excelente para depuração.
    Binary,      ///< Binário Big-Endian do formato Legacy (.vtk). Rápido e compacto.
    AppendedRaw  ///< Binário "anexado" do formato XML (.vtp), sem a sobrecarga do Base64.
};

/**
 * @brief Agrega todas as opções de exportação para o formato VTK.
 * Esta struct é passada para as funções de escrita para controlar o seu comportamento.
 */
struct VtkOptions {
    // --- Configuração do Formato ---
    Dialect  dialect      { Dialect::Legacy };      ///< Dialeto padrão.
    Topology topology     { Topology::PolyLines };  ///< Topologia padrão.
    Encoding encoding     { Encoding::ASCII };      ///< Codificação padrão para segurança e depuração.

    // --- Configuração da Formatação ---
    int      precision    { 17 };    ///< Precisão máxima para double em ASCII, garantindo reprodutibilidade.
    bool     write_z      { false }; ///< Se true, emite uma coordenada z=0 para compatibilidade com software 3D.
    bool     cell_data    { true };  ///< Controla se dados associados às células (como RegionId) são escritos.

    // --- Configuração de camadas geométricas para diagramas 2D completos ---
    bool     write_boundary       { true }; ///< Escreve o contorno do domínio.
    bool     write_sites          { true }; ///< Escreve os pontos geradores.
    bool     write_delaunay_edges { true }; ///< Escreve as arestas da triangulação de Delaunay.
    bool     write_voronoi_cells  { true }; ///< Escreve os volumes de Voronoi recortados.

    // --- Configuração da Execução ---
    bool     deterministic{ true };  ///< Garante que a saída seja sempre a mesma para a mesma entrada, essencial para testes.
    bool     parallel_prep{ false }; ///< Flag para otimizações futuras: permite paralelizar a preparação dos dados antes da escrita.
};

IO_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
