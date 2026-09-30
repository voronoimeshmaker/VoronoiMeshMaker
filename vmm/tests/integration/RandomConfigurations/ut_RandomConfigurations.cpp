// ============================================================================
// File: ut_RandomConfigurations.cpp
// Description: P10 acceptance: DEC-011 invariants on at least 1000 random
//              seeded configurations (random domains with 1-4 regions,
//              rectangles, circles and holes, different densities per
//              region, optional interface pairs).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include "mesh_checks.hpp"
#include <vmm/backend/cgal.hpp>
#include <vmm/core/random.hpp>
#include <vmm/sites/sources.hpp>
#include <vmm/voronoi/builder2d.hpp>

namespace {

using vmm::Vec2;

TEST(RandomConfigurations, InvariantsOnAThousandConfigurations) {
    const auto backend = vmm::cgal_backend_2d();
    std::size_t built = 0;
    std::size_t rejected = 0;
    std::size_t with_pairs = 0;
    for (std::uint64_t seed = 1; built < 1000 && seed < 3000; ++seed) {
        vmm::Random rng(seed);
        vmm::Declaration2D d({16});
        const auto m = *d.media().add("m");
        const double w = rng.uniform(1, 4);
        const double h = rng.uniform(1, 4);
        (void)d.add_region("base", m, vmm::Rectangle(Vec2{0, 0}, Vec2{w, h}, {"s", "e", "n", "w"}));
        const auto extra = rng.below(4);
        for (std::uint64_t k = 0; k < extra; ++k) {
            const Vec2 c{rng.uniform(0.2 * w, 0.8 * w), rng.uniform(0.2 * h, 0.8 * h)};
            const double r = rng.uniform(0.1, 0.3) * std::min(w, h);
            const auto kind = rng.below(3);
            if (kind == 0) {
                (void)d.add_region("r" + std::to_string(k), m, vmm::Circle(c, r));
            } else if (kind == 1) {
                (void)d.add_region("r" + std::to_string(k), m, vmm::Rectangle(c - Vec2{r, 0.7 * r}, c + Vec2{r, 0.7 * r}));
            } else {
                (void)d.add_hole(vmm::Circle(c, 0.5 * r));
            }
        }
        const auto p = backend.build_partition(d);
        ASSERT_TRUE(p) << seed;
        const double base_h = 0.08 * std::min(w, h);
        std::vector<vmm::RegionSites> src;
        for (std::size_t r = 0; r < p->region_count(); ++r) {
            const double spacing = base_h * rng.uniform(0.4, 1.0);
            src.push_back(vmm::sites_for(vmm::RegionId::from_index(r), vmm::UniformRandomSource(spacing)));
        }
        vmm::SiteGenerationOptions o;
        o.seed = seed;
        if (rng.below(4) == 0) {
            o.interface_pairs = vmm::InterfacePairs(base_h);
            ++with_pairs;
        }
        const auto sites = vmm::generate_sites(*p, src, o);
        if (!sites) {
            // Valid rejections only: a region too thin or emptied for its spacing.
            const auto code = sites.error().code();
            ASSERT_TRUE(code == vmm::ErrorCode::RegionWithoutSites) << seed << " " << sites.error().message();
            ++rejected;
            continue;
        }
        const auto b = vmm::build_mesh_2d(*p, *sites, backend);
        ASSERT_TRUE(b) << seed << " " << b.error().message();
        const auto rep = vmm::test::expect_invariants(*p, *b, "seed " + std::to_string(seed));
        ASSERT_TRUE(rep.passed([&] {
            auto ref = vmm::invariant_reference(*p);
            ref.cell_measure = b->cell_polygon_area;
            return ref;
        }())) << seed;
        ++built;
    }
    EXPECT_EQ(built, 1000u);
    std::cout << "[ INFO     ] configurations built " << built << ", rejected (region without sites) " << rejected
              << ", with interface pairs " << with_pairs << "\n";
}

}  // namespace
