# ============================================================================
# P05a - proofs of concept (DEC-027). Standalone project, outside the main
# build: nothing here is added by the root CMakeLists.txt.
# ============================================================================
cmake_minimum_required(VERSION 3.28)
project(P05a LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(CGAL 6.2 REQUIRED)
file(STRINGS "${CGAL_INCLUDE_DIRS}/CGAL/version.h" cgal_nr_line REGEX "^#define CGAL_VERSION_NR [0-9]+")
string(REGEX MATCH "[0-9]+$" P05A_CGAL_VERSION_NR "${cgal_nr_line}")
message(STATUS "P05a CGAL package ${CGAL_VERSION} at ${CGAL_DIR}; headers ${CGAL_INCLUDE_DIRS} (NR ${P05A_CGAL_VERSION_NR})")
message(STATUS "P05a compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}; CMake ${CMAKE_VERSION}")

set(P05A_PUBLIC_HEADERS
    backend.hpp
    csr.hpp
    invariants.hpp
    mesh.hpp
    types.hpp
    voronoi2d.hpp
    voronoi3d.hpp)
foreach(header IN LISTS P05A_PUBLIC_HEADERS)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/p05a/${header}")
        list(REMOVE_ITEM P05A_PUBLIC_HEADERS "${header}")
    endif()
endforeach()

# ---------------------------------------------------------------------------
# Core (no CGAL) and CGAL backend.
# ---------------------------------------------------------------------------
set(p05a_core_sources "")
foreach(source IN ITEMS src/core/voronoi2d.cpp src/core/voronoi3d.cpp)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
        list(APPEND p05a_core_sources "${source}")
    endif()
endforeach()
if(p05a_core_sources)
    add_library(p05a_core STATIC ${p05a_core_sources})
    target_include_directories(p05a_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
else()
    add_library(p05a_core INTERFACE)
    target_include_directories(p05a_core INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()

add_library(p05a_backend_cgal STATIC
    src/backend_cgal/delaunay_pairs.cpp
    src/backend_cgal/region_clip.cpp)
target_link_libraries(p05a_backend_cgal PUBLIC p05a_core PRIVATE CGAL::CGAL)
target_compile_definitions(p05a_backend_cgal PRIVATE P05A_EXPECTED_CGAL_VERSION_NR=${P05A_CGAL_VERSION_NR})

# ---------------------------------------------------------------------------
# Compilation firewall (DEC-007): one translation unit per public header,
# compiled without the CGAL target; each fails if CGAL or Boost was reached.
# ---------------------------------------------------------------------------
function(p05a_firewall_sources out_var include_dir)
    set(sources "")
    foreach(header IN LISTS ARGN)
        get_filename_component(name "${header}" NAME_WE)
        set(source "${CMAKE_CURRENT_BINARY_DIR}/firewall/${name}_check.cpp")
        file(CONFIGURE OUTPUT "${source}" CONTENT
"#include <p05a/${name}.hpp>
#if defined(CGAL_VERSION_NR) || defined(BOOST_VERSION)
#error \"p05a/${name}.hpp reaches CGAL or Boost\"
#endif
")
        list(APPEND sources "${source}")
    endforeach()
    set(${out_var} "${sources}" PARENT_SCOPE)
endfunction()

p05a_firewall_sources(p05a_firewall_srcs include ${P05A_PUBLIC_HEADERS})
add_library(p05a_firewall OBJECT ${p05a_firewall_srcs})
target_include_directories(p05a_firewall PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

p05a_firewall_sources(p05a_negative_srcs firewall_negative contaminated.hpp)
add_library(p05a_firewall_negative OBJECT EXCLUDE_FROM_ALL ${p05a_negative_srcs})
target_include_directories(p05a_firewall_negative PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/firewall_negative")

enable_testing()
set(p05a_std_flag "-std=c++23")
list(TRANSFORM P05A_PUBLIC_HEADERS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/include/p05a/" OUTPUT_VARIABLE p05a_header_paths)
add_test(NAME p05a_firewall_scan
    COMMAND "${CMAKE_COMMAND}" -DCOMPILER=${CMAKE_CXX_COMPILER} -DSTD_FLAG=${p05a_std_flag}
            -DINCLUDE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/include "-DHEADERS=${p05a_header_paths}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ScanIncludes.cmake")
add_test(NAME p05a_firewall_scan_negative_control
    COMMAND "${CMAKE_COMMAND}" -DCOMPILER=${CMAKE_CXX_COMPILER} -DSTD_FLAG=${p05a_std_flag}
            -DINCLUDE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/firewall_negative "-DHEADERS=${CMAKE_CURRENT_SOURCE_DIR}/firewall_negative/p05a/contaminated.hpp"
            -DEXPECT_LEAK=ON -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ScanIncludes.cmake")
add_test(NAME p05a_firewall_compile_negative_control
    COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target p05a_firewall_negative)
set_tests_properties(p05a_firewall_compile_negative_control PROPERTIES WILL_FAIL TRUE)

# ---------------------------------------------------------------------------
# Proof executables. Each returns non-zero if an invariant fails.
# ---------------------------------------------------------------------------
add_executable(p05a_core_only apps/p05a_core_only.cpp)
target_link_libraries(p05a_core_only PRIVATE p05a_core)
add_test(NAME p05a_core_only COMMAND p05a_core_only)

foreach(app IN ITEMS p05a_1_csr p05a_2_multiregion p05a_3_box3d)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/apps/${app}.cpp")
        add_executable(${app} apps/${app}.cpp)
        target_link_libraries(${app} PRIVATE p05a_backend_cgal)
        add_test(NAME ${app} COMMAND ${app})
    endif()
endforeach()
