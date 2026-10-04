cmake_minimum_required(VERSION 3.28)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

project(VoronoiMeshMaker
    VERSION 1.1.0
    DESCRIPTION "Voronoi finite-volume mesh generation library"
    LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# Project configuration modules
# -----------------------------------------------------------------------------

include(cmake/ConfigUtils.cmake)          # Guard rails (no in-source builds)
include(cmake/ConfigOptions.cmake)        # Project options and layout
include(cmake/ConfigDependencies.cmake)   # CGAL package and header check
enable_testing()                          # ctest from the build root
add_subdirectory(vmm)                     # The library (P06 architecture)

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------

message(STATUS "VoronoiMeshMaker configuration complete")
message(STATUS "Build type          : ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard        : ${CMAKE_CXX_STANDARD}")
message(STATUS "Build examples      : ${VMM_BUILD_EXAMPLES}")
message(STATUS "Build tests         : ${VMM_BUILD_TESTS}")
message(STATUS "Install prefix      : ${CMAKE_INSTALL_PREFIX}")
