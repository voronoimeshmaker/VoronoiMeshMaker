# SPDX-License-Identifier: BSD-3-Clause
# vmm/ tools: benchmark. The oracle golden files (tests/data/golden) were
# generated once from the legacy VMMLib and are frozen since its retirement.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmark/benchmark.cpp")
    add_executable(vmm_benchmark benchmark/benchmark.cpp)
    target_link_libraries(vmm_benchmark PRIVATE ${vmm_targets} vmm_settings)
endif()
