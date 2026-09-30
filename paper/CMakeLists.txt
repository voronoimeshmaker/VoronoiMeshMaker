# ==============================================================================
# paper/CMakeLists.txt
# - One executable per folder that contains .cpp files
# - New paper program folders are detected automatically
# - Each executable receives VMM_PAPER_CASE_SOURCE_DIR with its own source folder
# ==============================================================================

add_custom_target(paper_programs COMMENT "Building all paper/research programs")
add_custom_target(run_all_paper_programs COMMENT "Running all paper/research programs")

set(PAPER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE PAPER_CPP_ABS CONFIGURE_DEPENDS "${PAPER_ROOT}/*.cpp")

if(PAPER_CPP_ABS STREQUAL "")
    message(STATUS "[paper] No .cpp files found in ${PAPER_ROOT}")
    return()
endif()

set(PAPER_DIRS_ABS "")
foreach(src_abs IN LISTS PAPER_CPP_ABS)
    get_filename_component(dir_abs "${src_abs}" DIRECTORY)
    list(APPEND PAPER_DIRS_ABS "${dir_abs}")
endforeach()
list(REMOVE_DUPLICATES PAPER_DIRS_ABS)

message(STATUS "[paper] Program folders detected:")

foreach(dir_abs IN LISTS PAPER_DIRS_ABS)
    file(GLOB PAPER_SOURCES_THIS_DIR CONFIGURE_DEPENDS "${dir_abs}/*.cpp")
    if(PAPER_SOURCES_THIS_DIR STREQUAL "")
        continue()
    endif()

    file(RELATIVE_PATH dir_rel "${PAPER_ROOT}" "${dir_abs}")
    if(dir_rel STREQUAL "")
        set(dir_rel ".")
    endif()

    list(LENGTH PAPER_SOURCES_THIS_DIR source_count)
    if(source_count EQUAL 1)
        list(GET PAPER_SOURCES_THIS_DIR 0 only_source)
        get_filename_component(stem "${only_source}" NAME_WE)
        set(target_name "paper_${stem}")
        set(output_name "${target_name}")
    else()
        string(REPLACE "/" "_" target_suffix "${dir_rel}")
        string(REPLACE " " "_" target_suffix "${target_suffix}")
        set(target_name "paper_${target_suffix}")
        set(output_name "${target_name}")
        message(WARNING "[paper] Folder '${dir_rel}' has ${source_count} sources; creating one target '${target_name}'.")
    endif()

    if(TARGET "${target_name}")
        string(MD5 target_hash "${dir_rel}")
        string(SUBSTRING "${target_hash}" 0 8 target_hash_short)
        set(target_name "${target_name}__${target_hash_short}")
        set(output_name "${target_name}")
    endif()

    message(STATUS "  - ${dir_rel} -> target: ${target_name}")

    add_executable(${target_name} ${PAPER_SOURCES_THIS_DIR})

    target_link_libraries(${target_name}
        PRIVATE
            VoronoiMeshMaker
    )

    target_compile_definitions(${target_name}
        PRIVATE
            "VMM_PAPER_CASE_SOURCE_DIR=\"${dir_abs}\""
    )

    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/paper/${dir_rel}"
        OUTPUT_NAME "${output_name}"
    )

    add_dependencies(paper_programs ${target_name})

    add_custom_target(run_${target_name}
        DEPENDS ${target_name}
        COMMAND "${dir_abs}/${output_name}"
        WORKING_DIRECTORY "${dir_abs}"
        COMMENT "Running paper program ${output_name} (${dir_rel})"
    )

    add_dependencies(run_all_paper_programs run_${target_name})
endforeach()
