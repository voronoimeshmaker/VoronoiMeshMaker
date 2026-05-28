// -----------------------------------------------------------------------------
//  include gtest
// -----------------------------------------------------------------------------
#include <gtest/gtest.h>

// -----------------------------------------------------------------------------
//  include c++
// -----------------------------------------------------------------------------
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <chrono>

// -----------------------------------------------------------------------------
//  include VoronoiMeshMaker
// -----------------------------------------------------------------------------
#include <VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp>
#include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
#include <VoronoiMeshMaker/Boundary2D/Policies/PolygonizePolicy.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/ErrorHandling/VMMException.h>

#include <VoronoiMeshMaker/IO/Options.hpp>
#include <VoronoiMeshMaker/IO/PathUtils.hpp>
#include <VoronoiMeshMaker/IO/Sinks.hpp>
#include <VoronoiMeshMaker/IO/Boundary2D/Writers/VTK_Legacy_PolyLines.hpp>

using namespace vmm::b2d;
using namespace vmm::io;

namespace {

std::string read_file(const std::filesystem::path& p) {
    std::ifstream ifs(p, std::ios::in | std::ios::binary);
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

TEST(IO_VTK_Legacy_Integration, WriteFile_WithPathUtils_AndFileSink) {
    // 1) Geometria simples (1 ring, 4 vértices)
    // ✅ CORREÇÃO: Use o construtor padrão em vez de designated initializers.
    Rectangle R(2.0, 1.0);
    PolygonizePolicy pol{};
    auto ring = R.polygonize(pol);

    Boundary2DData data;
    data.append_ring(std::span<const Point2>(ring.data(), ring.size()),
                     LoopKind::Outer, RegionId{42});

    // 2) Opções: Legacy ASCII
    VtkOptions opt{};
    opt.dialect   = Dialect::Legacy;
    opt.topology  = Topology::PolyLines;
    opt.encoding  = Encoding::ASCII;
    opt.precision = 12;
    opt.cell_data = true;

    // 3) Pasta temporária (estável o suficiente para testes locais/CI)
    const auto tmp = std::filesystem::temp_directory_path()
                   / ("vmm_io_ut_" + std::to_string(
                       std::chrono::steady_clock::now().time_since_epoch().count()));

    // 4) Resolve caminho final e garante criação de pasta + extensão .vtk
    const auto path = ::vmm::io::join_with_extension(tmp.string(), "boundary_io_test", opt);

    // 5) Writer + FileSink
    {
        FileSink sink(path.string());
        VTK_Legacy_PolyLinesWriter{}(data, opt, sink);
    }

    // 6) Verifica que o arquivo existe e não está vazio
    ASSERT_TRUE(std::filesystem::exists(path));
    ASSERT_GT(std::filesystem::file_size(path), 0u);

    // 7) Verificações estruturais básicas no conteúdo
    const std::string s = read_file(path);
    const std::size_t pc = static_cast<std::size_t>(data.vertex_count()); // 4
    const std::size_t rc = static_cast<std::size_t>(data.ring_count());   // 1
    const std::size_t total_idx = 1 + (pc + 1); // len + n + repeat-first = 6

    EXPECT_NE(s.find("# vtk DataFile Version"), std::string::npos);
    EXPECT_NE(s.find("DATASET POLYDATA"), std::string::npos);

    {
        std::ostringstream expect;
        expect << "POINTS " << pc << " double";
        EXPECT_NE(s.find(expect.str()), std::string::npos);
    }
    {
        std::ostringstream expect;
        expect << "LINES " << rc << " " << total_idx;
        EXPECT_NE(s.find(expect.str()), std::string::npos);
    }
    EXPECT_NE(s.find("2.000000000000 1.000000000000 0.0"), std::string::npos);
    EXPECT_EQ(s.find("1.000000000000 2.000000000000 0.0"), std::string::npos);

    EXPECT_NE(s.find("CELL_DATA 1"), std::string::npos);
    EXPECT_NE(s.find("SCALARS loop_kind int 1"), std::string::npos);
    EXPECT_NE(s.find("SCALARS region_id int 1"), std::string::npos);
    EXPECT_NE(s.find("\n0\n"), std::string::npos);  // Outer
    EXPECT_NE(s.find("\n42\n"), std::string::npos); // RegionId{42}
}

TEST(IO_PathUtils, EmptyFilename_Throws) {
    VtkOptions opt{};
    EXPECT_THROW({
        (void)::vmm::io::join_with_extension("", "", opt);
    }, ::vmm::error::VMMException);
}

TEST(IO_PathUtils, EmptyFolder_UsesCwd_AndAddsExtension) {
    VtkOptions opt{};
    opt.dialect = Dialect::Legacy;

    const auto p = ::vmm::io::join_with_extension("", "just_name", opt);
    // Deve ter extensão .vtk quando legado
    EXPECT_EQ(p.extension().string(), ".vtk");
    // E deve estar relativo ao cwd (parent_path pode ser vazia ou cwd-resolvida,
    // então checamos apenas se o filename corresponde)
    EXPECT_EQ(p.filename().string(), "just_name.vtk");
}

} // namespace
