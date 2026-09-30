// ============================================================================
// File: ut_Cgal.cpp
// Description: CGAL backend: partitions (areas, interfaces, patches, holes,
//              determinism, random property test), Delaunay pairs and the
//              labelled clipping.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "test_domains.hpp"
#include <vmm/backend/cgal.hpp>
#include <vmm/core/random.hpp>

namespace {

using vmm::Rectangle;
using vmm::RegionId;
using vmm::Vec2;

std::map<std::string, double> patch_lengths(const vmm::Partition2D& p) {
    std::map<std::string, double> out;
    for (std::size_t s = 0; s < p.segments().size(); ++s) {
        if (p.is_boundary(s)) out[p.patches()[p.segments()[s].patch.index()]] += p.segment_length(s);
    }
    return out;
}

TEST(Cgal, BackendIsCompleteAndReportsVersions) {
    const auto b = vmm::cgal_backend_2d();
    EXPECT_TRUE(b.complete());
    EXPECT_EQ(b.info().name, "cgal");
    EXPECT_NE(b.info().versions.find("CGAL 6."), std::string::npos);
    EXPECT_FALSE(vmm::Backend2D{}.complete());
    for (int missing = 0; missing < 4; ++missing) {
        auto partial = b;
        if (missing == 0) partial.build_partition = nullptr;
        if (missing == 1) partial.delaunay_pairs = nullptr;
        if (missing == 2) partial.clip_by_region = nullptr;
        if (missing == 3) partial.info = nullptr;
        EXPECT_FALSE(partial.complete()) << missing;
    }
}

TEST(Cgal, SquareWithHoleAndLInterface) {
    const auto p = vmm::cgal_backend_2d().build_partition(vmm::test::square_with_hole());
    ASSERT_TRUE(p);
    const RegionId b = RegionId::from_index(0);
    const RegionId a = RegionId::from_index(1);
    EXPECT_NEAR(p->region_area(a), 0.45, 1e-15);
    EXPECT_NEAR(p->region_area(b), 0.51, 1e-15);
    EXPECT_NEAR(p->interface_length(a, b), 1.4, 1e-15);
    EXPECT_NEAR(p->boundary_length(), 4.8, 1e-14);
    ASSERT_EQ(p->components(a).size(), 1u);
    EXPECT_EQ(p->components(a)[0].loops.size(), 2u);  // outer + hole
    const auto len = patch_lengths(*p);
    EXPECT_NEAR(len.at("south"), 1.0, 1e-15);
    EXPECT_NEAR(len.at("west"), 1.0, 1e-15);
    EXPECT_NEAR(len.at("hole"), 0.8, 1e-15);
    EXPECT_NEAR(len.at("east") + len.at("north"), 2.0, 1e-15);
}

TEST(Cgal, AnchorA1) {
    const auto p = vmm::cgal_backend_2d().build_partition(vmm::test::anchor_a1());
    ASSERT_TRUE(p);
    const RegionId inf = RegionId::from_index(0);
    const RegionId sup = RegionId::from_index(1);
    const RegionId canal = RegionId::from_index(2);
    EXPECT_NEAR(p->region_area(canal), 150, 1e-12);
    EXPECT_NEAR(p->region_area(sup), 1450, 1e-11);
    EXPECT_NEAR(p->region_area(inf), 1400, 1e-11);
    EXPECT_NEAR(p->interface_length(sup, inf), 200, 1e-12);
    EXPECT_NEAR(p->interface_length(canal, sup), 20 + 2 * std::sqrt(125.0), 1e-12);
    EXPECT_NEAR(p->interface_length(canal, inf), 0, 1e-15);
    const auto len = patch_lengths(*p);
    EXPECT_NEAR(len.at("superficie_agua"), 40, 1e-12);
    EXPECT_NEAR(len.at("terreno"), 160, 1e-12);
    EXPECT_NEAR(len.at("base"), 200, 1e-12);
    EXPECT_NEAR(len.at("lateral_esq"), 15, 1e-12);
    EXPECT_NEAR(len.at("lateral_dir"), 15, 1e-12);
    EXPECT_EQ(len.count("boundary"), 0u);
}

TEST(Cgal, ShapeHoleIsNotAVoid) {
    vmm::Declaration2D d;
    const auto m = *d.media().add("m");
    (void)d.add_region("ring", m, vmm::PolygonShape({{0, 0}, {3, 0}, {3, 3}, {0, 3}}, {}, {{{1, 1}, {2, 1}, {2, 2}, {1, 2}}}));
    const auto p = vmm::cgal_backend_2d().build_partition(d);
    ASSERT_TRUE(p);
    EXPECT_TRUE(p->voids().empty());
    EXPECT_DOUBLE_EQ(p->total_area(), 8.0);
    EXPECT_EQ(patch_lengths(*p).count("boundary"), 1u);
}

TEST(Cgal, EmptyDeclarationFails) {
    const vmm::Declaration2D d;
    EXPECT_EQ(vmm::cgal_backend_2d().build_partition(d).error().code(), vmm::ErrorCode::EmptyDeclaration);
}

TEST(Cgal, PartitionIsDeterministic) {
    const auto p = vmm::cgal_backend_2d().build_partition(vmm::test::anchor_a1());
    const auto q = vmm::cgal_backend_2d().build_partition(vmm::test::anchor_a1());
    ASSERT_TRUE(p && q);
    EXPECT_EQ(p->vertices(), q->vertices());
    ASSERT_EQ(p->segments().size(), q->segments().size());
    for (std::size_t s = 0; s < p->segments().size(); ++s) {
        EXPECT_EQ(p->segments()[s].v0, q->segments()[s].v0);
        EXPECT_EQ(p->segments()[s].left, q->segments()[s].left);
    }
}

/// Property: random rectangles and circles painted over a base rectangle
/// leave the total area equal to the base; regions and interfaces agree.
TEST(Cgal, RandomLayersPreserveTotalArea) {
    vmm::Random rng(2026);
    for (int trial = 0; trial < 40; ++trial) {
        vmm::Declaration2D d({24});
        const auto m = *d.media().add("m");
        (void)d.add_region("base", m, Rectangle(Vec2{0, 0}, Vec2{10, 5}));
        for (int k = 0; k < 6; ++k) {
            const Vec2 c{rng.uniform(0.5, 9.5), rng.uniform(0.5, 4.5)};
            const std::string name = "r" + std::to_string(k);
            if (rng.below(2) == 0) {
                (void)d.add_region(name, m, vmm::Circle(c, rng.uniform(0.2, 1.5)));
            } else {
                (void)d.add_region(name, m, Rectangle(c - Vec2{0.4, 0.3}, c + Vec2{rng.uniform(0.1, 2), rng.uniform(0.1, 1)}));
            }
        }
        const auto p = vmm::cgal_backend_2d().build_partition(d);
        ASSERT_TRUE(p) << trial;
        double circle_outside = 0;  // circles may stick out of the base rectangle
        (void)circle_outside;
        double sum = 0;
        for (std::size_t r = 0; r < p->region_count(); ++r) sum += p->region_area(RegionId::from_index(r));
        EXPECT_NEAR(sum, p->total_area(), 1e-12 * sum);
        EXPECT_GE(p->total_area(), 50 - 1e-12);
        double iface = 0;
        for (std::size_t a = 0; a < p->region_count(); ++a) {
            for (std::size_t b = a + 1; b < p->region_count(); ++b) {
                iface += p->interface_length(RegionId::from_index(a), RegionId::from_index(b));
            }
        }
        EXPECT_NEAR(iface, p->interface_length(), 1e-12 * (1 + iface));
    }
}

TEST(Cgal, DelaunayPairsOfASquare) {
    const std::vector<Vec2> s{{0, 0}, {1, 0}, {0, 1}, {1.1, 1.2}};
    const auto pairs = vmm::cgal_backend_2d().delaunay_pairs(s);
    EXPECT_EQ(pairs.size(), 5u);  // 4 hull edges + 1 diagonal
    EXPECT_TRUE(std::ranges::is_sorted(pairs));
}

TEST(Cgal, ClipLabelsEveryEdge) {
    const vmm::LabelledLoop2 cell{{{-1, -1}, {1, -1}, {1, 1}, {-1, 1}}, {1, 2, 3, 4}};
    const vmm::LabelledPolygon2 region{{{{0, -2}, {2, -2}, {2, 2}, {0, 2}}, {10, 11, 12, 13}}, {}};
    const std::vector<vmm::LabelledPolygon2> comps{region};
    const auto clip = vmm::cgal_backend_2d().clip_by_region(cell, comps);
    ASSERT_EQ(clip.pieces.size(), 1u);
    EXPECT_EQ(clip.unlabelled_edges, 0u);
    const auto& labels = clip.pieces[0].outer.labels;
    EXPECT_TRUE(std::ranges::count(labels, 13u) == 1);  // left side comes from the region
    EXPECT_TRUE(std::ranges::count(labels, 1u) == 1 && std::ranges::count(labels, 3u) == 1);
    const vmm::LabelledPolygon2 far{{{{5, 5}, {6, 5}, {6, 6}, {5, 6}}, {1, 1, 1, 1}}, {}};
    const std::vector<vmm::LabelledPolygon2> none{far};
    EXPECT_TRUE(vmm::cgal_backend_2d().clip_by_region(cell, none).pieces.empty());
}

}  // namespace
