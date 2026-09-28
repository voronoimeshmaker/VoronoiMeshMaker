# -----------------------------------------------------------------------------
# Shared compile and link settings
# -----------------------------------------------------------------------------

add_library(vmm_project_options INTERFACE)
add_library(VoronoiMeshMaker::project_options ALIAS vmm_project_options)

target_compile_features(vmm_project_options
    INTERFACE cxx_std_${VMM_CXX_STANDARD}
)

target_compile_options(vmm_project_options INTERFACE
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU,Clang>>:-O3>
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU,Clang>>:-DNDEBUG>
    $<$<AND:$<CONFIG:RelWithDebInfo>,$<CXX_COMPILER_ID:GNU,Clang>>:-O3>
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2>
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/DNDEBUG>
    $<$<AND:$<CONFIG:RelWithDebInfo>,$<CXX_COMPILER_ID:MSVC>>:/O2>
)

if(VMM_ENABLE_NATIVE_ARCH)
    target_compile_options(vmm_project_options INTERFACE
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU,Clang>>:-march=native>
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU,Clang>>:-mtune=native>
        $<$<AND:$<CONFIG:RelWithDebInfo>,$<CXX_COMPILER_ID:GNU,Clang>>:-march=native>
        $<$<AND:$<CONFIG:RelWithDebInfo>,$<CXX_COMPILER_ID:GNU,Clang>>:-mtune=native>
    )
endif()

if(VMM_ENABLE_FAST_MATH)
    target_compile_options(vmm_project_options INTERFACE
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU,Clang>>:-ffast-math>
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU,Clang>>:-fno-math-errno>
        $<$<AND:$<CONFIG:RelWithDebInfo>,$<CXX_COMPILER_ID:GNU,Clang>>:-ffast-math>
        $<$<AND:$<CONFIG:RelWithDebInfo>,$<CXX_COMPILER_ID:GNU,Clang>>:-fno-math-errno>
    )
endif()

add_library(vmm_project_warnings INTERFACE)
add_library(VoronoiMeshMaker::project_warnings ALIAS vmm_project_warnings)

if(VMM_ENABLE_WARNINGS)
    target_compile_options(vmm_project_warnings INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wextra>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wpedantic>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wshadow>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wconversion>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wsign-conversion>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<CXX_COMPILER_ID:MSVC>:/permissive->
    )
endif()

if(VMM_TEST_SANITIZERS)
    # Keep diagnostic translation units separate. GCC LTO can internalise CGAL
    # header globals differently in the executable and the shared library,
    # resulting in duplicate ASan global registrations.
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO OFF)
    message(STATUS "ASan/UBSan diagnostic build: LTO/IPO disabled")
elseif(VMM_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    else()
        message(WARNING "IPO/LTO not supported: ${ipo_error}")
    endif()
endif()

function(vmm_apply_common_target_settings target)
    target_link_libraries(${target} PRIVATE
        vmm_project_options
        vmm_project_warnings
    )
endfunction()

# Compatibility helper used by older CMake files in this repository.
function(set_target_optimizations target)
    target_link_libraries(${target} PRIVATE vmm_project_options)
endfunction()
