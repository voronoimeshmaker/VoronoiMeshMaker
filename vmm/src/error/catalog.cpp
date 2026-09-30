// ============================================================================
// File: catalog.cpp
// Description: Message catalogue (pt/en).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <array>
#include <atomic>
#include <optional>
#include <span>
#include <utility>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/error/catalog.hpp>

namespace vmm {
namespace {

using Row = std::pair<ErrorCode, CatalogEntry>;

// Sorted by code; the tests check order, uniqueness and both texts.
constexpr std::array kCatalog{
    Row{ErrorCode::InvalidArgument, {"argumento inválido", "invalid argument"}},
    Row{ErrorCode::InvalidLengthScale, {"escala de comprimento inválida (precisa ser finita e positiva)",
                                        "invalid length scale (must be finite and positive)"}},
    Row{ErrorCode::IndexOutOfRange, {"índice fora do intervalo", "index out of range"}},
    Row{ErrorCode::EmptyDeclaration, {"declaração de domínio sem regiões", "domain declaration without regions"}},
    Row{ErrorCode::DegenerateShape, {"forma degenerada (área nula ou parâmetros inválidos)",
                                     "degenerate shape (zero area or invalid parameters)"}},
    Row{ErrorCode::InvalidPolygon, {"polígono inválido (menos de 3 vértices, auto-interseção ou coordenada não finita)",
                                    "invalid polygon (fewer than 3 vertices, self-intersection or non-finite coordinate)"}},
    Row{ErrorCode::DomainVoid, {"o domínio tem um vazio não coberto por nenhuma região",
                                "the domain has a void covered by no region"}},
    Row{ErrorCode::RegionEmptied, {"região anulada pela ordem de precedência", "region emptied by the precedence order"}},
    Row{ErrorCode::RegionFragmented, {"região dividida em várias componentes", "region split into several components"}},
    Row{ErrorCode::Sliver, {"lasca de região abaixo da escala local", "region sliver below the local scale"}},
    Row{ErrorCode::UnknownShape, {"forma desconhecida no registro", "unknown shape in the registry"}},
    Row{ErrorCode::InvalidShapeParameter, {"parâmetro de forma inválido", "invalid shape parameter"}},
    Row{ErrorCode::UnknownMedium, {"meio desconhecido", "unknown medium"}},
    Row{ErrorCode::DuplicateName, {"nome repetido", "duplicate name"}},
    Row{ErrorCode::InvalidSurface, {"superfície inválida (aberta, com orientação inconsistente, com auto-interseção ou com "
                                    "triângulo degenerado)",
                                    "invalid surface (open, inconsistently oriented, self-intersecting or with a degenerate "
                                    "triangle)"}},
    Row{ErrorCode::SiteOutsideRegion, {"sítio fora do interior da sua região", "site outside the interior of its region"}},
    Row{ErrorCode::DuplicateSite, {"sítio repetido", "duplicate site"}},
    Row{ErrorCode::RegionWithoutSites, {"componente de região sem sítios", "region component without sites"}},
    Row{ErrorCode::SiteGenerationFailed, {"não foi possível gerar os sítios pedidos", "the requested sites could not be generated"}},
    Row{ErrorCode::InvalidSpacing, {"espaçamento de sítios inválido", "invalid site spacing"}},
    Row{ErrorCode::InvariantViolated, {"invariante da malha violado", "mesh invariant violated"}},
    Row{ErrorCode::FragmentWithoutNeighbour, {"fragmento de célula sem vizinho na mesma região",
                                              "cell fragment without a neighbour in the same region"}},
    Row{ErrorCode::InterfaceNotConforming, {"interface não conforme", "non-conforming interface"}},
    Row{ErrorCode::UnresolvedLabel, {"rótulo de aresta sem construção canônica", "edge label without a canonical construction"}},
    Row{ErrorCode::FileOpenFailed, {"não foi possível abrir o arquivo", "could not open the file"}},
    Row{ErrorCode::ParseError, {"erro de leitura do arquivo", "file parse error"}},
    Row{ErrorCode::UnsupportedVersion, {"versão de formato não suportada", "unsupported format version"}},
    Row{ErrorCode::InconsistentData, {"dados inconsistentes no arquivo", "inconsistent data in the file"}},
    Row{ErrorCode::BackendFailure, {"falha do backend geométrico", "geometric backend failure"}},
    Row{ErrorCode::InternalError, {"erro interno (invariante violado; por favor, reporte)",
                                   "internal error (invariant violated; please report)"}},
};

constexpr auto kCodes = [] {
    std::array<ErrorCode, kCatalog.size()> codes{};
    for (std::size_t k = 0; k < kCatalog.size(); ++k) codes[k] = kCatalog[k].first;
    return codes;
}();

std::atomic<Language> g_language{Language::Portuguese};

}  // namespace

std::optional<CatalogEntry> catalog_entry(ErrorCode code) noexcept {
    const auto it = std::lower_bound(kCatalog.begin(), kCatalog.end(), code,
                                     [](const Row& r, ErrorCode c) { return r.first < c; });
    if (it == kCatalog.end() || it->first != code) return std::nullopt;
    return it->second;
}

std::span<const ErrorCode> all_error_codes() noexcept { return kCodes; }

void set_language(Language language) noexcept { g_language.store(language, std::memory_order_relaxed); }

Language current_language() noexcept { return g_language.load(std::memory_order_relaxed); }

}  // namespace vmm
