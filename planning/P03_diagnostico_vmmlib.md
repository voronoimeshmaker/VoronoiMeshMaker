# P03 — Diagnóstico da VMMLib (v2.2)

- **Data:** 2026-09-28
- **Sequência:** v4, prompt P03
- **Destino no repositório:** `planning/P03_diagnostico_vmmlib.md`, `planning/P03_inventario.csv` e `planning/P03_cobertura_por_arquivo.csv`
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação · **NÃO VERIFICADO** quando não foi possível obter.
- **Histórico:**
  - **v1:** primeira execução, na nuvem.
  - **v2:** reexecução com a especificação detalhada do João.
  - **v2.1:**
    - evidência oficial do WSL (build, testes, cobertura, versões);
    - correções da autorrevisão: CGAL em 93 de 137 headers, cobertura por arquivo, inventário corrigido, tolerâncias absolutas;
    - pontos aceitos da revisão externa: argumento a favor de A explícito; a restrição de domínio convexo não pode ser herdada.
  - **v2.2 (esta):**
    - reexecução no WSL depois da atualização do CGAL e do Boost (§1, §3, §4, §5);
    - cobertura de ramos e funções medida; cobertura por arquivo do WSL (`P03_cobertura_por_arquivo.csv`) substitui a da nuvem;
    - correção: o `.cpp` da biblioteca é instrumentado;
    - DEC-011 e DEC-012 aprovadas com ajustes (ver `planning/DECISIONS.md`).

> **Base da análise.** A análise estática foi feita sobre o commit `7402178` do GitHub. **[F]** No WSL oficial, o HEAD é o mesmo commit, e `git diff --ignore-cr-at-eol -w` sai vazio. Os 84 arquivos marcados como modificados diferem só em fim de linha (CRLF/LF), então **a análise estática vale para a árvore de trabalho do João**. Build, testes e cobertura foram executados **no WSL oficial** pelo João, em 28/09, com o roteiro deste documento, primeiro com o CGAL 6.0-beta1 (v2.1) e depois com o CGAL 6.2.1-I-900 e o Boost 1.92 (v2.2).

---

## 1. Ambiente verificado

| Item | WSL `Ubuntu-26.04-Test` (**oficial**, v2.2) | Como foi obtido |
|---|---|---|
| Compilador | g++ **15.2.0** | `g++ --version` |
| CMake | **4.2.3** | `cmake --version` |
| gcovr | 7.2 | `gcovr --version` |
| CGAL usado pelo CMake | **6.2.1-I-900** (`CGAL_VERSION_NR 1060210900`), em `/usr/local` | mensagem `VMM CGAL:` do configure; `CGAL_DIR=/usr/local/lib/cmake/CGAL` |
| Outro CGAL instalado | `libcgal-dev` **6.1.1**, em `/usr` | `/usr/include/CGAL/version.h` |
| Boost usado pelo CMake | **1.92** | `Boost_DIR=/usr/local/lib/cmake/Boost-1.92.0` |
| Outros Boost instalados | 1.86 em `/usr/local/include/boost-1_86`; 1.90 em `/usr` | `BOOST_LIB_VERSION` |
| TBB / GoogleTest | pacotes do sistema (`TBB_DIR` e `GTest_DIR` em `/usr/lib/x86_64-linux-gnu/cmake`); na v2.1: TBB 2022.3.0, GTest 1.17.0 | `CMakeCache.txt`; `dpkg -l` (v2.1) |
| GMP / MPFR | 6.3.0 / 4.2.2 (v2.1; não reverificado) | `dpkg -l` |

**[F]** A execução da v2.1 usou um CGAL **6.0-beta1** e o Boost 1.86. A da v2.2 usou um CGAL que se identifica como **6.2.1-I-900**. Esse identificador não é o de um release: o pacote oficial `CGAL-6.2.1.tar.xz` e a tag `v6.2.1` trazem `CGAL_VERSION 6.2.1` e `CGAL_VERSION_NR 1060211000`.

**[F]** Depois da execução, o João reinstalou o CGAL a partir do pacote oficial; `version.h` em `/usr/local` passou a indicar `6.2.1` / `1060211000`. Build e testes **não** foram repetidos com essa instalação.

**[I]** A diferença entre o build interno e o release 6.2.1 não afeta nenhuma conclusão deste diagnóstico, que é sobre a estrutura do código. A linha de base de referência (golden files, DEC-011) é gerada no P07, já com versões estáveis registradas (DEC-012).

**[I]** As licenças por pacote verificadas no P02 (6.2.x) valem para a versão em uso.

## 2. Estado do repositório

| Item | Valor **[F]** |
|---|---|
| Branch / HEAD (WSL) | `main` / `740217839232d76e74b6b1e3d441e83e635ab487`, igual ao GitHub |
| Arquivos modificados | 84. `git diff --stat`: 7 860 inserções e 7 860 remoções; `git diff --ignore-cr-at-eol -w`: **vazio** → só fim de linha |
| Arquivos não versionados | 231 na primeira execução do roteiro, **0** depois da segunda execução do gcovr **[I]**: eram `.gcov` temporários deixados pela primeira execução, que abortou |

**[R]** Normalizar os fins de linha (por exemplo, um `.gitattributes` com `* text=auto eol=lf`) antes da migração, para os diffs das iterações não virem poluídos. Isso entra na DEC-012.

## 3. Build

**No WSL (oficial)** **[F]**:

```bash
cmake -S . -B /tmp/vmm_p03b/build -DCMAKE_BUILD_TYPE=Debug -DVMM_BUILD_TESTS=ON \
  -DVMM_BUILD_EXAMPLES=OFF -DVMM_BUILD_PAPER=OFF -DVMM_BUILD_DOCS=OFF \
  -DCMAKE_CXX_FLAGS=--coverage -DCMAKE_EXE_LINKER_FLAGS=--coverage
cmake --build /tmp/vmm_p03b/build -j4
```

| Item | Resultado |
|---|---|
| Configuração | ok (código de saída 0). Único aviso: o "CGAL performance notice", sobre `CMAKE_BUILD_TYPE=Debug` (esperado) |
| Opções detectadas (v2.1) | Native architecture ON; ASan/UBSan OFF; LTO/IPO ON; Fast math OFF; prefixo de instalação `/usr/local` |
| Compilação | código de saída 0; **nenhuma linha** no filtro `grep -E "error\|warning"` (v2.1 e v2.2) |
| Efeito colateral | nenhum arquivo versionado alterado pelo build (exemplos e paper desligados) |

**Na nuvem (indicativo)** **[F]**:
- A configuração **falha** em `VerifyCGALHeaders.cmake:48` quando há uma única instalação do CGAL (5.6.1, CMake 3.28): o try_compile não enxerga o alvo `CGAL::CGAL`. Com a verificação desativada, compila.
- Com exemplos ligados, os executáveis de exemplos e do paper são gravados **na árvore de fontes** (`examples/CMakeLists.txt:125`, `paper/CMakeLists.txt:76`): 18 arquivos versionados sobrescritos.
- Os testes vão para o diretório de build (`tests/CMakeLists.txt:191`).

**[I]** A verificação de headers funciona no ambiente do João (duas instalações), mas quebra num ambiente de CI com uma instalação só. Precisa ser tornada portável (DEC-012).

## 4. Testes

| Métrica | WSL v2.2 (**oficial**) | WSL v2.1 | Nuvem |
|---|---|---|---|
| Total | **249** | 249 | 249 |
| PASS | **249** | 249 | 249 |
| FAIL | **0** | 0 | 0 |
| SKIP / DISABLED | 0 | 0 (4 `GTEST_SKIP()` condicionais em `ut_Macros.cpp` não acionados; nenhum `DISABLED_`) | 0 |
| Tempo | 35,8 s (Debug com cobertura, `ctest -j4`) | 35,7 s | 8,9 s (Release) |

**[F] Qualidade dos testes:**
- 18 dos 249 `TEST` não têm `EXPECT_`/`ASSERT_` no corpo; a maioria delega a helpers que verificam.
- **Só executam código, sem verificar nada:** 4 testes "Perf_Smoke" (`CoreErrors.Perf_Smoke` em dois arquivos, `CoreErrors.Perf_Smoke_LookupAndRender`, `VMMException.Perf_Smoke_CreateAndWhat`).
- **Verificação só em tempo de compilação:** 2 testes (`Severity.IsEnumType`, `Language.IsEnumAndHasTwoLocales`).
- **Casos patológicos:** nenhum teste nomeia explicitamente casos degenerados. Grades cartesianas, cocirculares por natureza, aparecem em `ut_Release`.

**[F] GMock:** `ut_Config.cpp:23` inclui `gmock/gmock.h` e usa `MOCK_METHOD` (linha 327) e `EXPECT_CALL` (linha 335). Mas o CMake só liga o GMock `if(TARGET GTest::gmock)` (`tests/CMakeLists.txt:181`). Na nuvem, sem o GMock instalado, 3 alvos não compilaram.

## 5. Cobertura

| Métrica | WSL v2.2 (**oficial**) | WSL v2.1 | Nuvem |
|---|---|---|---|
| Linhas | **94,9 %** (3 471 de 3 657) | 94 % (3 300 de 3 482) | 94,9 % (3 479 de 3 667) |
| Ramos | **81,9 %** (2 467 de 3 013) | NÃO VERIFICADO | 80,9 % |
| Funções | **99,6 %** (495 de 497) | NÃO VERIFICADO | — |
| Arquivos instrumentados | **65** | NÃO VERIFICADO | 62 |

- **Comando no WSL (v2.2):** `gcovr -r . --object-directory /tmp/vmm_p03b/build --filter 'VMMLib/' --exclude-throw-branches --gcov-ignore-parse-errors=negative_hits.warn_once_per_file --gcov-ignore-errors=no_working_dir_found --print-summary --csv`. A opção de contagens negativas é necessária no GCC 15.
- **[I]** A diferença de denominador entre v2.1 e v2.2 (3 482 → 3 657 linhas) vem do método: a v2.2 aponta o `--object-directory` do build, o que passa a incluir o `.cpp` da biblioteca (157 linhas) e mais alguns headers. O HEAD é o mesmo.
- **Cobertura por arquivo:** `P03_cobertura_por_arquivo.csv` (WSL, 65 arquivos, linhas, ramos e funções).
- **Pontos fracos por arquivo [F]:**
  - `Sites2D/Site2D.hpp`: 0 de 6 ramos;
  - `Voronoi2D/Diagram/ClippedVoronoiDiagram2D.hpp`: 50 % dos ramos;
  - `Boundary2D/Shapes/RoundedRect.hpp`: 58 % dos ramos;
  - `Boundary2D/Boundary2DData.hpp`: 59 % dos ramos;
  - `ErrorHandling/IErrorLogger.h`: 1 de 2 funções (interface virtual; viola o R3);
  - `Voronoi2D/Metrics/VoronoiEdgeLengthDiagnostics2D.hpp`: 80 % das linhas, 65 % dos ramos.
- **Correção em relação à v2.1:** o `.cpp` da biblioteca (`src/VTK_XML_ClippedVoronoi2D.cpp`) **é** instrumentado: 157 linhas, 98 % cobertas.
- **A cobertura real é menor que a medida:**
  1. templates só contam o que foi instanciado nos testes;
  2. arquivos que nenhum teste inclui ficam fora do total.

## 6. Inventário resumido (`P03_inventario.csv`)

**[F]** 140 arquivos, 13 360 linhas, **115 tipos** (`class`/`struct`, sem `enum class`). A contagem foi confirmada por duas expressões regulares diferentes.

| Indicador | Valor |
|---|---|
| Com `ut_<Classe>.cpp` próprio | **6**: Rectangle, Ring2D, Config, Status, Site2D, SiteSet |
| Citados em algum teste / nunca citados | 71 / **44** |
| Estado | 85 ativos; 26 ativos só dentro do próprio arquivo; **4 mortos** |
| Classificação (v2.1) | **50 APROVEITAR**, **61 REFATORAR**, **4 DESCARTAR** |

Correções no CSV em relação à v2:
- `VolumeNumberingState2D`: APROVEITAR → **REFATORAR**, porque usa o enum de despacho `VolumeNumberingMethod2D`;
- 11 tipos sem nenhum teste que os cite deixaram de ser justificados como "testados";
- a justificativa de `BoundaryShortEdgeCollapseResult2D` foi corrigida.

Por módulo (tipos citados em teste × não citados): Voronoi2D 29 × 21; Boundary2D 14 × 4; Sites2D 11 × 4; IO 10 × 10; ErrorHandling 7 × 4; src 0 × 1.

## 7. Duplicações e código morto

| Item | Achado **[F]** |
|---|---|
| Headers de encaminhamento | **32** (27 na raiz de `include/VoronoiMeshMaker/` e 5 em `Boundary2D/`), com um `#include` cada. Não são cópias de código, são caminhos públicos duplicados |
| Arquivos vazios (placeholders) | **25**, só com comentários: vários de `Boundary2D/` (incluindo `Runtime/AnyBoundary2D`, `Policies/*` exceto `PolygonizePolicy`, `CGAL/*` e `Traits/*`), `Core/groups.h`, `IO/Concepts`, `IO/ExportTags`, `IO/Registry`, dois escritores `VTK_XML_*`, e `src/Boundary2D/Boundary2D.cpp` |
| Não alcançados pelo build | 51 de 140 (arquivos `.d` do compilador): os 32 encaminhamentos, os vazios e 5 com código (`IO/Boundary2D/Writers/Vtk.hpp`, `VTK_Legacy_Polys.hpp`, `IO/Boundary2D/Topology/TriangulatePolicy.hpp`, `ErrorHandling/FileErrors.h`, `ErrorHandling/ErrorHandling.h`) |
| Tipos mortos | 4: `FileErrorInfo`, `TriangulatedView`, `VTK_Legacy_PolysWriter`, `VtkBoundaryWriter` |
| Implementações antigas compiladas | nenhuma. `examples/Shape_legacy_unused/` e `examples/Parameters/` não são compilados, mas os executáveis deles continuam versionados |
| Binários versionados | 55 executáveis ELF e um `.pyc`; os arquivos versionados somam cerca de 167 MB |
| Esqueleto `VoronoiGridMaker/` | só placeholders; destino decidido no P06 |

## 8. Violações arquiteturais

| Regra | Achado **[F]** |
|---|---|
| R3: virtual/herança | `IErrorLogger` virtual, com `ThreadLocalBufferLogger : public IErrorLogger`; `VMMException : public std::exception` (política no P05) |
| Enums de despacho fechado | `ErrorConfig::Policy`; `IO::Dialect/Topology/Encoding` (`VtkOptions`); `BoundaryShortEdgePolicy2D`; `VolumeNumberingMethod2D` (`ClippedVoronoiDiagram2D`, `VolumeNumberingState2D`); `AdjacencyGraph2D` (`VoronoiBandwidth2D`). Enums de dado (`Severity`, `Language`, `CoreErr`, `FileErr`, `LoopKind`, `SiteValidationError`, `Ring2D::Orientation`, `SegmentEndpointOrigin2D`) são aceitáveis |
| Ordem de includes | 1 arquivo fora da ordem dos grupos (`src/VTK_XML_ClippedVoronoi2D.cpp`); 18 fora da ordem alfabética |
| DEC-009 | `find_package(TBB REQUIRED)`; `<tbb/parallel_for.h>` em header público (`ClippedVoronoiBuilder2D.hpp`); GMP por padrão |
| DEC-008 | `cmake/VerifyCGALHeaders.cmake` com `SPDX: GPL-3.0-or-later`; ainda não há separação `vmm_core` × `vmm_backend_cgal` |
| Consistência de erros | 113 `VMM_THROW` e 15 `throw std::runtime_error` direto, que contornam o catálogo de erros |
| Flags | `VMM_ENABLE_NATIVE_ARCH=ON` e `VMM_ENABLE_LTO=ON` por padrão, confirmados no WSL. Há uma opção `-ffast-math` (OFF) |

### Fronteira backend → modelo VMM (DEC-006/007)

**[F]** **93 de 137 headers públicos incluem o CGAL transitivamente.** O caminho é `Boundary2DTypes.hpp:28` → `Core/constants.h` → `Core/type.h` → `<CGAL/…>` (teste `g++ -fsyntax-only -H`, header a header).

Pontos de vazamento:
- **`Core/type.h`:**
  - `Point2D = Kernel::Point_2` (linha 66) e `Polygon2D = CGAL::Polygon_2<Kernel>` (linha 80);
  - `Real = vmm::gtp::Kernel::FT` e `Int = vmm::gtp::Kernel::RT` (linhas 115–116). No EPICK, o `FT` é `double` na representação, mas **a definição depende do CGAL**, e esse `Real` é o de todo o código (`Boundary2DTypes.hpp:41`, `Site2D.hpp:50`).
- **`DelaunayBuilder2D.hpp`:** devolve a triangulação do CGAL em header público.
- **`ClippedVoronoiDiagram2D`:** **guarda a triangulação do CGAL no objeto de saída** (`ClippedVoronoiDiagram2D.hpp:158`, `DelaunayTriangulation2D delaunay{}`). É o vazamento mais grave, porque atinge o objeto que o solver consumiria.
- `CgalKernelTraits2D.hpp` público; o IO usa tipos do CGAL (`VTK_Legacy_Delaunay2D.hpp`); o CDT do CGAL aparece em `TriangulatePolicy.hpp`, que não é alcançado pelo build.

**[F] O que já serve à fronteira:**
- o **layout** do `Point2` (`Boundary2DTypes.hpp`) é próprio e POD;
- `Site2D` reserva `weight`;
- `DelaunayNeighborProvider2D` e `DelaunaySiteIndex` isolam a consulta de vizinhos (embrião do `neighbor_pairs`).

⚠ **Correção da v2:** o `Point2` **não** é independente do CGAL. O header e o tipo escalar dependem dele.

### Tolerâncias absolutas, sem escala

**[F]** `kEpsilon = 1e-6` (`Core/constants.h`, absoluto) é usado em:
- `Halfplane2D.hpp:45,51` (tolerância padrão dos testes de lado);
- `BisectorHalfplane.hpp:39` (degenerescência `norm2 <= kEpsilon²`);
- `BoundaryConditionPoint2D.hpp:84` (`64·kEpsilon`);
- `VoronoiCell2D.hpp:251`.

A conectividade usa uma tolerância com escala (`VoronoiFaceConnectivity2D.hpp:107`, 64·eps·escala).

**[I]** Em domínios em milímetros, ou em coordenadas UTM (~10⁶ m), as tolerâncias absolutas mudam o comportamento. Isso é relevante para os casos ambientais. **[R]** Uma política de tolerâncias com escala explícita, a decidir no P05.

## 9. Avaliação dos módulos geométricos

**Experimento de ordem de inserção [F] (nuvem):**
- **Programa:** `P03_experimento_ordem_insercao.cpp`, fora do repositório. Monta o diagrama com os sítios na ordem original e em 5 permutações, e compara célula a célula pela identidade do sítio (caixa 4 × 3).
- **Topologia:** **0 polígonos diferentes** (arredondados a 1e-9) e **0 conjuntos de vizinhos diferentes**. Casos: aleatório com 2 000 sítios (em paralelo e em série), cartesiano 40 × 30 (cocircular) e hexagonal (470).
- **Geometria:** diferença máxima de área de **1,1e-16**, então a malha **não é idêntica bit a bit** entre ordens de inserção.
- **Reprodutibilidade:** a mesma entrada rodada duas vezes dá resultado bit a bit idêntico.
- **Limite:** um só domínio e três padrões de sítios. Não cobre sítios quase coincidentes, densidade muito variável nem regiões estreitas.

| Módulo | Robustez | Degenerescências | Determinismo | Testes | Acoplamento ao CGAL | Reuso |
|---|---|---|---|---|---|---|
| Primitivas | `double`; tolerâncias absolutas (ver §8) | — | ok | indiretos | **total** via `type.h` | layout do `Point2` sim; `type.h` refatorar |
| Delaunay | predicados exatos (EPICK) | cocircularidade tratada sem depender da ordem (experimento) | topologia independente da ordem | `ut_Delaunay` | total e exposto | alto, atrás do backend |
| Voronoi / células | recorte em `long double`, sem predicado exato | **só domínio convexo de um anel** (`VoronoiCellBuilder2D.hpp:110–131`) | ~1e-16 entre ordens | `ut_Cells`, `ut_Release` | via `Real`/`type.h` | médio: base da célula convexa; recorte do domínio a reescrever |
| Clipping | Sutherland–Hodgman, teste `<= 0`; colapso de arestas curtas por política | sem caso quase degenerado nomeado | ok | `ut_Clipping` (7 citados, 8 não) | via `Real` | alto (semiplanos) |
| CVT/Lloyd | tolerância 1e-10; 20 iterações por padrão | — | ok | `ut_CVT` | via `Real` | alto |
| Domínios | SoA POD; validação de orientação | `Ring2D` e `Polygon` existem, mas não podem ser malhados | ok | bons | via `Real` | alto; falta meio × região |
| Sítios | hash espacial na validação | — | **não portável** (`SiteFactory.hpp:932–934`, `std::uniform_real_distribution`), incompatível com o P10 | bons | via `Real` | alto (tipos); `SiteFactory` refatorar |
| Topologia | casamento **geométrico** com tolerância com escala e verificação de reciprocidade | **rejeita** faces menores que a resolução (`runtime_error`) | ok | `ut_Diagram`, `ut_Release` | nenhum direto | médio: trocar por chave combinatória (i, j) |
| IO | precisão configurável | — | ok | parciais | parcial | médio |

## 10. Comparação das três estratégias

| Critério | **A.** Evoluir a VMMLib no lugar | **B.** Migrar para o esqueleto VoronoiGridMaker | **C.** Estrutura nova + migração seletiva |
|---|---|---|---|
| Risco | médio | **alto** | médio |
| Volume de refatoração | alto e disfarçado: o CGAL está em 93 de 137 headers e no tipo `Real` | alto, mais a adaptação de um esqueleto anterior aos requisitos | alto, mas explícito, por módulo |
| Risco de regressão | baixo, com testes verdes a cada passo | alto (sem código nem testes) | baixo, se os testes migrarem junto e a VMMLib servir de oráculo |
| Facilidade de testes | boa (249) | ruim | boa |
| Compatibilidade com P01/P02 | baixa no início (DEC-006/007/009 contrariadas) | média | **alta** (nasce com `vmm_core` sem CGAL) |
| Preserva o código validado | sim | só por cópia manual | sim (os tipos APROVEITAR migram com os seus testes) |
| Dívida técnica herdada | alta (encaminhamentos, placeholders, macros de namespace, nomes `…2D`, binários versionados) | baixa no código, alta no desenho | baixa |

**O argumento mais forte a favor de A [I]** (pedido pela revisão externa): A preserva **o comportamento não documentado** da VMMLib (heurísticas de colapso de arestas, metadados de contorno, ordem de numeração) sem risco de omissão numa migração, e minimiza o trabalho de mover arquivos. C neutraliza esse argumento com o oráculo de regressão (as mesmas entradas precisam produzir a mesma malha, dentro de uma tolerância declarada). Isso só funciona se os casos do oráculo cobrirem esses comportamentos, o que exige casos-referência escolhidos de propósito.

## 11. Recomendação

**[R] Estratégia C**, com três salvaguardas:

1. **Oráculo operacional.** A VMMLib continua compilando, com os 249 testes verdes, até a migração terminar. As malhas dela em casos de referência (os de `ut_Release` e os problemas-âncora do P04) são gravadas como golden files, com a tolerância declarada. A estrutura nova precisa reproduzi-las.
2. **Critério de retirada.** A VMMLib é removida quando todos os casos do oráculo forem reproduzidos na estrutura nova e todos os tipos migrados tiverem teste por classe (R1).
3. **A restrição de domínio convexo não é herdada.** O mapa de migração do P06 marca `VoronoiCellBuilder2D::validate_domain` como pré-condição a **substituir**, não a migrar. Os P09/P10 exigem domínio não convexo, buracos e multirregião.

**Por que C e não A:**
- o CGAL está no tipo do núcleo (93 de 137 headers) e no objeto de saída;
- a DEC-007 exige alvos separados;
- o recorte (só convexo) e a topologia (casamento geométrico) precisam ser substituídos;
- 51 de 140 arquivos nem são alcançados pelo build.

B é descartada: o esqueleto antecede os requisitos atuais.

## 12. Riscos e incertezas

| # | Risco / incerteza | Mitigação |
|---|---|---|
| 1 | Várias instalações do CGAL e do Boost convivem (CGAL em `/usr/local` e 6.1.1 em `/usr`; Boost 1.86, 1.90 e 1.92); a evidência da v2.2 veio de um build interno do CGAL | só versões estáveis, versão mínima no CMake e versões registradas em cada execução (DEC-012); o CMake já registra o CGAL escolhido; remover instalações antigas quando possível |
| 2 | A verificação de headers do CGAL quebra em ambientes com uma instalação só (CI) | tornar portável no P07 (DEC-012) |
| 3 | A cobertura real é menor que a medida (templates não instanciados, arquivos não incluídos) | medir linhas, ramos e funções na CI (P07); testes por classe na migração (R1) |
| 4 | Tolerâncias absolutas e casamento de faces por tolerância falham em escalas extremas | política de tolerâncias com escala (P05); chave combinatória (i, j) no Mesh2D (P06/P11) |
| 5 | Determinismo entre plataformas (distribuição aleatória) e entre ordens (~1e-16) | gerador portável próprio (P10); ordem canônica de vizinhos antes do recorte |
| 6 | Fins de linha mistos (CRLF/LF) poluem os diffs | `.gitattributes` e normalização antes da migração (DEC-012) |
| 7 | A estratégia C mantém duas árvores durante a migração | critério de retirada (§11) |

---

## Entradas em `planning/DECISIONS.md`

Este diagnóstico propôs a DEC-011 (estratégia C) e a DEC-012 (higiene do repositório e do build). O João aprovou as duas com ajustes:
- **DEC-011:** os invariantes geométricos passam a ser o critério principal; o oráculo da VMMLib é segunda checagem, gerado num build limpo e com as versões registradas.
- **DEC-012:** a versão do CGAL não é fixada; valem a versão mínima no CMake, as versões testadas na CI e as versões usadas registradas, só estáveis.

O texto aprovado está em `planning/DECISIONS.md`.

## Resumo

- **Evidência oficial no WSL (g++ 15.2, CMake 4.2.3, CGAL 6.2.1-I-900, Boost 1.92):** configura, compila sem avisos, **249 de 249 testes passam**; cobertura de 94,9 % das linhas, 81,9 % dos ramos e 99,6 % das funções. O CGAL foi depois trocado pelo release 6.2.1.
- **Árvore do João = commit `7402178`**, exceto fins de linha. A análise estática vale para ela.
- **Inventário:** 115 tipos, só 6 com teste próprio, 44 sem teste. Classificação: 50 APROVEITAR, 61 REFATORAR, 4 DESCARTAR.
- **Ruído no repositório:** 51 arquivos que o build não alcança; 55 binários versionados.
- **CGAL em toda parte:** 93 de 137 headers públicos incluem o CGAL; `Real` vem do CGAL; o objeto de saída guarda a triangulação.
- **Geometria:** recorte só convexo; faces casadas por tolerância; tolerâncias absolutas; malha idêntica bit a bit só para a mesma ordem de entrada.
- **Recomendação:** estratégia C, com oráculo operacional, critério de retirada e a restrição convexa não herdada. Aprovada como DEC-011, com os invariantes geométricos como critério principal.
