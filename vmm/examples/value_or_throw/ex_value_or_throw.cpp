// ============================================================================
// File: ex_value_or_throw.cpp
// Title: Errors - exceptions instead of checking every Result
// Description: The quick start written with vmm::value_or_throw: each call
//              gives its value or throws vmm::Exception, so one try/catch
//              replaces the checks. A deliberate mistake (a circle of
//              negative radius) shows the message the user gets.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <print>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/error/exception.hpp>
#include <vmm/io/native.hpp>
#include <vmm/io/vtu.hpp>
#include <vmm/vmm.hpp>

using vmm::value_or_throw;

int main() {
    vmm::set_language(vmm::Language::English);
    try {
        vmm::MeshRequest2D request;
        auto& d = request.declaration;
        const auto rock = value_or_throw(d.media().add("rock"));
        const auto outer = value_or_throw(d.add_region("outer", rock, vmm::Rectangle({0, 0}, {1, 1})));
        const auto inner = value_or_throw(d.add_region("inner", rock, vmm::Rectangle({0, 0}, {0.7, 0.7})));
        value_or_throw(d.add_hole(vmm::Rectangle({0.4, 0.4}, {0.6, 0.6}, {"hole", "hole", "hole", "hole"})));
        request.sources = {vmm::sites_for(outer, vmm::UniformRandomSource(0.05)),
                           vmm::sites_for(inner, vmm::UniformRandomSource(0.03))};
        request.sites.seed = 42;

        const auto result = value_or_throw(vmm::generate_mesh_2d(request));
        std::println("cells {} | internal faces {} | boundary faces {}", result.mesh.cell_count(),
                     result.mesh.internal_face_count(), result.mesh.boundary_faces().size());
        value_or_throw(vmm::write_vtu(result.mesh, "value_or_throw.vtu"));
        value_or_throw(vmm::write_native(result.mesh, "value_or_throw.vmesh"));
        std::println("wrote value_or_throw.vtu and value_or_throw.vmesh");

        // A mistake: the exception carries the same error a Result would.
        value_or_throw(d.add_hole(vmm::Circle({0.2, 0.8}, -0.1)));
        std::println("not reached");
    } catch (const vmm::Exception& e) {
        std::println("caught: {}", e.what());
        std::println("error code {}", static_cast<int>(e.error().code()));
    }
    return 0;
}
