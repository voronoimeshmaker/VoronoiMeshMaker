# Main library target

include(CMakePackageConfigHelpers)
find_package(ZLIB REQUIRED)

file(GLOB_RECURSE VMM_SOURCES CONFIGURE_DEPENDS
    "${VMMLib_DIR}/src/*.cpp"
)

add_library(VoronoiMeshMaker ${VMM_SOURCES})
add_library(VoronoiMeshMaker::VoronoiMeshMaker ALIAS VoronoiMeshMaker)

set_target_properties(VoronoiMeshMaker PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

target_include_directories(VoronoiMeshMaker
    PUBLIC
        $<BUILD_INTERFACE:${VMMLib_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(VoronoiMeshMaker
    PUBLIC
        cxx_std_${VMM_CXX_STANDARD}
)

target_link_libraries(VoronoiMeshMaker
    PUBLIC
        CGAL::CGAL
        CGAL::CGAL_Core
        TBB::tbb
    PRIVATE
        ZLIB::ZLIB
        vmm_project_options
        vmm_project_warnings
)

if(VMM_TEST_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Every build-tree executable loading the instrumented library must link
    # the sanitizer runtimes directly. Do not export diagnostic flags.
    foreach(flag IN ITEMS -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
        target_compile_options(VoronoiMeshMaker PUBLIC "$<BUILD_INTERFACE:${flag}>")
        target_link_options(VoronoiMeshMaker PUBLIC "$<BUILD_INTERFACE:${flag}>")
    endforeach()
    message(STATUS "Sanitizers enabled: use a separate non-instrumented build for installation")
endif()

if(NOT VMM_INSTALL_VMMLIB)
    return()
endif()

install(TARGETS VoronoiMeshMaker
    EXPORT VoronoiMeshMakerTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY "${VMMLib_DIR}/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*.tpp"
)

install(EXPORT VoronoiMeshMakerTargets
    FILE VoronoiMeshMakerTargets.cmake
    NAMESPACE VoronoiMeshMaker::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VoronoiMeshMaker
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VoronoiMeshMakerConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/VoronoiMeshMakerConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VoronoiMeshMaker
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/VoronoiMeshMakerConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyCGALHeaders.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/VoronoiMeshMakerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/VoronoiMeshMakerConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VoronoiMeshMaker
)
