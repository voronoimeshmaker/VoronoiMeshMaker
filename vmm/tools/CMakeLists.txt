# SPDX-License-Identifier: BSD-3-Clause
# vmm/ tools: benchmark and vmm-mesh. The oracle golden files (tests/data/golden) were
# generated once from the legacy VMMLib and are frozen since its retirement.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmark/benchmark.cpp")
    add_executable(vmm_benchmark benchmark/benchmark.cpp)
    target_link_libraries(vmm_benchmark PRIVATE ${vmm_targets} vmm_settings)
endif()

# vmm-mesh (DEC-040): mesh from a configuration file; installed with the library.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mesh/vmm_mesh.cpp")
    add_executable(vmm_mesh mesh/vmm_mesh.cpp)
    target_link_libraries(vmm_mesh PRIVATE vmm_facade vmm_settings)
    set_target_properties(vmm_mesh PROPERTIES OUTPUT_NAME vmm-mesh)
    install(TARGETS vmm_mesh RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    if(VMM_BUILD_VMM_TESTS)
        # The executable on the configuration examples of the manual, writing into the build tree.
        file(GLOB vmm_config_examples CONFIGURE_DEPENDS "${VMM_ROOT}/examples/config/*.cfg")
        foreach(config IN LISTS vmm_config_examples)
            get_filename_component(name "${config}" NAME_WE)
            add_test(NAME vmm/tools/vmm_mesh/${name}
                     COMMAND vmm_mesh --output "${CMAKE_BINARY_DIR}/vmm/config_examples/${name}" "${config}")
            set_tests_properties(vmm/tools/vmm_mesh/${name} PROPERTIES LABELS "vmm")
        endforeach()
        add_test(NAME vmm/tools/vmm_mesh/version COMMAND vmm_mesh --version)
        add_test(NAME vmm/tools/vmm_mesh/no_file COMMAND vmm_mesh)
        add_test(NAME vmm/tools/vmm_mesh/bad_argument COMMAND vmm_mesh --language fr x.cfg)
        add_test(NAME vmm/tools/vmm_mesh/missing_file COMMAND vmm_mesh "${CMAKE_BINARY_DIR}/no/such/file.cfg")
        set_tests_properties(vmm/tools/vmm_mesh/version PROPERTIES LABELS "vmm"
                             PASS_REGULAR_EXPRESSION "^vmm-mesh [0-9]+\\.[0-9]+\\.[0-9]+")
        set_tests_properties(vmm/tools/vmm_mesh/no_file vmm/tools/vmm_mesh/bad_argument vmm/tools/vmm_mesh/missing_file
                             PROPERTIES LABELS "vmm" WILL_FAIL TRUE)
    endif()
endif()
