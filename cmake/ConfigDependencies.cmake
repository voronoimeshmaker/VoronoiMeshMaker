# Dependencies configuration

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
set(CMAKE_POLICY_DEFAULT_CMP0167 NEW)

find_package(CGAL REQUIRED COMPONENTS Core)

find_package(TBB REQUIRED)

if(VMM_BUILD_EXAMPLES)
    find_package(yaml-cpp QUIET)
    if(yaml-cpp_FOUND)
        message(STATUS "yaml-cpp found: YAML-based examples enabled")
    else()
        message(STATUS "yaml-cpp not found: YAML-based examples will be skipped")
    endif()
endif()

if(VMM_BUILD_TESTS)
    find_package(GTest REQUIRED)
    find_package(Threads REQUIRED)
endif()