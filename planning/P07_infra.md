# P07 — Infraestrutura de build, testes e CI

- **Data:** 2026-09-29
- **Sequência:** v5.2, prompt P07. Rodou em sequência com o P08–P14, sem aprovação entre prompts, por pedido do João.
- **Ambiente:** WSL `Ubuntu-26.04-Test`; g++ 15.2.0 e g++ 14.3.0 (build Release com `-Werror` e 154/154 testes aprovados em 30/09); CMake 4.2.3; Ninja 1.13.2; CGAL 6.2.1 (`/usr/local`, headers verificados por `static_assert`); Boost 1.92; GMP 6.3.0; GoogleTest 1.17.0; gcovr 7.2; Python 3.14.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. O que foi feito

| Tarefa | Entrega |
|---|---|
| 1. CMake, alvos exportados, `install()`, `find_package` | `vmm/CMakeLists.txt`: alvos `vmm_core`, `vmm_backend_cgal`, `vmm_io`, `vmm` (fachada), aliases `VoronoiMeshMaker::*`, `install(EXPORT)`, `VoronoiMeshMakerConfig.cmake` (`find_package(VoronoiMeshMaker 0.1)`). A VMMLib legada continua como oráculo (`VMM_BUILD_VMMLIB`, ON) e deixou de ser instalada por padrão (`VMM_INSTALL_VMMLIB`, OFF) |
| 2. Árvore de testes espelhada | `vmm/tests/<módulo>/<Classe>/ut_<Classe>.cpp`, uma executável por arquivo, descoberta automática; `tests/integration/*` para os testes de integração |
| 3. Helpers de teste | `tests/support/`: domínios de teste, malhas pequenas, malha aleatória com semente, verificador de invariantes e assinatura canônica |
| 4. GitHub Actions | `.github/workflows/vmm-ci.yml`: GCC 14 Debug/Release, Clang 18 Release, ASan/UBSan, cobertura por arquivo, verificadores estáticos; instala o CGAL 6.2.1 do tarball oficial |
| 5. Validação local | tudo abaixo foi executado no WSL |
| 6. DEC-012 | `NATIVE_ARCH` e `LTO` OFF por padrão; opção `-ffast-math` removida; executáveis de exemplos e paper fora da árvore de fontes; C++23; `-Werror` nos alvos novos; versões impressas no configure |
| 7. C++23 | `VMM_CXX_STANDARD` = 23 (única opção) |
| 8. Golden files | `vmm/tools/golden/generate_golden.cpp` (O1–O4), gerados num build Release sem `-ffast-math` e sem `NATIVE_ARCH`, com compilador, flags e versões no cabeçalho: `vmm/tests/data/golden/*.golden` |
| 9. Benchmark | `vmm/tools/benchmark/benchmark.cpp` (B1–B3), resultado em §4 |
| 10. Verificadores | `vmm/tools/ci/check_requirements.py` (R1, R2, R3, R4, R8, R13, R21, R23, ordem de includes do AGENTS.md), `check_coverage.py` (R25), firewall de headers (R19) com controle negativo |

## 2. Alterações em arquivos existentes (preservando as alterações não commitadas do João)

- `CMakeLists.txt`: legado sob `if(VMM_BUILD_VMMLIB)`, `enable_testing()`, `add_subdirectory(vmm)`, descrição do projeto.
- `cmake/ConfigOptions.cmake`: C++23, `LTO`/`NATIVE_ARCH` OFF, sem `FAST_MATH`, opções `VMM_BUILD_VMMLIB` e `VMM_INSTALL_VMMLIB`.
- `cmake/ConfigCompiler.cmake`: bloco do `-ffast-math` removido.
- `cmake/ConfigTargets.cmake`: instalação do legado condicionada a `VMM_INSTALL_VMMLIB`.
- `examples/CMakeLists.txt`, `paper/CMakeLists.txt`: saída em `${CMAKE_BINARY_DIR}`; sem `-march=native` nos exemplos.

**Higiene do git (DEC-012 (1)), feita em 30/09:**
- os 55 executáveis e o `.pyc` versionados saíram do índice (`git rm --cached`, arquivos mantidos no disco) e entraram no `.gitignore`. A remoção está em *staging*, aguardando o commit do João;
- `.gitattributes` com `* text=auto eol=lf`;
- as ~85 cópias de trabalho que diferiam do commit só por CRLF foram convertidas para LF (conteúdo idêntico ao commit, verificado por hash), com backup.

## 3. Evidência (WSL)

| Verificação | Resultado **[F]** |
|---|---|
| Build Debug (`VMM_BUILD_VMMLIB=OFF`) com `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror` | sem avisos |
| `ctest -L vmm` | 143/143 na última rodada completa registrada (inclui 1000 configurações aleatórias); ver §5 do P10 |
| Firewall: 26 headers públicos compilados sem o alvo CGAL e varridos com `-M` | 0 headers do CGAL/Boost/GMP/MPFR; o controle negativo é detectado |
| `check_requirements.py` | 0 problemas; grafo de módulos sem ciclos |
| Build Release completo com a VMMLib | sem erros; **249/249** testes legados passando |
| Oráculo O1–O4 | topologia idêntica à da VMMLib; área com erro relativo ≤ 1,1·10⁻¹³ (P10 §4) |

**Não verificado localmente:**
- **Clang 18:** não está instalado no WSL; só roda na CI.
- **O workflow em si:** o YAML é válido, mas ele não foi executado no GitHub, porque não fiz push.

## 4. Benchmark (DEC-021), Release, 1 thread

**[F]** Medido com `vmm_benchmark all 1000000`:

| Caso | Células | Sítios | Construção | Delaunay | Células | Montagem | Invariantes | Pico de RSS |
|---|---|---|---|---|---|---|---|---|
| B1 (quadrado, 10⁶ sítios aleatórios) | 1 000 000 | 0,19 s | 5,1 s | 0,76 s | 1,6 s | 2,5 s | PASS | 642 MB (673 B/célula) |
| B2 (A1) | 32 053 | 0,01 s | 0,34 s | 0,02 s | 0,24 s | 0,08 s | PASS | — |
| B3 (A2) | 288 514 | 1,8 s | 7,9 s | 0,16 s | 6,5 s | 0,44 s | PASS | — |

Primeira medida (antes da otimização da montagem): B1 em 10,2 s e 1,34 KB/célula. A troca do mapa de peças de
bissetor por um vetor plano ordenado e da grade de fusão de vértices (`std::map`) por um `unordered_multimap`
reduziu o pico de memória à metade e o tempo de B1 à metade, com o mesmo checksum de topologia.

**[I]** O RSS é o pico do processo inteiro, e B2 e B3 rodam depois de B1. Por isso a coluna só vale para B1.

**Recalibração única das metas (DEC-020):**
- **Tempo 2D:** 10⁶ células em 10,2 s, dentro da meta de 30 s. Mantida.
- **Memória 2D:** 673 B/célula, dentro da meta de 1 KB. Mantida, sem recalibração.
- **3D:** sem medida; revisar após o P15a.

## 5. Como reproduzir

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DVMM_BUILD_VMMLIB=ON -DVMM_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build -L vmm
python3 vmm/tools/ci/check_requirements.py --root vmm
build/bin/vmm_golden_generator vmm/tests/data/golden
build/bin/vmm_benchmark all 1000000
```
