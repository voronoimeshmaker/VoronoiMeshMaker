# -----------------------------------------------------------------------------
# Tests configuration
# -----------------------------------------------------------------------------

if(VMM_BUILD_TESTS)
    message(STATUS "Building tests")
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()
