# -----------------------------------------------------------------------------
# Project options and layout
# -----------------------------------------------------------------------------

option(VMM_BUILD_SHARED_LIBS
    "Build VoronoiMeshMaker as a shared library"
    ON
)

option(VMM_BUILD_EXAMPLES
    "Build example programs"
    ON
)

option(VMM_BUILD_PAPER
    "Build paper/research helper programs"
    ON
)

option(VMM_BUILD_TESTS
    "Build GTest-based unit tests"
    ON
)

option(VMM_BUILD_DOCS
    "Build Doxygen/Sphinx documentation"
    OFF
)

option(VMM_ENABLE_WARNINGS
    "Enable project warning flags"
    ON
)

option(VMM_ENABLE_LTO
    "Enable interprocedural/link-time optimization in optimized builds"
    ON
)

option(VMM_ENABLE_NATIVE_ARCH
    "Optimize release builds for the native CPU architecture"
    ON
)

option(VMM_ENABLE_FAST_MATH
    "Enable aggressive floating-point optimizations; may change geometric results"
    OFF
)

set(VMM_CXX_STANDARD 20 CACHE STRING
    "C++ standard used by VoronoiMeshMaker"
)
set_property(CACHE VMM_CXX_STANDARD PROPERTY STRINGS 20 23)

# Backward compatibility with the previous cache option names.
if(DEFINED BUILD_EXAMPLES)
    set(VMM_BUILD_EXAMPLES ${BUILD_EXAMPLES} CACHE BOOL
        "Build example programs"
        FORCE
    )
endif()

if(DEFINED BUILD_TESTS)
    set(VMM_BUILD_TESTS ${BUILD_TESTS} CACHE BOOL
        "Build GTest-based unit tests"
        FORCE
    )
endif()

if(DEFINED BUILD_DOCS)
    set(VMM_BUILD_DOCS ${BUILD_DOCS} CACHE BOOL
        "Build Doxygen/Sphinx documentation"
        FORCE
    )
endif()

set(BUILD_EXAMPLES ${VMM_BUILD_EXAMPLES} CACHE BOOL
    "Compatibility alias for VMM_BUILD_EXAMPLES"
    FORCE
)
set(BUILD_TESTS ${VMM_BUILD_TESTS} CACHE BOOL
    "Compatibility alias for VMM_BUILD_TESTS"
    FORCE
)
set(BUILD_DOCS ${VMM_BUILD_DOCS} CACHE BOOL
    "Compatibility alias for VMM_BUILD_DOCS"
    FORCE
)

set(CMAKE_CXX_STANDARD ${VMM_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug
        Release
        RelWithDebInfo
        MinSizeRel
    )
endif()

include(GNUInstallDirs)

set(VMMLib_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VMMLib" CACHE PATH
    "VoronoiMeshMaker library root"
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}"
)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}"
)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin"
)

set(VMM_OUTPUT_BIN_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(BUILD_SHARED_LIBS ${VMM_BUILD_SHARED_LIBS})
