// ============================================================================
// File: test_domains.hpp
// Description: Test helpers: domain declarations used by several tests
//              (square with hole and L interface from P05a; anchor A1).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <string>
#include <utility>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/declaration.hpp>
#include <vmm/domain/shapes.hpp>

namespace vmm::test {

/// Unit square scaled by s; region "a" = [0, 0.7]^2 painted over "b", hole [0.4, 0.6]^2.
inline Declaration2D square_with_hole(Real s = 1) {
    Declaration2D d;
    const MediumId m = *d.media().add("solid");
    (void)d.add_region("b", m, Rectangle(Vec2{0, 0}, Vec2{s, s}, {"south", "east", "north", "west"}));
    (void)d.add_region("a", m, Rectangle(Vec2{0, 0}, Vec2{0.7 * s, 0.7 * s}, {"south", "", "", "west"}));
    (void)d.add_hole(Rectangle(Vec2{0.4 * s, 0.4 * s}, Vec2{0.6 * s, 0.6 * s}, {"hole", "hole", "hole", "hole"}));
    return d;
}

/// Anchor A1 (P04 §2.1): 200 m x 15 m cross-section, trapezoidal channel
/// (top 40 m, depth 5 m, 1V:2H), soil contact at z = -8 m.
inline Declaration2D anchor_a1() {
    Declaration2D d;
    const MediumId water = *d.media().add("water");
    const MediumId solid = *d.media().add("solid");
    (void)d.add_region("solo_inf", solid,
                       Rectangle(Vec2{-100, -15}, Vec2{100, 0}, {"base", "lateral_dir", "terreno", "lateral_esq"}));
    (void)d.add_region("solo_sup", solid,
                       Rectangle(Vec2{-100, -8}, Vec2{100, 0}, {"", "lateral_dir", "terreno", "lateral_esq"}));
    (void)d.add_region("canal", water,
                       PolygonShape({{-20, 0}, {-10, -5}, {10, -5}, {20, 0}}, {"", "", "", "superficie_agua"}));
    return d;
}

}  // namespace vmm::test
