#include <filesystem>
#include <iostream>
#include <string>

#include <VoronoiMeshMaker/Boundary2D/Shapes/Capsule.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Circle.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ellipse.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Polygon.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/RegularNGon.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ring2D.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/RoundedRect.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Triangle.hpp>
#include <VoronoiMeshMaker/IO/Boundary2D/Boundary2DExport.hpp>

namespace b2d = vmm::b2d;
namespace io = vmm::io;

template <class Shape>
void write_shape(const Shape& shape,
                 const std::filesystem::path& output_dir,
                 std::string_view name)
{
    io::VtkOptions options{};
    options.precision = 12;
    options.cell_data = true;

    const auto path = io::write_shape_vtk_legacy(
        shape,
        output_dir.string(),
        name,
        options,
        b2d::PolygonizePolicy{},
        b2d::RegionId{1});

    std::cout << path.string() << '\n';
}

int main() {
#ifdef VMM_EXAMPLE_SOURCE_DIR
    const std::filesystem::path base_dir = VMM_EXAMPLE_SOURCE_DIR;
#else
    const std::filesystem::path base_dir = std::filesystem::current_path();
#endif

    const auto output_dir = base_dir / "vtk_output";
    std::filesystem::create_directories(output_dir);

    write_shape(b2d::Rectangle(2.0, 1.0), output_dir, "rectangle");
    write_shape(b2d::Circle(b2d::Point2{0.0, 0.0}, 1.0, 64), output_dir, "circle");
    write_shape(b2d::Ellipse(b2d::Point2{0.0, 0.0}, 2.0, 1.0, 64), output_dir, "ellipse");
    write_shape(b2d::Triangle(b2d::Point2{0.0, 0.0},
                              b2d::Point2{1.0, 0.0},
                              b2d::Point2{0.0, 1.0}), output_dir, "triangle");
    write_shape(b2d::Polygon{
                    b2d::Point2{0.0, 0.0},
                    b2d::Point2{2.0, 0.0},
                    b2d::Point2{2.0, 1.0},
                    b2d::Point2{0.0, 1.0}},
                output_dir,
                "polygon");
    write_shape(b2d::RegularNGon(b2d::Point2{0.0, 0.0}, 7, 1.0), output_dir, "regular_ngon");
    write_shape(b2d::Capsule(b2d::Point2{0.0, 0.0}, 2.0, 0.5, 24), output_dir, "capsule");
    write_shape(b2d::RoundedRect(b2d::Point2{0.0, 0.0}, 3.0, 2.0, 0.25, 12),
                output_dir,
                "rounded_rect");
    write_shape(b2d::Ring2D(b2d::Point2{0.0, 0.0}, 0.5, 2.0, 64), output_dir, "ring2d");

    return 0;
}
