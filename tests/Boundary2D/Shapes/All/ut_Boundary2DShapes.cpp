#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Capsule.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Circle.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ellipse.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Polygon.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/RegularNGon.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ring2D.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/RoundedRect.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Triangle.hpp>
#include <VoronoiMeshMaker/Boundary2D/Validation/Boundary2DValidation.hpp>
#include <VoronoiMeshMaker/ErrorHandling/VMMException.h>
#include <VoronoiMeshMaker/IO/Boundary2D/Boundary2DExport.hpp>

using namespace vmm::b2d;
using namespace vmm::io;
using vmm::error::VMMException;

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

std::filesystem::path temp_output_dir() {
    const auto dir = std::filesystem::temp_directory_path() / "vmm_boundary2d_shapes_vtk";
    std::filesystem::create_directories(dir);
    return dir;
}

void expect_vtk_shape(const std::string& label,
                      const Boundary2DData& boundary,
                      Index expected_rings,
                      Index expected_vertices)
{
    ASSERT_TRUE(boundary.invariant_ok()) << label;
    EXPECT_EQ(boundary.ring_count(), expected_rings) << label;
    EXPECT_EQ(boundary.vertex_count(), expected_vertices) << label;
    EXPECT_TRUE(is_valid_boundary_minimal(boundary)) << label;
    EXPECT_GT(boundary_area(boundary), 0.0) << label;

    VtkOptions options{};
    options.precision = 12;
    options.cell_data = true;

    const auto path = write_boundary_vtk_legacy(boundary,
                                                temp_output_dir().string(),
                                                label,
                                                options);
    ASSERT_TRUE(std::filesystem::exists(path)) << label;
    ASSERT_GT(std::filesystem::file_size(path), 0U) << label;

    const auto vtk = read_file(path);
    EXPECT_NE(vtk.find("DATASET POLYDATA"), std::string::npos) << label;
    EXPECT_NE(vtk.find("SCALARS loop_kind int 1"), std::string::npos) << label;
    EXPECT_NE(vtk.find("SCALARS region_id int 1"), std::string::npos) << label;

    std::ostringstream points;
    points << "POINTS " << expected_vertices << " double";
    EXPECT_NE(vtk.find(points.str()), std::string::npos) << label;

    std::ostringstream lines;
    lines << "LINES " << expected_rings << " ";
    EXPECT_NE(vtk.find(lines.str()), std::string::npos) << label;
}

template <class Shape>
void expect_shape_path(const std::string& label,
                       const Shape& shape,
                       Index expected_rings,
                       Index expected_vertices)
{
    const auto boundary = make_boundary(shape, PolygonizePolicy{}, RegionId{7});
    expect_vtk_shape(label, boundary, expected_rings, expected_vertices);
}

} // namespace

TEST(Boundary2DShapes, AllPlannedShapesGenerateValidBoundaryDataAndVtk) {
    expect_shape_path("rectangle", Rectangle(2.0, 1.0), 1, 4);
    expect_shape_path("circle", Circle(Point2{0.0, 0.0}, 1.0, 32), 1, 32);
    expect_shape_path("ellipse", Ellipse(Point2{0.0, 0.0}, 2.0, 1.0, 40), 1, 40);
    expect_shape_path("triangle", Triangle(Point2{0.0, 0.0},
                                           Point2{1.0, 0.0},
                                           Point2{0.0, 1.0}), 1, 3);
    expect_shape_path("polygon", Polygon{
        Point2{0.0, 0.0},
        Point2{2.0, 0.0},
        Point2{2.0, 1.0},
        Point2{0.0, 1.0}
    }, 1, 4);
    expect_shape_path("regular_ngon", RegularNGon(Point2{0.0, 0.0}, 7, 1.0), 1, 7);
    expect_shape_path("capsule", Capsule(Point2{0.0, 0.0}, 2.0, 0.5, 12), 1, 26);
    expect_shape_path("rounded_rect", RoundedRect(Point2{0.0, 0.0}, 3.0, 2.0, 0.25, 6), 1, 28);
    expect_shape_path("ring2d", Ring2D(Point2{0.0, 0.0}, 0.5, 2.0, 48), 2, 96);
}

TEST(Boundary2DShapes, ShapeExporterCanWriteDirectlyFromShape) {
    const auto path = write_shape_vtk_legacy(Ring2D(0.5, 1.5, 24),
                                             temp_output_dir().string(),
                                             "direct_ring2d",
                                             VtkOptions{},
                                             PolygonizePolicy{},
                                             RegionId{11});

    ASSERT_TRUE(std::filesystem::exists(path));
    const auto vtk = read_file(path);
    EXPECT_NE(vtk.find("POINTS 48 double"), std::string::npos);
    EXPECT_NE(vtk.find("LINES 2 "), std::string::npos);
    EXPECT_NE(vtk.find("\n11\n"), std::string::npos);
}

TEST(Boundary2DShapes, RejectInvalidInputs) {
    EXPECT_THROW(Circle(0.0), VMMException);
    EXPECT_THROW(Ellipse(1.0, 0.0), VMMException);
    EXPECT_THROW(Triangle(Point2{0.0, 0.0},
                          Point2{1.0, 0.0},
                          Point2{2.0, 0.0}), VMMException);
    EXPECT_THROW(Polygon({Point2{0.0, 0.0}, Point2{1.0, 0.0}}), VMMException);
    EXPECT_THROW(RegularNGon(2, 1.0), VMMException);
    EXPECT_THROW(Capsule(0.0, 1.0), VMMException);
    EXPECT_THROW(RoundedRect(1.0, 1.0, 2.0), VMMException);
}
