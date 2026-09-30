// ============================================================================
// File: validator.cpp
// Description: Partition validation.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <format>
#include <utility>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/domain/validator.hpp>

namespace vmm {

void ValidationReport::add(Error e) {
    (e.severity() == Severity::Warning || e.severity() == Severity::Info ? warnings_ : errors_).push_back(std::move(e));
}

ValidationReport validate_partition(const Partition2D& p, const ValidationOptions& options) {
    ValidationReport report;
    const Real h = options.local_spacing > 0 ? options.local_spacing : 1e-3 * p.length_scale();

    for (const auto& v : p.voids()) {
        report.add(Error(ErrorCode::DomainVoid, std::format("area {:.6g}", v.area())));
    }
    for (std::size_t r = 0; r < p.region_count(); ++r) {
        const RegionId id = RegionId::from_index(r);
        const auto& name = p.regions()[r].name;
        const auto& comps = p.components(id);
        if (comps.empty()) {
            report.add(Error(ErrorCode::RegionEmptied, name, id));
            continue;
        }
        if (comps.size() > 1) {
            report.add(Error(ErrorCode::RegionFragmented, std::format("{}: {} components", name, comps.size()), id,
                             Severity::Warning));
        }
        for (std::size_t c = 0; c < comps.size(); ++c) {
            const auto& comp = comps[c];
            for (std::size_t l = 0; l < comp.loops.size(); ++l) {
                const auto& loop = comp.loops[l];
                bool closed = loop.size() >= 3;
                for (std::size_t k = 0; closed && k < loop.size(); ++k) {
                    closed = p.end(loop[k]) == p.start(loop[(k + 1) % loop.size()]);
                }
                const Real a = signed_area(p.loop_points(loop));
                if (!closed || (l == 0 ? a <= 0 : a >= 0)) {
                    report.add(Error(ErrorCode::InvalidPolygon,
                                     std::format("{}: component {} loop {} open or wrongly oriented", name, c, l), id));
                }
            }
            const auto poly = p.component_polygon(id, c);
            const Real thickness = 2 * poly.area() / poly.perimeter();
            if (thickness < options.sliver_fraction * h) {
                report.add(Error(ErrorCode::Sliver,
                                 std::format("{}: component {} thickness {:.3g} < {:.3g}", name, c, thickness,
                                             options.sliver_fraction * h),
                                 id, options.slivers_are_errors ? Severity::Error : Severity::Warning));
            }
        }
    }
    return report;
}

}  // namespace vmm
