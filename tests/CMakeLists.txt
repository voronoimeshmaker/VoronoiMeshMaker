# ==============================================================================
# tests/CMakeLists.txt — One test per folder (ut_<FOLDER>.cpp), always on
# - GTest/GMock + CTest discovery
# - Parallel run (ctest -j)
# - Coverage (gcovr) target
# - Valgrind (memcheck + per-target runners)
# - ASan + UBSan enabled automatically on GCC/Clang
# ==============================================================================

cmake_minimum_required(VERSION 3.20)

# Standalone mode (permite rodar tests/ isolado, opcional)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(VoronoiMeshMakerTests LANGUAGES CXX)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

include(CTest)
enable_testing()
include(GoogleTest)

# ------------------------------------------------------------------------------
# Dependências
# ------------------------------------------------------------------------------
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Procurar TBB (Threading Building Blocks), opcional.
find_package(TBB QUIET)

# --- Core lib para resolver símbolos (ex.: ErrorConfig::ErrorConfig) ---
set(VMM_CORE_LIB VoronoiMeshMaker)
if(TARGET ${VMM_CORE_LIB})
  message(STATUS "[Tests] Linking against core lib: ${VMM_CORE_LIB}")
else()
  message(STATUS "[Tests] Core lib '${VMM_CORE_LIB}' not found; building tests without it")
endif()

# ------------------------------------------------------------------------------
# Sanitizers (sempre habilitados quando suportados)
#   - TSan não é ativado por padrão (conflita com ASan).
# ------------------------------------------------------------------------------
set(_SAN_FLAGS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  list(APPEND _SAN_FLAGS -fsanitize=address -fsanitize=undefined)
  list(APPEND _SAN_FLAGS -fno-omit-frame-pointer)
endif()

# ------------------------------------------------------------------------------
# Coverage flags (aplicados quando suportados)
# ------------------------------------------------------------------------------
set(_COV_FLAGS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  list(APPEND _COV_FLAGS --coverage -fprofile-arcs -ftest-coverage)
endif()

# ------------------------------------------------------------------------------
# Utilitário: nome de alvo único e legível a partir do diretório relativo
# ------------------------------------------------------------------------------
function(_sanitize_target_name out_name rel_dir)
  string(REPLACE "/" "_" _tmp "${rel_dir}")
  string(REPLACE "\\" "_" _tmp "${_tmp}")
  string(REPLACE "." "_" _tmp "${_tmp}")
  set(${out_name} "${_tmp}" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------------------
# Descoberta: exatamente 1 driver por pasta, nomeado ut_<PASTA>.cpp
# Ex.: tests/ErrorHandling/CoreErrors/ut_CoreErrors.cpp
# ------------------------------------------------------------------------------
set(TESTS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# Primeiro, vamos verificar se a pasta tests existe e listar seu conteúdo
message(STATUS "Tests root directory: ${TESTS_ROOT}")

# Lista todos os arquivos ut_*.cpp recursivamente
file(GLOB_RECURSE _ALL_UT_CPP RELATIVE "${TESTS_ROOT}" "ut_*.cpp")

# Se não encontrar, tenta métodos alternativos
if(NOT _ALL_UT_CPP)
  message(WARNING "No ut_*.cpp files found with GLOB_RECURSE. Trying alternative search...")

  # Método alternativo: lista diretórios primeiro, depois procura por ut_*.cpp
  file(GLOB_RECURSE _ALL_DIRS RELATIVE "${TESTS_ROOT}" "*")
  foreach(item IN LISTS _ALL_DIRS)
    if(IS_DIRECTORY "${TESTS_ROOT}/${item}")
      file(GLOB _ut_files_in_dir RELATIVE "${TESTS_ROOT}" "${item}/ut_*.cpp")
      if(_ut_files_in_dir)
        list(APPEND _ALL_UT_CPP ${_ut_files_in_dir})
      endif()
    endif()
  endforeach()
endif()

# Se ainda não encontrar, lista o conteúdo do diretório para debug
if(NOT _ALL_UT_CPP)
  message(STATUS "Listing contents of tests directory:")
  file(GLOB_RECURSE _ALL_FILES RELATIVE "${TESTS_ROOT}" "*")
  foreach(file IN LISTS _ALL_FILES)
    message(STATUS "  Found: ${file}")
  endforeach()
  message(FATAL_ERROR "[Tests] No ut_*.cpp drivers found under ${TESTS_ROOT}")
else()
  message(STATUS "Found test files: ${_ALL_UT_CPP}")
endif()

set(ALL_TEST_TARGETS "")
set(VALGRIND_RUN_TARGETS "")
set(RUN_TEST_TARGETS "")

# Descobre Valgrind (para targets mais abaixo)
find_program(VALGRIND_EXECUTABLE valgrind)
set(_HAVE_VALGRIND FALSE)
if(VALGRIND_EXECUTABLE)
  set(_HAVE_VALGRIND TRUE)
  # Integração CTest Memcheck
  set(MEMORYCHECK_COMMAND "${VALGRIND_EXECUTABLE}")
  set(MEMORYCHECK_COMMAND_OPTIONS
      "--leak-check=full --show-leak-kinds=all --track-origins=yes "
      "--error-exitcode=1")
endif()

# Descobre gcovr para coverage
find_program(GCOVR_EXECUTABLE gcovr)
set(_HAVE_GCOVR FALSE)
if(GCOVR_EXECUTABLE)
  set(_HAVE_GCOVR TRUE)
  message(STATUS "[Coverage] gcovr found: ${GCOVR_EXECUTABLE}")
else()
  message(STATUS "[Coverage] gcovr not found. Target 'coverage' disabled.")
endif()

foreach(ut_file_rel IN LISTS _ALL_UT_CPP)
  get_filename_component(test_dir_rel "${ut_file_rel}" DIRECTORY)
  get_filename_component(test_name_we "${ut_file_rel}" NAME_WE) # ut_X
  get_filename_component(file_name "${ut_file_rel}" NAME)       # ut_X.cpp
  get_filename_component(folder_name "${test_dir_rel}" NAME)    # X

  # Para debug - mostra informações do arquivo encontrado
  message(STATUS "Processing test file: ${ut_file_rel}")
  message(STATUS "  Directory: ${test_dir_rel}")
  message(STATUS "  File name: ${file_name}")
  message(STATUS "  Folder name: ${folder_name}")

  # Verifica se o nome do arquivo corresponde ao padrão esperado
  if(NOT file_name MATCHES "^ut_.*\\.cpp$")
    message(WARNING "File ${file_name} doesn't match ut_*.cpp pattern")
    continue()
  endif()

  _sanitize_target_name(_tgt "${test_dir_rel}")
  set(target_name "ut_${_tgt}")  # ex.: ut_ErrorHandling_CoreErrors

  message(STATUS "Creating target: ${target_name} for ${ut_file_rel}")

  add_executable(${target_name} "${TESTS_ROOT}/${ut_file_rel}")
  target_compile_features(${target_name} PRIVATE cxx_std_20)

  # Sanitizers (somente quando suportados)
  if(_SAN_FLAGS)
    target_compile_options(${target_name} PRIVATE ${_SAN_FLAGS})
    target_link_options(${target_name}    PRIVATE ${_SAN_FLAGS})
  endif()

  # Coverage flags - aplicado apenas se gcovr estiver disponível
  if(_HAVE_GCOVR AND _COV_FLAGS)
    target_compile_options(${target_name} PRIVATE ${_COV_FLAGS})
    target_link_options(${target_name}    PRIVATE ${_COV_FLAGS})
  endif()

  target_link_libraries(${target_name} PRIVATE
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
  )
  if(TARGET TBB::tbb)
    target_link_libraries(${target_name} PRIVATE TBB::tbb)
  endif()
  if(TARGET GTest::gmock)
    target_link_libraries(${target_name} PRIVATE GTest::gmock)
  endif()

  # Link na lib principal, se presente (resolve símbolos como ErrorConfig::ErrorConfig)
  if(TARGET ${VMM_CORE_LIB})
    target_link_libraries(${target_name} PRIVATE ${VMM_CORE_LIB})
  endif()

  # Binário sai dentro da própria pasta do driver
  set_target_properties(${target_name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TESTS_ROOT}/${test_dir_rel}"
    OUTPUT_NAME              "${test_name_we}"
  )

  # Registro no CTest (descobre os TEST() dentro do driver)
  gtest_discover_tests(${target_name}
    WORKING_DIRECTORY "${TESTS_ROOT}/${test_dir_rel}"
    TEST_PREFIX "${folder_name}:"
    DISCOVERY_TIMEOUT 30
  )

  list(APPEND ALL_TEST_TARGETS ${target_name})

  # Alvo para executar apenas este teste específico
  set(run_target_name "run_${target_name}")
  add_custom_target(${run_target_name}
    COMMAND $<TARGET_FILE:${target_name}> --gtest_output=xml:${target_name}_results.xml
    DEPENDS ${target_name}
    WORKING_DIRECTORY "${TESTS_ROOT}/${test_dir_rel}"
    COMMENT "Running ${folder_name} tests"
  )
  list(APPEND RUN_TEST_TARGETS ${run_target_name})

  # Runner Valgrind por-alvo (se disponível)
  if(_HAVE_VALGRIND)
    set(vtgt "run_${target_name}_valgrind")
    add_custom_target(${vtgt}
      COMMAND "${VALGRIND_EXECUTABLE}"
              --leak-check=full --show-leak-kinds=all
              --track-origins=yes --error-exitcode=1
              $<TARGET_FILE:${target_name}>
      DEPENDS ${target_name}
      WORKING_DIRECTORY "${TESTS_ROOT}/${test_dir_rel}"
      COMMENT "[Valgrind] ${folder_name}"
    )
    list(APPEND VALGRIND_RUN_TARGETS ${vtgt})
  endif()
endforeach()

# ------------------------------------------------------------------------------
# Alvos agregadores
# ------------------------------------------------------------------------------
if(ALL_TEST_TARGETS)
  add_custom_target(tests DEPENDS ${ALL_TEST_TARGETS})
else()
  message(WARNING "No test targets were created!")
endif()

include(ProcessorCount)
ProcessorCount(NPROC)
if(NOT NPROC OR NPROC EQUAL 0)
  set(NPROC 2)
endif()

# Execução paralela de TODOS os testes (usando ctest)
if(ALL_TEST_TARGETS)
  add_custom_target(run_tests_all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -j ${NPROC}
                                   --no-tests=error
    DEPENDS ${ALL_TEST_TARGETS}
    COMMENT "Running ALL tests in parallel with ctest -j ${NPROC}"
  )

  # Execução paralela de testes INDIVIDUAIS por pasta
  if(RUN_TEST_TARGETS)
    add_custom_target(run_tests_parallel
      COMMAND ${CMAKE_COMMAND} --build . --target ${RUN_TEST_TARGETS} -j ${NPROC}
      DEPENDS ${ALL_TEST_TARGETS}
      COMMENT "Running test executables in parallel (${NPROC} jobs)"
    )

    # Execução sequencial de todos os testes individuais
    add_custom_target(run_tests_sequential
      DEPENDS ${RUN_TEST_TARGETS}
      COMMENT "Running all test executables sequentially"
    )
  endif()
endif()

# Memcheck via CTest (se Valgrind disponível)
if(_HAVE_VALGRIND AND ALL_TEST_TARGETS)
  add_custom_target(memcheck
    COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process
                                   --output-on-failure -T memcheck
    DEPENDS ${ALL_TEST_TARGETS}
    COMMENT "CTest Memcheck (Valgrind)"
  )
  if(VALGRIND_RUN_TARGETS)
    add_custom_target(valgrind_tests
      DEPENDS ${VALGRIND_RUN_TARGETS}
      COMMENT "Running all test drivers under Valgrind"
    )

    # Execução paralela dos testes com Valgrind
    add_custom_target(run_valgrind_parallel
      COMMAND ${CMAKE_COMMAND} --build . --target ${VALGRIND_RUN_TARGETS} -j ${NPROC}
      DEPENDS ${ALL_TEST_TARGETS}
      COMMENT "Running Valgrind tests in parallel (${NPROC} jobs)"
    )
  endif()
else()
  message(STATUS "[Valgrind] Not found. Skipping memcheck/valgrind targets.")
endif()

# ------------------------------------------------------------------------------
# Coverage report (gcovr) - apenas se gcovr estiver disponível
# ------------------------------------------------------------------------------
if(_HAVE_GCOVR AND ALL_TEST_TARGETS)
  # Cria diretório para relatórios de coverage
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/coverage_reports)

  add_custom_target(coverage
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -j ${NPROC}
                                   --no-tests=error
    COMMAND ${GCOVR_EXECUTABLE}
            -r ${CMAKE_SOURCE_DIR}
            --exclude '${CMAKE_CURRENT_SOURCE_DIR}'
            --exclude '.*/tests/.*'
            --exclude '.*/build/.*'
            --html coverage_reports/coverage.html
            --html-details coverage_reports/coverage_details.html
            --xml coverage_reports/coverage.xml
            --sonarqube coverage_reports/sonarqube.xml
            --print-summary
            --gcov-executable "gcov"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ${ALL_TEST_TARGETS}
    COMMENT "Generating coverage report in coverage_reports/"
  )

  # Target adicional para coverage apenas dos testes
  add_custom_target(coverage_tests_only
    COMMAND ${GCOVR_EXECUTABLE}
            -r ${CMAKE_CURRENT_SOURCE_DIR}
            --html coverage_reports/tests_coverage.html
            --xml coverage_reports/tests_coverage.xml
            --print-summary
            --gcov-executable "gcov"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ${ALL_TEST_TARGETS}
    COMMENT "Generating tests-only coverage report"
  )

else()
  message(STATUS "[Coverage] gcovr not found. Target 'coverage' disabled.")

  # Target dummy para quando coverage não está disponível
  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E echo "Coverage not available: gcovr not found"
    COMMAND ${CMAKE_COMMAND} -E echo "Install with: pip install gcovr"
    COMMENT "Coverage target requires gcovr"
  )
endif()

# ------------------------------------------------------------------------------
# Instalação do gcovr se não estiver disponível (opcional)
# ------------------------------------------------------------------------------
if(NOT _HAVE_GCOVR)
  find_program(PIP_EXECUTABLE pip pip3)
  if(PIP_EXECUTABLE)
    add_custom_target(install_gcovr
      COMMAND ${PIP_EXECUTABLE} install gcovr
      COMMENT "Installing gcovr for code coverage"
    )
  else()
    add_custom_target(install_gcovr
      COMMAND ${CMAKE_COMMAND} -E echo "pip not found. Install gcovr manually: pip install gcovr"
      COMMENT "Cannot install gcovr automatically"
    )
  endif()
endif()
