# ============================================================================
# P18a - proof of concept of conforming 3D interfaces (P15 §12). Standalone
# project, outside the main build; throwaway code built on the vmm/ library.
#   cmake -S prototypes/P18a -B <build> -G Ninja && cmake --build <build>
# SPDX-License-Identifier: GPL-3.0-or-later
# ============================================================================
cmake_minimum_required(VERSION 3.28)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
project(P18a LANGUAGES CXX)

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

add_executable(p18a p18a.cpp)
target_compile_options(p18a PRIVATE -Wall -Wextra -ffp-contract=off)
target_link_libraries(p18a PRIVATE vmm_backend_cgal vmm_core)
enable_testing()
add_test(NAME p18a_quick COMMAND p18a quick)
