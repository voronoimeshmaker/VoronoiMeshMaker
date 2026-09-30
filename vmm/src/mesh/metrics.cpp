// ============================================================================
// File: metrics.cpp
// Description: Finite-volume metrics and quality report.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/mesh/metrics.hpp>

namespace vmm {
namespace {

Real percentile99(std::vector<Real> v) {
    if (v.empty()) return 0;
    const auto k = static_cast<std::size_t>(std::ceil(0.99 * static_cast<Real>(v.size()))) - 1;
    std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(k), v.end());
    return v[k];
}

}  // namespace

template <std::size_t D>
Metrics<D> compute_metrics(const Mesh<D>& m) {
    Metrics<D> r;
    const std::size_t nf = m.face_count();
    const std::size_t nc = m.cell_count();
    r.face_area_vector.resize(nf);
    r.face_centroid.resize(nf);
    r.face_measure.resize(nf);
    r.distance.resize(nf);
    r.intersection.resize(nf);
    r.nonorthogonality.resize(nf);
    r.skewness.resize(nf);
    r.cell_measure.assign(nc, 0);
    r.cell_centroid.assign(nc, Vec<D>{});
    std::vector<Vec<D>> moment(nc, Vec<D>{});
    constexpr Real dim = static_cast<Real>(D);

    for (const FaceId f : m.faces()) {
        const auto g = face_geometry(m, f);
        const std::size_t i = f.index();
        r.face_area_vector[i] = g.area_vector;
        r.face_centroid[i] = g.centroid;
        r.face_measure[i] = norm(g.area_vector);
        const Vec<D> n = r.face_measure[i] > 0 ? (1.0 / r.face_measure[i]) * g.area_vector : Vec<D>{};
        const CellId o = m.owner(f);
        const Vec<D>& xo = m.site(o);
        // Pyramid (triangle in 2D) from the owner's generator to the face.
        auto accumulate = [&](CellId c, Real sign) {
            const Vec<D>& x = m.site(c);
            const Real v = sign * dot(g.centroid - x, g.area_vector) / dim;
            r.cell_measure[c.index()] += v;
            moment[c.index()] = moment[c.index()] + v * (x + (dim / (dim + 1)) * (g.centroid - x));
        };
        accumulate(o, 1);
        if (m.is_internal(f)) {
            const CellId nb = m.neighbour(f);
            accumulate(nb, -1);
            const Vec<D> d = m.site(nb) - xo;
            r.distance[i] = norm(d);
            const Real dn = dot(d, n);
            const Real t = dn != 0 ? dot(g.centroid - xo, n) / dn : 0.5;
            r.intersection[i] = xo + t * d;
            r.nonorthogonality[i] = angle_between(g.area_vector, d);
        } else {
            const Real h = dot(g.centroid - xo, n);
            r.distance[i] = std::abs(h);
            r.intersection[i] = xo + h * n;
            r.nonorthogonality[i] = angle_between(g.area_vector, g.centroid - xo);
        }
        r.skewness[i] = r.distance[i] > 0 ? norm(g.centroid - r.intersection[i]) / r.distance[i] : 0;
    }
    for (std::size_t c = 0; c < nc; ++c) {
        r.cell_centroid[c] = r.cell_measure[c] != 0 ? (1.0 / r.cell_measure[c]) * moment[c] : m.sites()[c];
    }
    // Aspect ratio: farthest vertex over nearest face line/plane, from the centroid.
    std::vector<Real> far(nc, 0);
    std::vector<Real> near(nc, std::numeric_limits<Real>::infinity());
    for (const FaceId f : m.faces()) {
        const std::size_t i = f.index();
        const Vec<D> n = r.face_measure[i] > 0 ? (1.0 / r.face_measure[i]) * r.face_area_vector[i] : Vec<D>{};
        for (const CellId c : {m.owner(f), m.neighbour(f)}) {
            if (!c.valid()) continue;
            const Vec<D>& x = r.cell_centroid[c.index()];
            near[c.index()] = std::min(near[c.index()], std::abs(dot(r.face_centroid[i] - x, n)));
            for (const VertexId v : m.face_vertices(f)) far[c.index()] = std::max(far[c.index()], norm(m.point(v) - x));
        }
    }
    r.cell_aspect_ratio.resize(nc);
    for (std::size_t c = 0; c < nc; ++c) r.cell_aspect_ratio[c] = near[c] > 0 ? far[c] / near[c] : std::numeric_limits<Real>::infinity();
    return r;
}

template <std::size_t D>
QualityReport quality_report(const Mesh<D>& m, const Metrics<D>& x, const QualityLimits& limits) {
    QualityReport q;
    const std::size_t nr = m.regions().size();
    q.regions.resize(nr);
    std::vector<std::vector<Real>> nonortho(nr);
    std::vector<std::vector<Real>> skew(nr);
    std::vector<std::vector<Real>> aspect(nr);
    for (auto& r : q.regions) r.min_face_fraction = std::numeric_limits<Real>::infinity();
    for (const CellId c : m.cells()) {
        auto& r = q.regions[m.region(c).index()];
        ++r.cells;
        r.max_aspect_ratio = std::max(r.max_aspect_ratio, x.cell_aspect_ratio[c.index()]);
        aspect[m.region(c).index()].push_back(x.cell_aspect_ratio[c.index()]);
    }
    constexpr Real exponent = static_cast<Real>(D - 1) / static_cast<Real>(D);
    for (const FaceId f : m.faces()) {
        const std::size_t i = f.index();
        const CellId o = m.owner(f);
        const std::size_t ro = m.region(o).index();
        Real scale = std::pow(x.cell_measure[o.index()], exponent);
        if (m.is_internal(f)) {
            const CellId n = m.neighbour(f);
            const std::size_t rn = m.region(n).index();
            scale = std::min(scale, std::pow(x.cell_measure[n.index()], exponent));
            const Real a = x.nonorthogonality[i];
            if (ro == rn) {
                q.regions[ro].max_nonortho_internal = std::max(q.regions[ro].max_nonortho_internal, a);
                nonortho[ro].push_back(a);
                if (a > limits.max_nonorthogonality) ++q.nonortho_violations;
            } else {
                for (const std::size_t r : {ro, rn}) q.regions[r].max_nonortho_interface = std::max(q.regions[r].max_nonortho_interface, a);
            }
            for (const std::size_t r : {ro, rn}) {
                q.regions[r].max_skewness = std::max(q.regions[r].max_skewness, x.skewness[i]);
                skew[r].push_back(x.skewness[i]);
            }
            if (x.skewness[i] > limits.max_skewness && ro == rn) ++q.skewness_violations;
        }
        const Real fraction = scale > 0 ? x.face_measure[i] / scale : 0;
        q.regions[ro].min_face_fraction = std::min(q.regions[ro].min_face_fraction, fraction);
        if (fraction < limits.min_face_fraction) ++q.short_faces;
    }
    for (std::size_t r = 0; r < nr; ++r) {
        q.regions[r].p99_nonortho_internal = percentile99(nonortho[r]);
        q.regions[r].p99_skewness = percentile99(skew[r]);
        q.regions[r].p99_aspect_ratio = percentile99(aspect[r]);
        if (q.regions[r].cells == 0) q.regions[r].min_face_fraction = 0;
    }
    return q;
}

template Metrics<2> compute_metrics(const Mesh<2>&);
template Metrics<3> compute_metrics(const Mesh<3>&);
template QualityReport quality_report(const Mesh<2>&, const Metrics<2>&, const QualityLimits&);
template QualityReport quality_report(const Mesh<3>&, const Metrics<3>&, const QualityLimits&);

}  // namespace vmm
