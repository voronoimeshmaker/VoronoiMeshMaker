// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/mesh/layered.hpp>
#include <vmm/mesh/metrics.hpp>
namespace vmm {
std::vector<ColumnQuality> column_quality_report(const LayeredMesh& layered) {
    const auto& data=layered.data(); const auto& mesh=layered.mesh();
    std::vector<std::size_t> interval;
    std::vector<Real> width;
    for(std::size_t h=0;h<data.fractions.size();++h)
        for(std::size_t k=1;k<data.fractions[h].size();++k) {
            interval.push_back(h); width.push_back(data.fractions[h][k]-data.fractions[h][k-1]);
        }
    std::vector<ColumnQuality> result(data.column_count);
    for(std::size_t c=0;c<data.column_count;++c) {
        auto& q=result[c]; const auto cells=layered.cells_in_column_view(c);
        q.absent_layers=interval.size()-cells.size();
        q.history_available=!data.omitted_count.empty();
        if(q.history_available) {
            q.omitted=data.omitted_count[c];q.merged=data.merged_count[c];q.bottom_adjusted=data.bottom_adjusted_count[c];
        }
        if(!cells.empty()) q.min_thickness=std::numeric_limits<Real>::infinity();
        if(!data.column_levels.empty()) for(auto cell:cells) {
            const auto l=data.layer[cell.index()];
            q.min_thickness=std::min(q.min_thickness,data.column_levels[c][l+1]-data.column_levels[c][l]);
        }
    }
    const auto metrics=compute_metrics(mesh);
    for(auto face:mesh.faces()) for(auto cell:{mesh.owner(face),mesh.neighbour(face)}) if(cell.valid()) {
        auto& q=result[data.column[cell.index()]];
        if(data.face_level[face.index()]==lateral_face)
            q.max_vertical_skew=std::max(q.max_vertical_skew,metrics.vertical_skew[face.index()]);
        else {
            const auto normal=metrics.face_area_vector[face.index()];
            q.max_interface_slope=std::max(q.max_interface_slope,std::atan2(std::hypot(normal[0],normal[1]),std::abs(normal[2])));
        }
        if(data.column_levels.empty()) {
            const auto l=data.layer[cell.index()],h=interval[l];
            for(auto vertex:mesh.face_vertices(face)) {
                const auto p=mesh.point(vertex);
                const auto lower=data.horizons.elevation(h,{p[0],p[1]});
                const auto upper=data.horizons.elevation(h+1,{p[0],p[1]});
                // LayeredMesh validation guarantees all these vertices are in the grid.
                if(lower && upper) q.min_thickness=std::min(q.min_thickness,(*upper-*lower)*width[l]);
            }
        }
    }
    return result;
}
} // namespace vmm
