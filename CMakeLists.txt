cmake_minimum_required(VERSION 3.25)

cmake_policy(SET CMP0135 NEW)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

project(VoronoiMeshMaker
    VERSION 1.0
    DESCRIPTION "Two-dimensional Voronoi mesh generation library based on CGAL"
    LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# Project configuration modules
# -----------------------------------------------------------------------------

include(cmake/ConfigUtils.cmake)          # Guard rails and CMake helpers
include(cmake/ConfigOptions.cmake)        # Project options and layout
include(cmake/ConfigCompiler.cmake)       # Compiler settings and shared flags
include(cmake/ConfigDependencies.cmake)   # External dependencies
include(cmake/ConfigTargets.cmake)        # Main library target
include(cmake/ConfigExamples.cmake)       # Optional examples
include(cmake/ConfigPaper.cmake)          # Optional paper/research programs
include(cmake/ConfigTests.cmake)          # Optional tests
include(cmake/ConfigDocs.cmake)           # Optional documentation

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------

message(STATUS "VoronoiMeshMaker configuration complete")
message(STATUS "Build type          : ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard        : ${CMAKE_CXX_STANDARD}")
message(STATUS "Build shared library: ${VMM_BUILD_SHARED_LIBS}")
message(STATUS "Build examples      : ${VMM_BUILD_EXAMPLES}")
message(STATUS "Build paper programs: ${VMM_BUILD_PAPER}")
message(STATUS "Build tests         : ${VMM_BUILD_TESTS}")
message(STATUS "Build docs          : ${VMM_BUILD_DOCS}")
message(STATUS "Warnings            : ${VMM_ENABLE_WARNINGS}")
message(STATUS "Native architecture : ${VMM_ENABLE_NATIVE_ARCH}")
message(STATUS "LTO/IPO             : ${VMM_ENABLE_LTO}")
message(STATUS "Fast math           : ${VMM_ENABLE_FAST_MATH}")
message(STATUS "Install prefix      : ${CMAKE_INSTALL_PREFIX}")
