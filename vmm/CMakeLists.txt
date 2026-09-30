# ============================================================================
# vmm/ - VoronoiMeshMaker library (architecture P06).
# Targets: vmm_core (no CGAL), vmm_backend_cgal, vmm_io, vmm (facade).
# SPDX-License-Identifier: BSD-3-Clause
# ============================================================================
cmake_minimum_required(VERSION 3.28)

option(VMM_WERROR "Treat warnings as errors in vmm/ targets (R24)" ON)
option(VMM_COVERAGE "Instrument vmm/ targets and tests for coverage (gcovr)" OFF)
option(VMM_SANITIZE "Build vmm/ targets and tests with ASan/UBSan" OFF)
option(VMM_BUILD_VMM_TESTS "Build the vmm/ unit and integration tests" ${VMM_BUILD_TESTS})
option(VMM_BUILD_VMM_EXAMPLES "Build the vmm/ gallery examples" ${VMM_BUILD_EXAMPLES})
option(VMM_BUILD_VMM_TOOLS "Build the vmm/ tools (benchmark, golden generator)" ON)

set(VMM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# ---------------------------------------------------------------------------
# Dependencies (R21, R22): minimum versions declared, versions used printed.
# ---------------------------------------------------------------------------
find_package(CGAL 6.2 REQUIRED)
find_package(Boost 1.83 REQUIRED)
file(STRINGS "${CGAL_INCLUDE_DIRS}/CGAL/version.h" _vmm_cgal_nr REGEX "^#define CGAL_VERSION_NR [0-9]+")
string(REGEX MATCH "[0-9]+$" VMM_CGAL_VERSION_NR "${_vmm_cgal_nr}")
message(STATUS "vmm: compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}; CMake ${CMAKE_VERSION}")
message(STATUS "vmm: CGAL ${CGAL_VERSION} (${CGAL_DIR}, NR ${VMM_CGAL_VERSION_NR}); Boost ${Boost_VERSION}")

# ---------------------------------------------------------------------------
# Common settings for every vmm/ target.
# ---------------------------------------------------------------------------
add_library(vmm_settings INTERFACE)
target_compile_features(vmm_settings INTERFACE cxx_std_23)
target_compile_options(vmm_settings INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion>
    $<$<AND:$<BOOL:${VMM_WERROR}>,$<CXX_COMPILER_ID:GNU,Clang>>:-Werror>)
if(VMM_COVERAGE)
    target_compile_options(vmm_settings INTERFACE --coverage -O0 -g)
    target_link_options(vmm_settings INTERFACE --coverage)
endif()
if(VMM_SANITIZE)
    target_compile_options(vmm_settings INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer
                                                  -fno-sanitize-recover=all)
    target_link_options(vmm_settings INTERFACE -fsanitize=address,undefined)
endif()

function(vmm_module_sources out_var)
    set(files "")
    foreach(module IN LISTS ARGN)
        file(GLOB_RECURSE module_files CONFIGURE_DEPENDS "${VMM_ROOT}/src/${module}/*.cpp")
        list(APPEND files ${module_files})
    endforeach()
    set(${out_var} "${files}" PARENT_SCOPE)
endfunction()

function(vmm_add_library name alias)
    add_library(${name} STATIC ${ARGN})
    add_library(VoronoiMeshMaker::${alias} ALIAS ${name})
    set_target_properties(${name} PROPERTIES POSITION_INDEPENDENT_CODE ON EXPORT_NAME ${alias})
    target_include_directories(${name} PUBLIC
        $<BUILD_INTERFACE:${VMM_ROOT}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_compile_features(${name} PUBLIC cxx_std_23)
    target_link_libraries(${name} PRIVATE $<BUILD_INTERFACE:vmm_settings>)
endfunction()

# ---------------------------------------------------------------------------
# Libraries.
# ---------------------------------------------------------------------------
vmm_module_sources(vmm_core_sources core error geometry domain sites voronoi mesh reorder)
vmm_add_library(vmm_core vmm_core ${vmm_core_sources})

set(vmm_targets vmm_core)
vmm_module_sources(vmm_backend_sources backend/cgal)
if(vmm_backend_sources)
    vmm_add_library(vmm_backend_cgal vmm_backend_cgal ${vmm_backend_sources})
    target_link_libraries(vmm_backend_cgal PUBLIC vmm_core PRIVATE CGAL::CGAL)
    target_compile_definitions(vmm_backend_cgal PRIVATE VMM_EXPECTED_CGAL_VERSION_NR=${VMM_CGAL_VERSION_NR})
    list(APPEND vmm_targets vmm_backend_cgal)
endif()
vmm_module_sources(vmm_io_sources io)
if(vmm_io_sources)
    vmm_add_library(vmm_io vmm_io ${vmm_io_sources})
    target_link_libraries(vmm_io PUBLIC vmm_core)
    list(APPEND vmm_targets vmm_io)
endif()
vmm_module_sources(vmm_facade_sources facade)
if(vmm_facade_sources)
    vmm_add_library(vmm_facade vmm ${vmm_facade_sources})
    target_link_libraries(vmm_facade PUBLIC vmm_core vmm_backend_cgal vmm_io)
    list(APPEND vmm_targets vmm_facade)
endif()

# ---------------------------------------------------------------------------
# Compilation firewall (DEC-007, R19): every public header compiles without
# the CGAL target and preprocesses without opening any CGAL/Boost/GMP header.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE vmm_public_headers CONFIGURE_DEPENDS "${VMM_ROOT}/include/vmm/*.hpp")
set(vmm_firewall_sources "")
foreach(header IN LISTS vmm_public_headers)
    file(RELATIVE_PATH rel "${VMM_ROOT}/include" "${header}")
    string(MAKE_C_IDENTIFIER "${rel}" id)
    set(source "${CMAKE_CURRENT_BINARY_DIR}/firewall/${id}.cpp")
    file(CONFIGURE OUTPUT "${source}" CONTENT
"#include <${rel}>
#if defined(CGAL_VERSION_NR) || defined(BOOST_VERSION) || defined(__GMP_H__)
#error \"${rel} reaches CGAL, Boost or GMP\"
#endif
")
    list(APPEND vmm_firewall_sources "${source}")
endforeach()
add_library(vmm_firewall OBJECT ${vmm_firewall_sources})
target_include_directories(vmm_firewall PRIVATE "${VMM_ROOT}/include")
target_link_libraries(vmm_firewall PRIVATE vmm_settings)

# ---------------------------------------------------------------------------
# Tests (R1, R2): vmm/tests/<module>/<Class>/ut_<Class>.cpp, one executable
# per file, discovered automatically.
# ---------------------------------------------------------------------------
if(VMM_BUILD_VMM_TESTS)
    find_package(GTest 1.14 REQUIRED)
    include(GoogleTest)
    enable_testing()
    add_library(vmm_test_support INTERFACE)
    target_include_directories(vmm_test_support INTERFACE "${VMM_ROOT}/tests/support" "${VMM_ROOT}/tools")
    target_link_libraries(vmm_test_support INTERFACE ${vmm_targets} GTest::gtest GTest::gtest_main vmm_settings)
    target_compile_definitions(vmm_test_support INTERFACE VMM_TEST_DATA_DIR="${VMM_ROOT}/tests/data")
    file(GLOB_RECURSE vmm_unit_tests CONFIGURE_DEPENDS "${VMM_ROOT}/tests/ut_*.cpp")
    foreach(test_source IN LISTS vmm_unit_tests)
        file(RELATIVE_PATH rel "${VMM_ROOT}/tests" "${test_source}")
        get_filename_component(dir "${rel}" DIRECTORY)
        string(REPLACE "/" "_" target "vmm_test_${dir}")
        add_executable(${target} "${test_source}")
        target_link_libraries(${target} PRIVATE vmm_test_support)
        set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/vmm/tests")
        gtest_discover_tests(${target} TEST_PREFIX "vmm/${dir}/" DISCOVERY_TIMEOUT 60
                             PROPERTIES LABELS "vmm")
    endforeach()

    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
        add_test(NAME vmm/ci/firewall_scan
                 COMMAND "${CMAKE_COMMAND}" -DCOMPILER=${CMAKE_CXX_COMPILER} -DSTD_FLAG=-std=c++23
                         -DINCLUDE_DIR=${VMM_ROOT}/include "-DHEADERS=${vmm_public_headers}"
                         -P "${VMM_ROOT}/cmake/ScanIncludes.cmake")
        add_test(NAME vmm/ci/firewall_scan_negative_control
                 COMMAND "${CMAKE_COMMAND}" -DCOMPILER=${CMAKE_CXX_COMPILER} -DSTD_FLAG=-std=c++23
                         -DINCLUDE_DIR=${VMM_ROOT}/tools/ci/negative
                         "-DHEADERS=${VMM_ROOT}/tools/ci/negative/vmm/contaminated.hpp" -DEXPECT_LEAK=ON
                         -P "${VMM_ROOT}/cmake/ScanIncludes.cmake")
        add_test(NAME vmm/ci/requirements
                 COMMAND "${Python3_EXECUTABLE}" "${VMM_ROOT}/tools/ci/check_requirements.py" --root "${VMM_ROOT}")
        set_tests_properties(vmm/ci/firewall_scan vmm/ci/firewall_scan_negative_control vmm/ci/requirements
                             PROPERTIES LABELS "vmm;ci")
    endif()

    if(VMM_COVERAGE)
        add_custom_target(vmm_coverage
            COMMAND "${Python3_EXECUTABLE}" "${VMM_ROOT}/tools/ci/check_coverage.py"
                    --root "${CMAKE_SOURCE_DIR}" --build "${CMAKE_BINARY_DIR}"
                    --output "${CMAKE_BINARY_DIR}/vmm_coverage"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT "vmm coverage per file (R25: 90% lines, 80% branches)")
    endif()
endif()

if(VMM_BUILD_VMM_TOOLS AND EXISTS "${VMM_ROOT}/tools/CMakeLists.txt")
    add_subdirectory(tools)
endif()
if(VMM_BUILD_VMM_EXAMPLES AND EXISTS "${VMM_ROOT}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

# ---------------------------------------------------------------------------
# Install and package (find_package(VoronoiMeshMaker)).
# ---------------------------------------------------------------------------
install(TARGETS ${vmm_targets} EXPORT VoronoiMeshMakerTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY "${VMM_ROOT}/include/vmm" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT VoronoiMeshMakerTargets NAMESPACE VoronoiMeshMaker::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VoronoiMeshMaker)
configure_package_config_file("${VMM_ROOT}/cmake/VoronoiMeshMakerConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/VoronoiMeshMakerConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VoronoiMeshMaker)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/VoronoiMeshMakerConfigVersion.cmake"
    VERSION 0.1.0 COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/VoronoiMeshMakerConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/VoronoiMeshMakerConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VoronoiMeshMaker)
