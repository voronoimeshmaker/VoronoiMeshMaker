# SPDX-License-Identifier: BSD-3-Clause
# Gallery examples (P13): examples/<name>/ex_<name>.cpp, run from the build tree.
file(GLOB vmm_example_dirs LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(dir IN LISTS vmm_example_dirs)
    if(IS_DIRECTORY "${dir}")
        get_filename_component(name "${dir}" NAME)
        if(EXISTS "${dir}/ex_${name}.cpp")
            add_executable(vmm_ex_${name} "${dir}/ex_${name}.cpp")
            target_link_libraries(vmm_ex_${name} PRIVATE vmm_facade vmm_io vmm_settings)
            target_include_directories(vmm_ex_${name} PRIVATE "${VMM_ROOT}/tools")
            set_target_properties(vmm_ex_${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/vmm/examples")
        endif()
    endif()
endforeach()
