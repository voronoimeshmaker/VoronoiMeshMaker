cmake_minimum_required(VERSION 3.25)

cmake_policy(SET CMP0135 NEW)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

project(VoronoiMeshMaker
    VERSION 1.0
    DESCRIPTION "Voronoi finite-volume mesh generation library"
    LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# Project configuration modules
# -----------------------------------------------------------------------------

include(cmake/ConfigUtils.cmake)          # Guard rails and CMake helpers
include(cmake/ConfigOptions.cmake)        # Project options and layout
include(cmake/ConfigCompiler.cmake)       # Compiler settings and shared flags
include(cmake/ConfigDependencies.cmake)   # External dependencies
if(VMM_BUILD_VMMLIB)
    include(cmake/ConfigTargets.cmake)    # Legacy VMMLib (oracle, DEC-011)
    include(cmake/ConfigExamples.cmake)   # Legacy examples
    include(cmake/ConfigPaper.cmake)      # Legacy paper/research programs
    include(cmake/ConfigTests.cmake)      # Legacy tests
endif()
enable_testing()                          # ctest from the build root
add_subdirectory(vmm)                     # New library (P06 architecture)
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
message(STATUS "ASan/UBSan          : ${VMM_TEST_SANITIZERS}")
if(VMM_TEST_SANITIZERS)
    message(STATUS "LTO/IPO             : OFF (diagnostic build)")
else()
    message(STATUS "LTO/IPO             : ${VMM_ENABLE_LTO}")
endif()
message(STATUS "Legacy VMMLib       : ${VMM_BUILD_VMMLIB}")
message(STATUS "Install prefix      : ${CMAKE_INSTALL_PREFIX}")
