// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <cmath>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/layered.hpp>
namespace vmm {
Result<void> merge_thin_layers(std::vector<Real>& levels,std::span<const std::size_t> intervals,Real minimum) {
    if(levels.size()!=intervals.size()+1 || levels.empty() || !std::isfinite(minimum) || minimum<0 ||
       !std::ranges::all_of(levels,[](Real z){return std::isfinite(z);}) || !std::is_sorted(levels.begin(),levels.end()))
        return fail(ErrorCode::InvalidArgument,"invalid thin-layer merge input");
    auto result=levels;
    for(std::size_t l=0;l<intervals.size();++l) {
        if(result[l+1]==result[l] || result[l+1]-result[l]>=minimum) continue;
        std::size_t above=l+1;
        while(above<intervals.size() && intervals[above]==intervals[l] && result[above+1]==result[above]) ++above;
        if(above<intervals.size() && intervals[above]==intervals[l]) {
            for(std::size_t k=l+1;k<=above;++k) result[k]=result[l];
            continue;
        }
        std::size_t below=l;
        while(below>0 && intervals[below-1]==intervals[l] && result[below]==result[below-1]) --below;
        if(below==0 || intervals[below-1]!=intervals[l])
            return fail(ErrorCode::InvalidArgument,"thin layer has no neighbour in the same horizon interval");
        for(std::size_t k=below;k<=l;++k) result[k]=result[l+1];
    }
    levels=std::move(result);
    return {};
}
} // namespace vmm
