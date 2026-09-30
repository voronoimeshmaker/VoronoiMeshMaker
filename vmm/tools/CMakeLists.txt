# SPDX-License-Identifier: BSD-3-Clause
# vmm/ tools: golden generator (needs the legacy VMMLib) and benchmark.
if(TARGET VoronoiMeshMaker)
    add_executable(vmm_golden_generator golden/generate_golden.cpp)
    target_link_libraries(vmm_golden_generator PRIVATE VoronoiMeshMaker)
    target_compile_definitions(vmm_golden_generator PRIVATE
        "VMM_GOLDEN_FLAGS=\"${CMAKE_BUILD_TYPE} ${CMAKE_CXX_FLAGS}\"")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmark/benchmark.cpp")
    add_executable(vmm_benchmark benchmark/benchmark.cpp)
    target_link_libraries(vmm_benchmark PRIVATE ${vmm_targets} vmm_settings)
endif()
