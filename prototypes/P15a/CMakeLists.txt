# ============================================================================
# P15a - 3D clipping proof of concept (sequencia_prompts.md, DEC-003, DEC-010).
# Standalone project, outside the main build; throwaway code. It reuses the
# generic core of vmm/ (Mesh<3>, check_invariants<3>).
#   cmake -S prototypes/P15a -B <build> -G Ninja && cmake --build <build>
# SPDX-License-Identifier: GPL-3.0-or-later
# ============================================================================
cmake_minimum_required(VERSION 3.28)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
project(P15a LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(VMM_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(VMM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(VMM_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(VMM_BUILD_VMM_TOOLS OFF CACHE BOOL "" FORCE)
include("${VMM_ROOT_DIR}/cmake/ConfigDependencies.cmake")
add_subdirectory("${VMM_ROOT_DIR}/vmm" vmm)

add_executable(p15a p15a_main.cpp p15a_build.cpp)
target_compile_options(p15a PRIVATE -Wall -Wextra)
target_link_libraries(p15a PRIVATE vmm_core CGAL::CGAL)
enable_testing()
add_test(NAME p15a_quick COMMAND p15a quick)
