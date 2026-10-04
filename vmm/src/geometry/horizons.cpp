// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <utility>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/geometry/horizons.hpp>
namespace vmm {
Result<HorizonGrid> HorizonGrid::make(std::vector<Real> x, std::vector<Real> y,
    std::vector<std::string> names, std::vector<std::vector<Real>> elevations) {
    const auto axis = [](const auto& a) {
        return a.size() >= 2 && std::ranges::all_of(a, [](Real v) { return std::isfinite(v); }) &&
            std::adjacent_find(a.begin(), a.end(), std::greater_equal<Real>{}) == a.end();
    };
    if (!axis(x) || !axis(y) || x.size() > std::numeric_limits<std::size_t>::max()/y.size())
        return fail(ErrorCode::InvalidArgument, "horizon axes must be finite, increasing and have at least two nodes");
    if (names.size() < 2 || names.size() != elevations.size())
        return fail(ErrorCode::InvalidArgument, "at least two named horizons are required");
    std::set<std::string> used;
    for (std::size_t h=0; h<names.size(); ++h) {
        if (names[h].empty() || !used.insert(names[h]).second)
            return fail(ErrorCode::DuplicateName, "empty or repeated horizon name");
        if (elevations[h].size() != x.size()*y.size())
            return fail(ErrorCode::InvalidArgument, "horizon node count");
        for (std::size_t k=0; k<elevations[h].size(); ++k) {
            if (!std::isfinite(elevations[h][k]) || (h && elevations[h][k] < elevations[h-1][k]))
                return fail(ErrorCode::InvalidArgument, "non-finite or crossing horizons");
        }
    }
    HorizonGrid g;
    g.x_=std::move(x); g.y_=std::move(y); g.names_=std::move(names); g.z_=std::move(elevations);
    return g;
}
Result<HorizonGrid> HorizonGrid::sample(std::vector<Real> x, std::vector<Real> y,
    std::vector<std::string> names, std::span<const Surface> surfaces) {
    if (x.size() < 2 || y.size() < 2 || x.size() > std::numeric_limits<std::size_t>::max()/y.size())
        return fail(ErrorCode::InvalidArgument, "horizon axes");
    std::vector<std::vector<Real>> z;
    for (const auto& f: surfaces) {
        if (!f) return fail(ErrorCode::InvalidArgument, "empty horizon function");
        std::vector<Real> row;
        for (Real b:y) for (Real a:x) row.push_back(f(Vec2{a,b}));
        z.push_back(std::move(row));
    }
    return make(std::move(x),std::move(y),std::move(names),std::move(z));
}
} // namespace vmm
