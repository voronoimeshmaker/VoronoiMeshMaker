#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Ellipse.hpp>
#include <VoronoiMeshMaker/Boundary2D/Transforms/Boundary2DTransform.hpp>
#include <VoronoiMeshMaker/ClippedVoronoi2DExport.hpp>
#include <VoronoiMeshMaker/Core/constants.h>
#include <VoronoiMeshMaker/Site2DTransform.hpp>
#include <VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiBuilder2D.hpp>
#include <VoronoiMeshMaker/VoronoiGenerationTimer2D.hpp>

namespace {

enum class SitePattern {
    Cartesian,
    Hexagonal,
    Random,
    TriangularII,
    TriangularIV
};

// Edit this block for the paper case you want to generate.
constexpr SitePattern kSitePattern = SitePattern::Hexagonal;
constexpr std::string_view kOutputFileName = "paper_ellipse_hexagonal";
constexpr bool kPrintTimings = true;

constexpr double kEllipseCenterX = 0.0;
constexpr double kEllipseCenterY = 0.0;
constexpr double kEllipseMajorRadius = 2.0;
constexpr double kEllipseMinorRadius = 1.0;
constexpr std::size_t kEllipseSegments = 192;
constexpr double kRotationAngleRadians = ::vmm::constants::kPi / 4.0;

constexpr double kMinimumDistanceToBoundary = 0.035;
constexpr double kMinimumDistanceBetweenSites = 0.04;

constexpr double kCartesianSpacingX = 0.16;
constexpr double kCartesianSpacingY = 0.16;
constexpr double kCartesianEpsilon = 0.0;

constexpr double kTriangularSpacing = 0.16;
constexpr double kTriangularEpsilon = 0.0;

constexpr std::size_t kHexagonalCount = 25;

constexpr std::size_t kRandomSiteCount = 185;
constexpr std::uint32_t kRandomSeed = 20260517U;

struct CaseSummary {
    std::string_view pattern_name{};
    std::size_t site_count{};
    std::size_t internal_count{};
    std::size_t boundary_count{};
    double total_area{};
    std::filesystem::path output_path{};
    ::vmm::vd2d::VoronoiGenerationTimings2D timings{};
};

[[nodiscard]] constexpr std::string_view pattern_name(SitePattern pattern) noexcept {
    switch(pattern) {
        case SitePattern::Cartesian:
            return "Cartesian";
        case SitePattern::Hexagonal:
            return "Hexagonal";
        case SitePattern::Random:
            return "Random";
        case SitePattern::TriangularII:
            return "TriangularII";
        case SitePattern::TriangularIV:
            return "TriangularIV";
    }
    return "Unknown";
}

template <class Pattern>
CaseSummary build_and_write_case(const Pattern& pattern)
{
    namespace b2d = ::vmm::b2d;
    namespace s2d = ::vmm::s2d;
    namespace vd2d = ::vmm::vd2d;
    namespace io = ::vmm::io;

    const b2d::Point2 center{kEllipseCenterX, kEllipseCenterY};

    auto generation = vd2d::VoronoiGenerationTimer2D::measure(
        [&] {
            return b2d::make_boundary(
                b2d::Ellipse(center,
                             kEllipseMajorRadius,
                             kEllipseMinorRadius,
                             kEllipseSegments),
                b2d::PolygonizePolicy{},
                b2d::RegionId{1});
        },
        [&](const b2d::Boundary2DData& boundary) {
            const s2d::SiteValidationOptions validation{
                .min_distance_to_boundary = kMinimumDistanceToBoundary,
                .min_distance_between_sites = kMinimumDistanceBetweenSites,
                .require_sequential_ids = true
            };

            return s2d::make_sites(
                boundary,
                pattern,
                validation,
                s2d::RegionId{1});
        },
        [&](b2d::Boundary2DData& boundary, s2d::SiteSet& sites) {
            const auto rotation =
                b2d::rotation_transform(kRotationAngleRadians, center);
            b2d::transform_in_place(boundary, rotation);
            s2d::transform_in_place(sites, rotation);

            return vd2d::ClippedVoronoiBuilder2D::build(sites, boundary);
        });

    io::VtkOptions vtk_options{};
    vtk_options.precision = 12;
    vtk_options.cell_data = true;
    vtk_options.write_delaunay_edges = false;

    const auto output_path = io::write_clipped_voronoi_vtk_legacy(
        generation.diagram,
        std::string(VMM_PAPER_CASE_SOURCE_DIR) + "/vtk_output",
        std::string(kOutputFileName),
        vtk_options);

    return CaseSummary{
        pattern_name(kSitePattern),
        generation.diagram.sites.size(),
        generation.diagram.internal_volume_count(),
        generation.diagram.boundary_volume_count(),
        static_cast<double>(generation.diagram.total_volume_area()),
        output_path,
        generation.timings
    };
}

[[nodiscard]] CaseSummary run_selected_case() {
    namespace s2d = ::vmm::s2d;

    switch(kSitePattern) {
        case SitePattern::Cartesian:
            return build_and_write_case(
                s2d::CartesianGrid2D{kCartesianSpacingX, kCartesianSpacingY}
                    .with_epsilon(kCartesianEpsilon)
                    .centered_in_box());

        case SitePattern::Hexagonal:
            return build_and_write_case(
                s2d::HexagonalGrid2D{kHexagonalCount});

        case SitePattern::Random:
            return build_and_write_case(
                s2d::UniformRandom2D{kRandomSiteCount, kRandomSeed});

        case SitePattern::TriangularII:
            return build_and_write_case(
                s2d::TriangularIIGrid2D{kTriangularSpacing, kTriangularEpsilon});

        case SitePattern::TriangularIV:
            return build_and_write_case(
                s2d::TriangularIVGrid2D{kTriangularSpacing, kTriangularEpsilon});
    }

    return {};
}

void print_summary(const CaseSummary& summary) {
    std::cout << "Paper ellipse mesh case\n"
              << "  pattern: " << summary.pattern_name << '\n'
              << "  sites: " << summary.site_count << '\n'
              << "  internal volumes: " << summary.internal_count << '\n'
              << "  boundary volumes: " << summary.boundary_count << '\n'
              << "  total area: " << summary.total_area << '\n'
              << "  vtk: " << summary.output_path << '\n';
}

void print_timings(const ::vmm::vd2d::VoronoiGenerationTimings2D& timings) {
    std::cout << "Generation timings (ms)\n"
              << "  boundary2d: " << timings.boundary2d_ms << '\n'
              << "  sites2d: " << timings.sites2d_ms << '\n'
              << "  voronoi2d: " << timings.voronoi2d_ms << '\n';
}

} // namespace

int main() {
    const auto summary = run_selected_case();
    print_summary(summary);
    if constexpr(kPrintTimings) {
        print_timings(summary.timings);
    }
    return 0;
}
