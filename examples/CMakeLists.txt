# ==============================================================================
# examples/CMakeLists.txt
# - Um executável por pasta que contenha ex_*.cpp
# - Detecção automática de dependência yaml-cpp por arquivo-fonte
# - Se yaml-cpp não estiver disponível, exemplos que usam YAML são pulados
# ==============================================================================
cmake_minimum_required(VERSION 3.10)

# Políticas úteis
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# Standalone (caso rode cmake direto nesta pasta)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(VoronoiMeshMakerExamples LANGUAGES CXX)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(PROJECT_ROOT      "${CMAKE_SOURCE_DIR}")
set(EXAMPLES_ROOT     "${CMAKE_CURRENT_SOURCE_DIR}")
set(VMM_INCLUDE_DIR "${PROJECT_ROOT}/VMMLib/include")

# --- yaml-cpp (opcional) ---
find_package(yaml-cpp QUIET)
if(yaml-cpp_FOUND)
  message(STATUS "[examples] yaml-cpp found; YAML-based examples will be built")
else()
  message(STATUS "[examples] yaml-cpp NOT found; YAML-based examples will be skipped")
endif()

# Alvos agregados
add_custom_target(examples           COMMENT "Compilando todos os exemplos")
add_custom_target(run_all_examples COMMENT "Executando todos os exemplos")

# 1) Varre recursivamente todos os ex_*.cpp
file(GLOB_RECURSE EXAMPLE_CPP_ABS CONFIGURE_DEPENDS "${EXAMPLES_ROOT}/ex_*.cpp")
if(EXAMPLE_CPP_ABS STREQUAL "")
  message(STATUS "[examples] Nenhum ex_*.cpp encontrado em ${EXAMPLES_ROOT}")
  return()
endif()

# 2) Agrupa por diretório
set(EXAMPLE_DIRS_ABS "")
foreach(f_abs IN LISTS EXAMPLE_CPP_ABS)
  get_filename_component(dir_abs "${f_abs}" DIRECTORY)
  list(APPEND EXAMPLE_DIRS_ABS "${dir_abs}")
endforeach()
list(REMOVE_DUPLICATES EXAMPLE_DIRS_ABS)

message(STATUS "[examples] Pastas de exemplos detectadas:")

foreach(dir_abs IN LISTS EXAMPLE_DIRS_ABS)
  file(GLOB EX_SOURCES_THIS_DIR "${dir_abs}/ex_*.cpp")
  if(EX_SOURCES_THIS_DIR STREQUAL "")
    continue()
  endif()

  # Nome do alvo
  file(RELATIVE_PATH dir_rel "${EXAMPLES_ROOT}" "${dir_abs}")
  if(dir_rel STREQUAL "")
    set(dir_rel ".")
  endif()

  list(LENGTH EX_SOURCES_THIS_DIR _n_src)
  if(_n_src EQUAL 1)
    list(GET EX_SOURCES_THIS_DIR 0 _only_src)
    get_filename_component(stem "${_only_src}" NAME_WE) # ex_foo
    set(target_name "${stem}")
  else()
    string(REPLACE "/" "_" target_name "${dir_rel}")
    set(target_name "ex_${target_name}")
    message(WARNING "[examples] Pasta '${dir_rel}' contém ${_n_src} exemplos; alvo único '${target_name}'.")
  endif()

  if(TARGET "${target_name}")
    string(MD5 _hash "${dir_rel}")
    string(SUBSTRING "${_hash}" 0 8 _short)
    set(target_name "${target_name}__${_short}")
  endif()

  # --- Detecta se algum fonte desta pasta usa yaml-cpp ---
  set(needs_yaml FALSE)
  foreach(src ${EX_SOURCES_THIS_DIR})
    file(READ "${src}" _src_txt)
    if(_src_txt MATCHES "yaml-cpp/yaml.h")
      set(needs_yaml TRUE)
    endif()
  endforeach()

  # Se precisa YAML mas não temos a lib, pula este exemplo
  if(needs_yaml AND NOT yaml-cpp_FOUND)
    message(STATUS "  - ${dir_rel}  -> SKIP (yaml-cpp não encontrado)")
    continue()
  else()
    message(STATUS "  - ${dir_rel}  -> alvo: ${target_name}"
                   "$<$<BOOL:${needs_yaml}>: (com yaml-cpp)>")
  endif()

  # Cria alvo
  add_executable(${target_name} ${EX_SOURCES_THIS_DIR})

  # ✅ CORREÇÃO: Adiciona a definição de compilação para passar o caminho fonte ao C++.
  # Esta linha cria a macro VMM_EXAMPLE_SOURCE_DIR que o seu .cpp pode ler.
  target_compile_definitions(${target_name} PRIVATE "VMM_EXAMPLE_SOURCE_DIR=\"${dir_abs}\"")

  target_link_libraries(${target_name} PRIVATE VoronoiMeshMaker)
  target_include_directories(${target_name} PRIVATE "${VMM_INCLUDE_DIR}")

  if(needs_yaml)
    target_link_libraries(${target_name} PRIVATE yaml-cpp)
  endif()

  # Output ao lado do .cpp
  if(_n_src EQUAL 1)
    set(out_name "${stem}")
  else()
    set(out_name "${target_name}")
  endif()
  set_target_properties(${target_name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/${dir_rel}"
    OUTPUT_NAME "${out_name}"
  )

  # Otimizações leves em Release
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(${target_name} PRIVATE -O3)
    elseif(MSVC)
      target_compile_options(${target_name} PRIVATE /O2)
    endif()
  endif()

  # Alvos auxiliares
  add_dependencies(examples ${target_name})
  add_custom_target(run_${target_name}
    DEPENDS ${target_name}
    COMMAND "${dir_abs}/${out_name}"
    WORKING_DIRECTORY "${dir_abs}"
    COMMENT "Executando exemplo ${out_name} (${dir_rel})"
  )
  add_dependencies(run_all_examples run_${target_name})
endforeach()