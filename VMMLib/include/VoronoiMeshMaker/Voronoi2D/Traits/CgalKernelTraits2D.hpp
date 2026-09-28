#pragma once
//==============================================================================
// Name        : CgalKernelTraits2D.hpp
// Project     : VoronoiMeshMaker (VMM)
// Module      : Voronoi2D / Traits
// Description : Central CGAL type aliases for the 2D Voronoi pipeline.
// License     : GNU GPL v3
//==============================================================================

/**
 * @file CgalKernelTraits2D.hpp
 * @brief Defines the canonical CGAL kernel and triangulation types for 2D.
 *
 * The rest of the Voronoi2D pipeline should depend on this traits type instead
 * of spelling CGAL types directly. This keeps the code flexible if the kernel or
 * triangulation storage needs to change later.
 */

//==============================================================================
// C++ standard library
//==============================================================================
#include <span>

//==============================================================================
// CGAL
//==============================================================================
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Triangulation_data_structure_2.h>
#include <CGAL/Triangulation_face_base_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>

//==============================================================================
// VoronoiMeshMaker
//==============================================================================
#include <VoronoiMeshMaker/Core/namespace.h>
#include <VoronoiMeshMaker/Sites2D/Site2D.hpp>

VORMAKER_NAMESPACE_OPEN
VORONOI2D_NAMESPACE_OPEN

/**
 * @brief Canonical CGAL traits for unweighted 2D Voronoi generation.
 */
struct CgalKernelTraits2D {
    using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
    using Real = ::vmm::s2d::Real;
    using SiteId = ::vmm::s2d::SiteId;
    using Point2 = ::vmm::s2d::Point2;

    using CgalPoint2 = Kernel::Point_2;
    using CgalSegment2 = Kernel::Segment_2;
    using CgalLine2 = Kernel::Line_2;

    using VertexBase =
        CGAL::Triangulation_vertex_base_with_info_2<SiteId, Kernel>;
    using FaceBase = CGAL::Triangulation_face_base_2<Kernel>;
    using TriangulationDataStructure =
        CGAL::Triangulation_data_structure_2<VertexBase, FaceBase>;
    using DelaunayTriangulation =
        CGAL::Delaunay_triangulation_2<Kernel, TriangulationDataStructure>;

    using VertexHandle = DelaunayTriangulation::Vertex_handle;
    using FaceHandle = DelaunayTriangulation::Face_handle;
    using Edge = DelaunayTriangulation::Edge;

    [[nodiscard]] static bool is_simple_ccw_convex(std::span<const Point2> ring) {
        CGAL::Polygon_2<Kernel> polygon;
        for (const auto point : ring) polygon.push_back(to_cgal(point));
        return polygon.size() >= 3U && polygon.is_simple()
            && polygon.is_counterclockwise_oriented() && polygon.is_convex();
    }

    [[nodiscard]] static CgalPoint2 to_cgal(Point2 point) {
        return CgalPoint2(point.x, point.y);
    }

    [[nodiscard]] static Point2 from_cgal(const CgalPoint2& point) {
        return Point2{CGAL::to_double(point.x()), CGAL::to_double(point.y())};
    }
};

VORONOI2D_NAMESPACE_CLOSE
VORMAKER_NAMESPACE_CLOSE
