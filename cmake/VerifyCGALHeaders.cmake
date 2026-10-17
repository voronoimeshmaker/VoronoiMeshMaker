# SPDX-License-Identifier: GPL-3.0-or-later
# Keep the selected CGAL package and compiled headers in the same installation.
include_guard(GLOBAL)

function(vmm_verify_cgal_headers)
    set(version_header "")
    foreach(include_dir IN LISTS CGAL_INCLUDE_DIRS)
        if(EXISTS "${include_dir}/CGAL/version.h")
            set(version_header "${include_dir}/CGAL/version.h")
            set(cgal_headers "${include_dir}/CGAL")
            break()
        endif()
    endforeach()
    if(NOT version_header)
        message(FATAL_ERROR "The selected CGAL package has no readable version.h")
    endif()
    file(STRINGS "${version_header}" version_definition
        REGEX "^#define CGAL_VERSION_NR [0-9]+")
    string(REGEX MATCH "[0-9]+$" version_number "${version_definition}")
    if(NOT version_number)
        message(FATAL_ERROR "Cannot determine the selected CGAL header version")
    endif()

    # Compilers ignore explicit -I/usr/include. A build-local link exposes only
    # the selected CGAL subtree before an older /usr/local/include/CGAL copy.
    # It is recreated independently by installed-package consumers, not exported.
    file(REAL_PATH "${cgal_headers}" cgal_headers)
    string(SHA256 location_hash "${cgal_headers}")
    string(SUBSTRING "${location_hash}" 0 12 location_hash)
    set(include_overlay "${CMAKE_CURRENT_BINARY_DIR}/vmm_cgal_${location_hash}")
    file(MAKE_DIRECTORY "${include_overlay}")
    if(NOT EXISTS "${include_overlay}/CGAL")
        file(CREATE_LINK "${cgal_headers}" "${include_overlay}/CGAL"
             SYMBOLIC RESULT link_result)
        if(NOT link_result STREQUAL "0")
            message(FATAL_ERROR "Cannot isolate the selected CGAL headers: ${link_result}")
        endif()
    endif()
    get_target_property(cgal_target CGAL::CGAL ALIASED_TARGET)
    if(NOT cgal_target)
        set(cgal_target CGAL::CGAL)
    endif()
    target_include_directories(${cgal_target} SYSTEM BEFORE INTERFACE "${include_overlay}")

    include(CheckCXXSourceCompiles)
    # CMake < 3.29 cannot export aliases of imported targets into try_compile.
    # This probe only includes version.h; do not link any CGAL target.
    # https://cmake.org/cmake/help/v3.29/command/try_compile.html
    set(CMAKE_REQUIRED_LIBRARIES "")
    set(CMAKE_REQUIRED_INCLUDES "${include_overlay}")
    set(CMAKE_REQUIRED_DEFINITIONS "")
    set(CMAKE_REQUIRED_FLAGS "")
    get_target_property(cgal_includes ${cgal_target} INTERFACE_INCLUDE_DIRECTORIES)
    if(cgal_includes)
        list(APPEND CMAKE_REQUIRED_INCLUDES ${cgal_includes})
    endif()
    get_target_property(cgal_definitions ${cgal_target} INTERFACE_COMPILE_DEFINITIONS)
    if(cgal_definitions)
        foreach(definition IN LISTS cgal_definitions)
            list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D${definition}")
        endforeach()
    endif()
    get_target_property(cgal_options ${cgal_target} INTERFACE_COMPILE_OPTIONS)
    if(cgal_options)
        # CGAL 6.2.1 guards floating-point flags with COMPILE_LANGUAGE.
        # The probe is C++ only; resolve that guard before using plain flags.
        string(REPLACE "$<$<COMPILE_LANGUAGE:CXX>:" "" cgal_options "${cgal_options}")
        string(REPLACE ">" "" cgal_options "${cgal_options}")
        if(cgal_options MATCHES "\\$<")
            message(FATAL_ERROR "Unsupported CGAL probe compile options: ${cgal_options}")
        endif()
        string(JOIN " " CMAKE_REQUIRED_FLAGS ${cgal_options})
    endif()
    unset(VMM_CGAL_HEADERS_MATCH CACHE)
    check_cxx_source_compiles(
        "#include <CGAL/version.h>\nstatic_assert(CGAL_VERSION_NR == ${version_number});\nint main() {}"
        VMM_CGAL_HEADERS_MATCH)
    if(NOT VMM_CGAL_HEADERS_MATCH)
        message(FATAL_ERROR
            "CGAL package/header mismatch. Remove conflicting CGAL include flags; selected ${version_header}")
    endif()
    message(STATUS "VMM CGAL: ${CGAL_VERSION}; headers ${cgal_headers}; version number ${version_number}")
endfunction()
