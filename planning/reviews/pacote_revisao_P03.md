# Pacote de revisão do P03 — VoronoiMeshMaker (VMM)

Conteúdo, na ordem de leitura do prompt REV: AGENTS.md, DECISIONS.md, P03 (relatório), P03 (inventário CSV), experimento de ordem de inserção e trechos de código citados (commit 7402178, com números de linha). P01 e P02 seguem em arquivos separados.

---
## 1. AGENTS.md (raiz do repositório, commit 7402178)
```text
# Workspace directives

## Required environment

- Use only the WSL distribution `Ubuntu-26.04-Test` for this project.
- The active repository is `/home/jflavio/Programas/VMM` in that distribution.
- Always specify `wsl -d Ubuntu-26.04-Test` explicitly when executing from Windows.
- Do not access or execute commands in any other WSL distribution, including
  `Ubuntu-20.04` and `Ubuntu-22.04`.
- Do not substitute `VoronoiMeshMaker_desativado` for this repository.
- Results, builds and changes from another repository or distribution are not
  validation evidence for this working tree.

## Existing user policies

- Preserve existing uncommitted changes. The user makes commits; do not commit or push.
- Do not introduce inheritance, enums or equivalent closed feature dispatch.
  Prefer composition, traits and open factories/registries.
- Group includes in this order: C++ standard library, external libraries, VMM.
  Sort each group alphabetically and use the requested separator comments.
- Test public class functions with GTest, then test multi-class integration.
  Only after the testing gates should new examples be added to the manual.
- Do not claim that tests passed, coverage is complete or a dependency problem
  is resolved without evidence from the active environment and source tree.
```
---
## 2. planning/DECISIONS.md

# Registro de decisões — VMM

Formato e regras: ver `sequencia-prompts-vmm.md`, seção "Registro de decisões".

## DEC-001 — Nicho do VMM
- Data: 2026-09-28
- Origem: P01
- Status: APROVADA
- Decisão: O VMM é uma biblioteca C++ moderna que produz malhas Voronoi/PEBI prontas para solvers de volumes
  finitos, com sítios controlados pelo usuário, topologia, métricas geométricas, regiões, interfaces e patches
  (nicho N1). Produto: domínio + sítios → modelo de malha de VF (Mesh2D/Mesh3D). Não é um pipeline GIS completo.
- Justificativa: nenhuma das ferramentas analisadas reúne essas características (P01 §3); alinhada à competência
  do autor; entrega 2D viável.
- Consequências: P04 foca em casos-âncora como exemplos e em escritores de formato como plugins; P02 avalia o
  backend como policy substituível.

## DEC-002 — Backend 3D como policy substituível
- Data: 2026-09-28
- Origem: P01
- Status: APROVADA
- Decisão: O backend geométrico 3D fica obrigatoriamente atrás de um concept/policy substituível. A API pública e
  o modelo de dados do VMM não se acoplam a CGAL, Geogram ou qualquer outro backend; o solver só vê o modelo de
  malha do VMM. O P02 compara CGAL × Geogram e investiga o VoroCrust (inicialmente como referência e benchmark).
- Justificativa: há concorrentes fortes no algoritmo 3D; o valor do VMM está no modelo de VF e na API.
- Consequências: P02 ganha a comparação técnica CGAL × Geogram para Voronoi recortado 3D.

## DEC-003 — Sequência v3
- Data: 2026-09-28
- Origem: P01 §7
- Status: APROVADA
- Decisão: P02 inclui a comparação CGAL × Geogram e a verificação do VoroCrust; uma prova de conceito do
  recorte 3D (P15a) precede a arquitetura 3D.
- Justificativa: reduzir o risco técnico do 3D antes de fixar a arquitetura.
- Consequências: sequência de prompts passa à versão 3.

## DEC-004 — Minimizar dependências externas
- Data: 2026-09-28
- Origem: pedido do João durante o P02
- Status: APROVADA
- Decisão: O VMM deve depender do menor número possível de bibliotecas externas. Cada dependência precisa de
  justificativa explícita; dependências de conveniência (logging, configuração, GIS, formatos) são opcionais ou
  substituídas por código próprio pequeno.
- Justificativa: facilidade de instalação e manutenção para usuários externos; menor superfície de licença.
- Consequências: o P02 recomenda no máximo um backend geométrico obrigatório; a lista de dependências das
  diretrizes (CGAL, GMP+MPFR, Boost, GTest, spdlog, yaml-cpp) é revista no P02/P05.

## DEC-005 — Sem Geogram
- Data: 2026-09-28
- Origem: P02 (decisão do João)
- Status: APROVADA
- Decisão: O Geogram não será usado, nem como backend nem como dependência opcional.
- Justificativa: DEC-004 — evitar uma segunda biblioteca geométrica quando o CGAL já foi selecionado como backend.
  (TetGen e Triangle embutidos no Geogram podem ser desligados por opções de build; não são o motivo.)
- Consequências: CGAL é o único backend geométrico; VoroCrust (executável) é só referência externa no P15a.

## DEC-006 — Divisão de responsabilidades VMM × backend
- Data: 2026-09-28
- Origem: P02 §5
- Status: APROVADA
- Decisão: O backend responde às operações geométricas especializadas (triangulação → pares vizinhos, predicados
  exatos, consultas ao domínio, recorte exato de células de contorno 3D, IO/reparo de superfícies). O VMM constrói,
  possui e expõe a malha de volumes finitos. Nenhum tipo do CGAL aparece no modelo de dados do VMM.
- Justificativa: concentra no VMM o seu diferencial; delega o que exige robustez numérica especializada.
- Consequências: contratos precisos do backend definidos no P06.

## DEC-007 — Firewall de compilação; backend como policy interna
- Data: 2026-09-28
- Origem: P02 §5.3–5.4
- Status: APROVADA
- Decisão: GeometryBackend é uma policy interna de implementação, não um parâmetro da API do usuário; a API pública
  expõe só tipos do VMM (o concept pode virar ponto de extensão no futuro, só com tipos do VMM). Alvos CMake
  vmm_core (sem CGAL) e vmm_backend_cgal; headers do CGAL só em src/Backend/CGAL/*.cpp; instanciação explícita
  para D = 2, 3; CI compila os headers públicos sem o CGAL e um exemplo mínimo com vmm_core + backend de teste.
- Justificativa: DEC-002; impedir contaminação da API e do modelo de dados.
- Consequências: P06 define os tipos de fronteira; P07 cria os alvos e os testes de CI.

## DEC-008 — Licença: vmm_core × vmm_backend_cgal
- Data: 2026-09-28
- Origem: P02 §6
- Status: APROVADA
- Decisão: vmm_core sob BSD-3-Clause, sem dependência do CGAL. vmm_backend_cgal usa pacotes GPL do CGAL e fica
  sujeito às obrigações dessas licenças quando distribuído com a licença open source do CGAL; aplicações ou
  distribuições combinadas devem cumprir as obrigações GPL aplicáveis. Documentação clara (README "Licensing",
  aviso no CMake, SPDX, DCO), sem apresentar isso como aconselhamento jurídico.
- Justificativa: preservar a possibilidade de um backend permissivo futuro sem relicenciar o core.
- Consequências: P05 e P07 aplicam cabeçalhos, avisos e a divisão de alvos.

## DEC-009 — Dependências
- Data: 2026-09-28
- Origem: P02 §7
- Status: APROVADA
- Decisão: Obrigatórias: CGAL (headers) e Boost (headers). Multiprecisão padrão: Boost.Multiprecision.
  Opcionais: GMP, MPFR, TBB. GoogleTest só para testes. spdlog e yaml-cpp fora do núcleo.
- Justificativa: DEC-004.
- Consequências: P05 atualiza project_guidelines.tex; P08 cria a fachada de logging própria.

## DEC-010 — Sequência v4
- Data: 2026-09-28
- Origem: P02 §9
- Status: APROVADA
- Decisão: P15a usa o CGAL; VoroCrust só como referência externa quando possível; o P15a testa células convexas por
  semiespaços + recorte só das células necessárias, nos casos: domínio convexo, não convexo, arestas vivas, células
  de fronteira complexas, múltiplos componentes, quase degenerados, determinismo quanto à ordem de inserção.
  A arquitetura 3D definitiva só é decidida depois do P15a.
- Justificativa: validar experimentalmente a hipótese central do backend 3D antes de fixar a arquitetura.
- Consequências: sequência de prompts passa à versão 4.

## DEC-011 — Estratégia de base de código: estrutura nova + migração seletiva
- Data: 2026-09-28
- Origem: P03 v2 §10–11
- Status: PROPOSTA
- Decisão: A nova arquitetura nasce numa estrutura nova (vmm_core sem CGAL + vmm_backend_cgal, conforme o P06), para
  a qual os componentes da VMMLib migram seletivamente, módulo a módulo, com os seus testes. A VMMLib continua
  compilando, com os 249 testes verdes, como oráculo de regressão até ser retirada.
- Justificativa: o CGAL vaza para o tipo do núcleo (Core/type.h) e para o modelo de saída (ClippedVoronoiDiagram2D
  guarda a triangulação); a DEC-007 exige alvos separados; o recorte só convexo e a topologia por proximidade
  geométrica precisam ser substituídos; 51 de 140 arquivos não são alcançados pelo build.
- Consequências: P06 define a estrutura nova, o mapa de migração e o critério de retirada da VMMLib; P08–P12 migram
  por iterações.

## DEC-012 — Higiene do repositório e do build
- Data: 2026-09-28
- Origem: P03 v2 §3, §7, §8
- Status: PROPOSTA
- Decisão: remover do git os 55 binários e o .pyc versionados; gerar a saída de exemplos e paper fora da árvore de
  fontes; não migrar os 32 headers de encaminhamento nem os 25 placeholders; VMM_ENABLE_NATIVE_ARCH OFF por padrão;
  remover a opção -ffast-math; declarar o GMock como dependência dos testes; tornar portável (ou remover) a
  verificação de headers do CGAL; instrumentar também a biblioteca na cobertura.
- Justificativa: CI pública, reprodutibilidade, robustez geométrica e medição honesta de cobertura.
- Consequências: P07 executa estas mudanças.

---
## 3. planning/P03_diagnostico_vmmlib.md

# P03 — Diagnóstico da VMMLib (v2)

- **Data:** 2026-09-28
- **Sequência:** v4, prompt P03, reexecutado com a especificação detalhada do João
- **Destino no repositório:** `planning/P03_diagnostico_vmmlib.md` e `planning/P03_inventario.csv`
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação · **NÃO VERIFICADO** quando não foi possível obter.
- **Esta versão substitui a v1.** Correções em relação à v1 estão marcadas com ⚠.

> **Limitação estrutural.** O ambiente exigido (WSL `Ubuntu-26.04-Test`, `/home/jflavio/Programas/VMM`) **não está acessível** a esta sessão. O pedido de acesso foi recusado pela ferramenta com "UNC paths are not allowed: \\wsl.localhost\Ubuntu-26.04-Test\…".
>
> Todo o diagnóstico foi feito numa cópia do repositório remoto, no container da nuvem. Pelo AGENTS.md, build, testes e cobertura daqui **não são evidência de validação** da sua árvore de trabalho. A análise estática do código (inventário, violações, duplicações, módulos) vale para o commit analisado. Se houver alterações não commitadas no WSL, elas não estão refletidas.

---

## 1. Ambiente verificado

| Item | Valor | Como foi obtido |
|---|---|---|
| Ambiente exigido (WSL Ubuntu-26.04-Test) | **NÃO VERIFICADO** | sem acesso (ver limitação) |
| Ambiente usado | container Ubuntu 24.04 | — |
| g++ | 13.3.0 | `g++ --version` |
| clang++ | 18.1.3 | `clang++ --version` |
| CMake | 3.28.3 | `cmake --version` |
| Make / Ninja | GNU Make 4.3 / Ninja 1.11.1 | `make --version`, `ninja --version` |
| CGAL | **5.6.1** (`CGAL_VERSION_NR 1050601000`) | `/usr/include/CGAL/version.h` |
| Boost | 1.83 | `boost/version.hpp` (`BOOST_LIB_VERSION`) |
| Multiprecisão do CGAL | **GMP** (`CGAL_USE_GMP=TRUE`) | `CMakeCache.txt` |
| GMP / MPFR | 6.3.0 / 4.2.1 | `dpkg -l` |
| TBB | 2021.11 | `dpkg -l`; `TBB_DIR` no CMakeCache |
| GoogleTest / GMock | 1.14.0 | `dpkg -l` |
| gcovr | 8 | `pip` (instalado para esta análise) |

**[I]** O P02 analisou as licenças no CGAL 6.2.x. O CGAL da nuvem é o 5.6.1. Nenhum pacote usado pela VMMLib mudou de licença nesse intervalo, mas isso **não foi verificado** pacote a pacote na 5.6. **A versão do CGAL no seu WSL continua NÃO VERIFICADA.**

## 2. Estado do repositório

| Item | Valor |
|---|---|
| Remoto | `https://github.com/voronoimeshmaker/VoronoiMeshMaker` |
| Branch | `main` |
| HEAD | `740217839232d76e74b6b1e3d441e83e635ab487` ("Primeiro commit", 2026-09-28 07:32 −03:00) |
| Alterações não commitadas no WSL | **NÃO VERIFICADO** |
| Alterações feitas na cópia da nuvem | uma só, necessária para configurar: chamada `vmm_verify_cgal_headers()` comentada em `cmake/ConfigDependencies.cmake` (ver §3) |

## 3. Build

Instruções seguidas: as do `README.md` (Release, `BUILD_TESTS=ON`, `BUILD_EXAMPLES=ON`), em diretório novo fora da árvore (`../build_rel`).

```bash
cmake -S . -B ../build_rel -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
cmake --build ../build_rel -j2
```

| Item | Resultado |
|---|---|
| Configuração sem alteração | **Falha.** `cmake/VerifyCGALHeaders.cmake:48`: o `check_cxx_source_compiles` com `CMAKE_REQUIRED_LIBRARIES CGAL::CGAL` gera um try_compile em que o alvo `CGAL::CGAL` "was not found" |
| Configuração com a verificação desativada | ok |
| Opções detectadas | C++20; biblioteca compartilhada ON; exemplos ON (16 pastas); paper ON (1 programa); testes ON; docs OFF; warnings ON; `VMM_ENABLE_LTO=ON`; `VMM_ENABLE_NATIVE_ARCH=ON`; `VMM_ENABLE_FAST_MATH=OFF`; yaml-cpp não encontrado (exemplos YAML pulados) |
| Compilação | ok, **0 avisos** com as flags de aviso do projeto |
| Tempo | cerca de 240 s para configurar e compilar com `-j2` (medido com `date`); com `-j8`, o `cc1plus` foi morto por falta de memória |
| Dependência de teste | sem o GMock instalado, 3 alvos (`ut_ErrorHandling_Config`, `_CoreErrors`, `_Exception`) não compilam: usam `gmock/gmock.h`, mas o CMake só liga o GMock `if(TARGET GTest::gmock)` |
| Efeito colateral do build | **18 arquivos versionados modificados**: os executáveis de exemplos e do paper são gerados dentro da árvore de fontes (`RUNTIME_OUTPUT_DIRECTORY "${dir_abs}"` em `examples/CMakeLists.txt:125` e `paper/CMakeLists.txt:76`) |

⚠ **Correção da v1:** os binários de **teste** são gerados no diretório de build (`${CMAKE_CURRENT_BINARY_DIR}/…`, `tests/CMakeLists.txt:191`), não na árvore de fontes. Os executáveis de teste versionados no git são sobras de builds antigos. Só **exemplos e paper** escrevem na árvore de fontes.

**[I] Atenção para o seu WSL:** rodar o build com exemplos ativados **modifica arquivos versionados**, o que conflita com "preservar alterações não commitadas". O roteiro da §12 desliga exemplos e paper por isso.

## 4. Testes

`ctest -j2` no build Release:

| Métrica | Valor |
|---|---|
| Total | **249** |
| PASS | **249** |
| FAIL | 0 |
| SKIP/DISABLED | 0 em execução. Há 4 `GTEST_SKIP()` condicionais em `ut_Macros.cpp` (se as macros não estiverem definidas), mas nenhum foi acionado: o binário reporta "4 tests PASSED". Não há `DISABLED_`. |
| Tempo | 8,9 s (Release); 82 s (Debug com cobertura) |

**[F] Qualidade dos testes.** Dos 249 `TEST`, 18 não têm `EXPECT_`/`ASSERT_` no próprio corpo. A maioria delega a helpers que verificam (`expect_near`, `invariant`, `nearest_owner`, `check_order`, `expect_shape_path`). Os que **só executam código, sem verificar comportamento**, são 4 testes de "Perf_Smoke":
- `CoreErrors.Perf_Smoke` (em `ut_CoreErrors.cpp` e em `ErrorHandling/CoreErrors`);
- `CoreErrors.Perf_Smoke_LookupAndRender`;
- `VMMException.Perf_Smoke_CreateAndWhat`.

Outros 2 (`Severity.IsEnumType`, `Language.IsEnumAndHasTwoLocales`) verificam só em tempo de compilação (`static_assert`).

**[F] Casos patológicos.** Nenhum teste nomeia explicitamente casos degenerados, colineares ou cocirculares. Grades cartesianas, que são cocirculares por natureza, aparecem em `ut_Release` (`T1T2Cartesian`, `T4T5Cartesian`).

## 5. Cobertura

Build Debug com instrumentação (alvos de teste), gcovr com `--exclude-throw-branches`:

| Métrica | Valor |
|---|---|
| Arquivos instrumentados | 62 de 140 |
| Linhas | **94,9 %** (3 479 de 3 667) |
| Ramos | **80,9 %** |

Cobertura por arquivo: coluna `cobertura` do CSV (e `cov_files.csv`, se útil). Quando um arquivo tem vários tipos, a coluna diz "arquivo compartilhado (N tipos)". Não se inventou cobertura por classe.

Menores coberturas de linha: `VoronoiEdgeLengthDiagnostics2D` (80,0 %; ramos 64,7 %), `RoundedRect` (80,7 %; ramos 57,7 %), `Site2D` (83,3 %), `VoronoiBandwidth2D` (88,0 %), `Boundary2DDistance` e `DelaunayBuilder2D` (88,2 %).

Os números são otimistas, por três motivos:
1. Templates header-only só contam o que foi instanciado nos testes.
2. O `.cpp` da biblioteca (`src/VTK_XML_ClippedVoronoi2D.cpp`, 203 linhas) não é instrumentado, porque a flag de cobertura só vai para os alvos de teste.
3. Os arquivos não incluídos por nenhum teste não entram no total.

## 6. Inventário resumido (`P03_inventario.csv`)

**[F]** 140 arquivos, 13 360 linhas, **115 tipos** (`class`/`struct`, sem contar `enum class`).

Colunas do CSV: nome, tipo, namespace, header, source, módulo, responsabilidade (`@brief`), dependências externas e VMM, teste específico, testes que citam, cobertura, virtual, herança, enum de despacho usado, duplicata (header de encaminhamento), estado (ativo / interno ao arquivo / morto), classificação e justificativa.

| Indicador | Valor |
|---|---|
| Com teste próprio `ut_<Classe>.cpp` | **6**: Rectangle, Ring2D, Config, Status, Site2D, SiteSet |
| Citados em algum teste | 71 |
| Nunca citados em teste | **44** |
| Estado | 85 ativos; 26 ativos só dentro do próprio arquivo (tipos auxiliares); **4 mortos** |
| Classificação preliminar | **51 APROVEITAR**, **60 REFATORAR**, **4 DESCARTAR** |

Por módulo (tipos citados em teste × não citados): Voronoi2D 29 × 21; Boundary2D 14 × 4; Sites2D 11 × 4; IO 10 × 10; ErrorHandling 7 × 4; src 0 × 1.

## 7. Duplicações e código morto

| Item | Achado **[F]** |
|---|---|
| **Headers de encaminhamento** | **32** arquivos (27 na raiz de `include/VoronoiMeshMaker/` e 5 em `Boundary2D/`), cada um com um único `#include` do header real. ⚠ **Correção de uma afirmação anterior:** não são cópias de código, são caminhos públicos duplicados |
| **Arquivos vazios (placeholders)** | **25** arquivos só com comentários, entre eles: `Boundary2D/Boundary2D.hpp`, `Boundary2DBVH`, `Boundary2DBooleanOps`, `Boundary2DClip`, `Boundary2DRegistry`, `Boundary2DTags`, `Boundary2DViews`, os 3 de `Boundary2D/CGAL/`, os 4 de `Policies/` exceto `PolygonizePolicy`, `Runtime/AnyBoundary2D`, `Runtime/FromConfig`, `Traits/CGALTraits`, `Traits/GeometryTraits`, `Core/groups.h`, `IO/Concepts`, `IO/ExportTags`, `IO/Registry`, `IO/…/VTK_XML_PolyLines`, `VTK_XML_Polys`, e `src/Boundary2D/Boundary2D.cpp`. ⚠ **Correção de uma afirmação anterior:** eu citei `AnyBoundary2D`/type erasure como existente, mas é um arquivo vazio |
| **Arquivos não alcançados pelo build** | 51 de 140 não são incluídos por nenhuma unidade compilada (biblioteca, testes, exemplos, paper; levantado pelos arquivos `.d` do compilador). São os 32 encaminhamentos, os vazios e 5 com código: `IO/Boundary2D/Writers/Vtk.hpp` (99 linhas), `VTK_Legacy_Polys.hpp` (56), `IO/Boundary2D/Topology/TriangulatePolicy.hpp` (39), `ErrorHandling/FileErrors.h` (71), `ErrorHandling/ErrorHandling.h` (10) |
| **Tipos mortos** | 4: `FileErrorInfo`, `TriangulatedView`, `VTK_Legacy_PolysWriter`, `VtkBoundaryWriter` |
| **Implementações antigas ainda compiladas** | nenhuma encontrada. `examples/Shape_legacy_unused/` e `examples/Parameters/` **não** são detectados pelo CMake, mas os executáveis deles continuam versionados (`Ex_Shape2D`, `Ex_GeometricData`) |
| **Binários versionados** | 55 executáveis ELF, mais `tests/Voronoi2D/Release/__pycache__/verify_xml.cpython-314.pyc`; os arquivos versionados somam cerca de 167 MB |
| **Esqueleto `VoronoiGridMaker/`** | só placeholders; destino decidido no P06 |

## 8. Violações arquiteturais

| Regra | Achado **[F]** |
|---|---|
| **R3: virtual/herança** | `IErrorLogger` (virtual), com `ThreadLocalBufferLogger : public IErrorLogger`; `VMMException : public std::exception` (política no P05) |
| **Enum de despacho fechado** (AGENTS.md) | usados em: `ErrorConfig` (`Policy`), `VtkOptions` (`Dialect`, `Topology`, `Encoding`), `BoundaryShortEdgeCollapseOptions2D` (`BoundaryShortEdgePolicy2D`), `ClippedVoronoiDiagram2D` e `VolumeNumberingState2D` (`VolumeNumberingMethod2D`), `VoronoiBandwidth2D` (`AdjacencyGraph2D`). Enums de dado (`Severity`, `Language`, `CoreErr`, `FileErr`, `LoopKind`, `SiteValidationError`, `Ring2D::Orientation`, `SegmentEndpointOrigin2D`) são aceitáveis |
| **Ordem de includes** (AGENTS.md) | 1 arquivo fora da ordem dos grupos (`src/VTK_XML_ClippedVoronoi2D.cpp`); 18 fora da ordem alfabética |
| **DEC-009 (dependências)** | `find_package(TBB REQUIRED)`; `<tbb/parallel_for.h>` incluído em header público (`ClippedVoronoiBuilder2D.hpp`); GMP usado por padrão (a DEC-009 pede Boost.Multiprecision por padrão) |
| **DEC-008 (licença)** | `cmake/VerifyCGALHeaders.cmake` declara `SPDX: GPL-3.0-or-later`; ainda não há separação `vmm_core` × `vmm_backend_cgal` |
| **Consistência de erros** | 113 usos de `VMM_THROW` e 15 de `throw std::runtime_error` direto (por exemplo, em `VoronoiFaceConnectivity2D`), que contornam o catálogo de erros |
| **Flags** | `VMM_ENABLE_NATIVE_ARCH=ON` por padrão: binários não portáveis, e resultados podem variar entre máquinas; existe a opção `-ffast-math` (OFF), incompatível com predicados robustos |

### Tipos do CGAL que atravessam a futura fronteira backend → modelo VMM (DEC-006/007)

| Ponto de vazamento | Detalhe **[F]** |
|---|---|
| `Core/type.h` | `Point2D = Kernel::Point_2`, `Polygon2D = CGAL::Polygon_2<Kernel>`, `Real = Kernel::FT`, `Int = Kernel::RT`: tipos do núcleo definidos pelo CGAL |
| `Voronoi2D/Delaunay/DelaunayBuilder2D.hpp` | `DelaunayTriangulation2D = CgalKernelTraits2D::DelaunayTriangulation`, devolvida em header público |
| **`ClippedVoronoiDiagram2D`** | **guarda a triangulação do CGAL no próprio modelo de saída** (`DelaunayTriangulation2D delaunay{}`). É o vazamento mais grave, porque atinge o objeto que o solver consumiria |
| `Voronoi2D/Traits/CgalKernelTraits2D.hpp` | traits do CGAL em header público |
| `IO/Voronoi2D/Writers/VTK_Legacy_Delaunay2D.hpp` | usa tipos do CGAL no IO |
| `IO/Boundary2D/Topology/TriangulatePolicy.hpp` | CDT do CGAL no IO (arquivo não alcançado) |

**Elementos já reutilizáveis para a fronteira [F]:**
- `Point2` POD próprio (`Boundary2DTypes.hpp`, trivially copyable, standard layout), usado por `Site2D` e pelas células;
- `Site2D` já reserva `weight` (diagrama de potência);
- as células guardam polígonos em `Point2`, não em tipos do CGAL;
- `DelaunayNeighborProvider2D` e `DelaunaySiteIndex` já isolam a consulta de vizinhos, que é o embrião do `neighbor_pairs` da DEC-006.

## 9. Avaliação dos módulos geométricos

**Experimento de ordem de inserção [F]:**
- **Programa:** um programa fora do repositório (`exp/perm.cpp`) monta o diagrama com os sítios em ordem original e em 5 permutações, e compara célula por célula (pela identidade do sítio). Caixa 4 × 3.
- **Topologia:** em todos os casos, **0 células** com polígono diferente (arredondado a 1e-9) e **0 conjuntos de vizinhos** de Delaunay diferentes. Casos testados: aleatório com 2 000 sítios (em paralelo e em série), grade cartesiana 40 × 30 (cocircular) e hexagonal (470 sítios).
- **Geometria:** a diferença máxima de área foi de **1,1e-16**. Portanto a malha **não é idêntica bit a bit** entre ordens de inserção, porque a ordem de recorte dos semiplanos segue a ordem dos vizinhos.
- **Reprodutibilidade:** a mesma entrada rodada duas vezes (em paralelo) dá resultado bit a bit idêntico.

| Módulo | Robustez numérica | Degenerescências | Determinismo | Testes | Acoplamento ao CGAL | Reuso (P01/P02) |
|---|---|---|---|---|---|---|
| **Primitivas** (`Point2`, tipos, `constants`) | `double`; tolerâncias globais fixas (`kZeroTol = 1e-12`, `kEpsilon = 1e-6`) sem escala | — | ok | indiretos | `type.h` acoplado; `Point2` não | **alto** (`Point2`); `type.h` refatorar |
| **Delaunay** | predicados exatos do CGAL (EPICK) | cocircularidade resolvida sem depender da ordem (experimento) | topologia independente da ordem **[F]** | `ut_Delaunay` | **total**, e exposto | **alto**, atrás do backend |
| **Voronoi / células** (`VoronoiCellBuilder2D`, `BisectorHalfplane`) | recorte em ponto flutuante `long double`, sem predicado exato | **só domínio convexo de um anel**: `NotImplemented` para buracos ou vários anéis, `InvalidArgument` para não convexo | bit a bit só para a mesma ordem; ~1e-16 entre ordens | `ut_Cells`, `ut_Release` | baixo (usa `Point2`) | **médio**: base da célula convexa; recorte do domínio a reescrever |
| **Clipping** (`HalfplaneClipper2D`, `ShortEdgeCollapse`, `BoundaryCellDetector`) | Sutherland–Hodgman com teste `<= 0` em `long double`; colapso de arestas curtas por política | sem teste nomeado de casos quase degenerados | ok | `ut_Clipping` (7 tipos citados, 8 não) | nenhum | **alto** para semiplanos; o detector de contorno precisa ser adaptado a não convexo e multirregião |
| **CVT/Lloyd** | tolerância 1e-10, máximo de 20 iterações por padrão | — | ok | `ut_CVT` (1 tipo citado, 2 não) | nenhum | **alto** |
| **Domínios** (Boundary2D) | SoA POD; validação de orientação e finitude | anéis com buracos existem (`Ring2D`), mas não podem ser malhados | ok | bons (formas, builder, queries, transforms, validation) | nenhum nos tipos | **alto**; falta meio × região |
| **Sítios** (Sites2D) | validação de distância mínima com hash espacial | — | **não portável entre bibliotecas padrão** (`std::uniform_real_distribution`, `SiteFactory.hpp:932–934`) | bons | nenhum | **alto** (tipos); `SiteFactory` refatorar |
| **Topologia** (`VoronoiFaceConnectivity2D`) | faces casadas por **proximidade geométrica** (buckets 3 × 3, tolerância 64·eps·escala), com verificação de reciprocidade | **rejeita** faces internas menores que a resolução (`runtime_error`) | ok | `ut_Diagram`, `ut_Release` | nenhum | **médio**: o modelo Mesh2D deveria casar faces pela chave combinatória (i, j) vinda do Delaunay, que dispensa tolerância |
| **IO** (VTK legacy/XML) | precisão configurável | — | ok | parciais (`Vtk.hpp`, `VTK_Legacy_Polys` sem teste e não alcançados) | parcial (Delaunay no IO) | **médio**: consolidar e separar visualização de persistência |

## 10. Comparação das três estratégias

| Critério | **A.** Evoluir a VMMLib no lugar | **B.** Migrar para o esqueleto VoronoiGridMaker | **C.** Estrutura nova + migração seletiva |
|---|---|---|---|
| Risco geral | médio | **alto** | médio |
| Volume de refatoração | **alto e disfarçado**: quase todo módulo muda de lugar ou de tipo (CGAL fora do núcleo, `vmm_core`/`vmm_backend_cgal`, Mesh2D, genérico em dimensão) | alto, mais o custo de adaptar um esqueleto que antecede os requisitos | alto, mas **explícito** e por módulo |
| Risco de regressão | baixo se os testes ficarem verdes a cada passo | alto: o esqueleto não tem testes nem código | baixo, **se** os testes migrarem junto e a VMMLib servir de oráculo |
| Facilidade de testes | boa: 249 testes existentes | ruim | boa: cada componente migra com os seus testes e é conferido contra a VMMLib |
| Compatibilidade com P01/P02 | baixa no início: `type.h`, o diagrama com a triangulação do CGAL e o TBB em header público contradizem DEC-006/007/009 até serem refeitos | média: nomes de módulos úteis, mas sem Mesh, região ou fronteira de backend | **alta**: a estrutura nasce com `vmm_core` sem CGAL e o backend separado |
| Preserva o código geométrico validado | sim | só por cópia manual | sim, pelos 51 tipos APROVEITAR, migrados com os testes |
| Dívida técnica herdada | **alta**: 32 encaminhamentos, 25 placeholders, namespaces por macro, nomes `…2D` fixos, binários versionados | baixa no código, alta no desenho | **baixa**: só entra o que é migrado de propósito |

## 11. Recomendação

**[R] Estratégia C**, com uma salvaguarda da A:
- **Estrutura nova** conforme o P06, com alvos `vmm_core` (sem CGAL) e `vmm_backend_cgal`.
- **Migração seletiva, módulo a módulo.** Entram os 51 tipos APROVEITAR e os 60 REFATORAR, cada um com os seus testes, já reorganizados por classe (R1/R2). Os 4 mortos, os 25 placeholders e os 32 encaminhamentos não migram.
- **A VMMLib continua compilando e com os 249 testes verdes até o fim da migração.** Ela é o oráculo de regressão: as mesmas entradas precisam produzir a mesma malha (até a tolerância acordada) na estrutura nova.

**Por que as evidências levam a C e não a A** (⚠ esta recomendação muda a da v1, que era A):
1. O vazamento do CGAL está no **objeto de saída** (`ClippedVoronoiDiagram2D` guarda a triangulação) e no **tipo do núcleo** (`type.h`), não só na periferia. Removê-lo toca todos os módulos.
2. A DEC-007 exige alvos separados e headers públicos sem CGAL. Isso é uma mudança de estrutura, não uma refatoração local.
3. O recorte do domínio (só convexo) e a topologia (casamento geométrico) precisam ser **substituídos**, não estendidos.
4. Um terço dos arquivos (51 de 140) nem é alcançado pelo build, e 25 são vazios: evoluir no lugar arrasta esse ruído.
5. O custo que A evitaria (perder os testes) é neutralizado em C mantendo a VMMLib como oráculo.

A estratégia B é descartada: o esqueleto antecede os requisitos atuais e não tem código nem testes.

## 12. Riscos e incertezas

| # | Risco / incerteza | Mitigação |
|---|---|---|
| 1 | **Nada disto foi verificado no seu WSL** (versão do CGAL, alterações não commitadas, testes) | rodar o roteiro abaixo e anexar a saída como `planning/P03_evidencia_wsl.txt` |
| 2 | A verificação de headers do CGAL pode passar no WSL e falhar na CI pública | tratar na P07 |
| 3 | A cobertura real é menor que 94,9 % (templates não instanciados, `.cpp` não instrumentado) | instrumentar a biblioteca na P07 |
| 4 | Casar faces por tolerância pode falhar com faces muito curtas ou escalas grandes (coordenadas UTM) | chave combinatória (i, j) no Mesh2D (P06/P11) |
| 5 | Determinismo entre plataformas (distribuição aleatória) e entre ordens (~1e-16) | gerador portável próprio (P10); ordem canônica de vizinhos antes do recorte |
| 6 | A estratégia C exige manter duas árvores durante a migração | prazo e critério de remoção da VMMLib definidos no P06 |

**Roteiro para evidência no WSL.** Rodar no `Ubuntu-26.04-Test`. Ele não altera arquivos versionados: build em `/tmp`, sem exemplos e sem paper.

```bash
cd /home/jflavio/Programas/VMM
{ git status --short; git branch --show-current; git rev-parse HEAD
  g++ --version | head -1; clang++ --version 2>/dev/null | head -1; cmake --version | head -1
  grep -h "CGAL_VERSION_NR\|CGAL_VERSION_STR" $(find /usr /opt $HOME -path '*/CGAL/version.h' 2>/dev/null | head -3)
  grep -m1 BOOST_LIB_VERSION /usr/include/boost/version.hpp
  dpkg -l | grep -E 'libgmp-dev|libmpfr-dev|libtbb-dev|libgmock-dev|libgtest-dev|libcgal-dev' | awk '{print $2,$3}'
  rm -rf /tmp/vmm_p03 && cmake -S . -B /tmp/vmm_p03 -DCMAKE_BUILD_TYPE=Debug -DVMM_BUILD_TESTS=ON \
    -DVMM_BUILD_EXAMPLES=OFF -DVMM_BUILD_PAPER=OFF -DVMM_BUILD_DOCS=OFF 2>&1 | tail -25
  grep -E "CGAL_USE_GMP|TBB_DIR" /tmp/vmm_p03/CMakeCache.txt
  cmake --build /tmp/vmm_p03 -j4 2>&1 | grep -E "error|warning" | sort | uniq -c | head -40
  (cd /tmp/vmm_p03 && ctest -j4 2>&1 | tail -20)
  command -v gcovr && gcovr -r . --filter 'VMMLib/' --exclude-throw-branches /tmp/vmm_p03 | tail -5
  git status --short | wc -l; } > /tmp/P03_evidencia_wsl.txt 2>&1
```

Depois, copie `/tmp/P03_evidencia_wsl.txt` para `planning/`, ou mande a saída aqui.

---

## Entradas acrescentadas a `planning/DECISIONS.md` (PROPOSTA)

- **DEC-011 — Estratégia C:** estrutura nova e migração seletiva, com a VMMLib como oráculo de regressão até ser retirada.
- **DEC-012 — Higiene do repositório e do build:**
  - remover do git os binários versionados e o `.pyc`;
  - gerar a saída de exemplos e paper fora da árvore de fontes;
  - não migrar os encaminhamentos nem os placeholders;
  - `VMM_ENABLE_NATIVE_ARCH` OFF por padrão;
  - remover a opção `-ffast-math`;
  - declarar o GMock como dependência dos testes;
  - tornar portável (ou remover) a verificação de headers do CGAL;
  - instrumentar a biblioteca para a cobertura.

## Resumo

- **Onde rodou:** o WSL exigido é inacessível daqui. Tudo foi feito numa cópia do commit `7402178`, na nuvem (CGAL 5.6.1, GMP, TBB).
- **Build:** falha na verificação de headers do CGAL fora do seu ambiente; com ela desativada, compila sem avisos. Com exemplos ativados, 18 arquivos versionados são sobrescritos.
- **Testes:** 249 de 249 passam. 4 testes só executam código ("Perf_Smoke"), e nenhum nomeia casos degenerados.
- **Cobertura:** 94,9 % das linhas e 80,9 % dos ramos, em 62 de 140 arquivos (otimista).
- **Inventário:** 115 tipos. Só 6 com teste próprio e 44 sem teste nenhum. 4 mortos; 51 APROVEITAR, 60 REFATORAR, 4 DESCARTAR.
- **Ruído no repositório:** 51 arquivos que nenhum build alcança (32 encaminhamentos, 25 vazios, 5 com código), 55 binários versionados.
- **Violações:** herança/virtual no ErrorHandling, 7 enums de despacho, TBB obrigatório, e o CGAL vazando para `type.h` e para o **modelo de saída** (`ClippedVoronoiDiagram2D` guarda a triangulação).
- **Geometria:** Delaunay robusto e independente da ordem; recorte só para domínio convexo; topologia por proximidade geométrica; malha idêntica bit a bit só para a mesma ordem de entrada.
- **Recomendação:** estratégia C, com a VMMLib como oráculo de regressão até ser retirada.

---
## 4. planning/P03_inventario.csv
```csv
nome,tipo,namespace,header,source,modulo,responsabilidade,dependencias_externas,dependencias_vmm,teste_especifico,testes_que_citam,cobertura,usa_virtual,heranca,enum_despacho_usado,duplicata,estado,classificacao,justificativa
Boundary2DData,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp,,Boundary2D,Canonical polygon-with-holes storage (SoA/CSR).,,Boundary2D ErrorHandling,não,ut_Boundary2DContains.cpp ut_Boundary2DDistance.cpp ut_Boundary2DShapes.cpp ut_Boundary2DTransform.cpp ut_Boundary2DValidation.cpp ut_Connectivity.cpp ut_Diagram.cpp ut_SiteValidation.cpp ut_VTK_Legacy.cpp,100.0% linhas / 59.1% ramos,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
RegionId,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp,,Boundary2D,Region identifier for non-homogeneous domains. */,,Core,não,ut_Boundary2DBuilder.cpp ut_Boundary2DShapes.cpp ut_Boundary2DTransform.cpp ut_Boundary2DTransformExport.cpp ut_Connectivity.cpp ut_Delaunay.cpp ut_Diagram.cpp ut_IO.cpp ut_Ring2D.cpp ut_Site2D.cpp ut_SiteFactory.cpp ut_SiteSet.cpp ut_SiteValidation.cpp ut_VTK_Legacy.cpp,arquivo compartilhado (5 tipos): 100.0% linhas,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
TagId,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp,,Boundary2D,Optional tag identifier (edge/ring/patch labelling). */,,Core,não,,arquivo compartilhado (5 tipos): 100.0% linhas,não,,,,ativo (interno ao arquivo),APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
Point2,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp,,Boundary2D,"2D point. Plain old data, aligned to 16 bytes to help vector loads.",,Core,não,ut_Boundary2DBuilder.cpp ut_Boundary2DContains.cpp ut_Boundary2DDistance.cpp ut_Boundary2DShapes.cpp ut_Boundary2DTransform.cpp ut_Boundary2DTransformExport.cpp ut_Boundary2DValidation.cpp ut_CVT.cpp ut_Cells.cpp ut_Clipping.cpp ut_Connectivity.cpp ut_Delaunay.cpp ut_Diagram.cpp ut_IO.cpp ut_Rectangle.cpp ut_Release.cpp ut_Ring2D.cpp ut_Site2D.cpp ut_SiteFactory.cpp ut_SiteSet.cpp ut_SiteValidation.cpp ut_VTK_Legacy.cpp,arquivo compartilhado (5 tipos): 100.0% linhas,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
Box2,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp,,Boundary2D,Axis-aligned bounding box.,,Core,não,,arquivo compartilhado (5 tipos): 100.0% linhas,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
Affine2,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp,,Boundary2D,Row-major 2x3 affine transform:,,Core,não,ut_Boundary2DTransform.cpp,arquivo compartilhado (5 tipos): 100.0% linhas,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
PolygonizePolicy,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Policies/PolygonizePolicy.hpp,,Boundary2D,Placeholder policy (extensível no futuro).,,Core,não,ut_Boundary2DBuilder.cpp ut_Boundary2DShapes.cpp ut_Boundary2DTransformExport.cpp ut_Delaunay.cpp ut_IO.cpp ut_Rectangle.cpp ut_Ring2D.cpp ut_SiteFactory.cpp ut_SiteValidation.cpp ut_VTK_Legacy.cpp,não instrumentado,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
BoundaryProjection2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Queries/Boundary2DDistance.hpp,,Boundary2D,,,Boundary2D,não,,88.2% linhas / 76.9% ramos,não,,,VMMLib/include/VoronoiMeshMaker/Boundary2D/Boundary2DDistance.hpp,ativo (interno ao arquivo),APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
Capsule,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Shapes/Capsule.hpp,,Boundary2D,Horizontal capsule polygonized as a CCW ring.,,Boundary2D Core ErrorHandling,não,ut_Boundary2DShapes.cpp,91.1% linhas / 68.4% ramos,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
Circle,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Shapes/Circle.hpp,,Boundary2D,Circular 2D shape polygonized as a counter-clockwise closed ring.,,Boundary2D Core ErrorHandling,não,ut_Boundary2DShapes.cpp,88.6% linhas / 69.2% ramos,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
Ellipse,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Shapes/Ellipse.hpp,,Boundary2D,Axis-aligned ellipse polygonized as a CCW ring.,,Boundary2D Core ErrorHandling,não,ut_Boundary2DShapes.cpp,88.6% linhas / 66.7% ramos,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
Polygon,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Shapes/Polygon.hpp,,Boundary2D,Explicit polygon shape represented by one CCW outer ring.,,Boundary2D Core ErrorHandling,não,ut_Boundary2DShapes.cpp,92.6% linhas / 80.0% ramos,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
Rectangle,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp,,Boundary2D,Axis-aligned rectangle polygonized as four CCW vertices.,,Boundary2D ErrorHandling,sim,ut_Boundary2DBuilder.cpp ut_Boundary2DShapes.cpp ut_CVT.cpp ut_Cells.cpp ut_Clipping.cpp ut_Connectivity.cpp ut_Delaunay.cpp ut_Diagram.cpp ut_IO.cpp ut_Rectangle.cpp ut_Release.cpp ut_SiteFactory.cpp ut_SiteValidation.cpp ut_VTK_Legacy.cpp,arquivo compartilhado (2 tipos): 92.9% linhas,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
CenteredTag,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp,,Boundary2D,Axis-aligned rectangle polygonized as four CCW vertices.,,Boundary2D ErrorHandling,não,,arquivo compartilhado (2 tipos): 92.9% linhas,não,,,,ativo (interno ao arquivo),APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
RegularNGon,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Shapes/RegularNGon.hpp,,Boundary2D,Regular polygon with `sides` equally spaced CCW vertices.,,Boundary2D Core ErrorHandling,não,ut_Boundary2DShapes.cpp,88.6% linhas / 69.2% ramos,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
Ring2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Shapes/Ring2D.hpp,,Boundary2D,Circular annulus represented by one outer loop and one hole loop.,,Boundary2D Core ErrorHandling,sim,ut_Boundary2DBuilder.cpp ut_Boundary2DShapes.cpp ut_Boundary2DTransformExport.cpp ut_Cells.cpp ut_Connectivity.cpp ut_Ring2D.cpp ut_SiteFactory.cpp ut_SiteValidation.cpp,97.4% linhas / 96.3% ramos,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
RoundedRect,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Shapes/RoundedRect.hpp,,Boundary2D,Rounded rectangle polygonized as a CCW outer ring.,,Boundary2D Core ErrorHandling,não,ut_Boundary2DShapes.cpp,80.7% linhas / 57.7% ramos,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
Triangle,struct,vmm,VMMLib/include/VoronoiMeshMaker/Boundary2D/Shapes/Triangle.hpp,,Boundary2D,Triangle shape stored as three counter-clockwise vertices.,,Boundary2D Core ErrorHandling,não,ut_Boundary2DShapes.cpp,95.5% linhas / 83.3% ramos,não,,,,ativo,APROVEITAR,modelo POD/SoA; falta meio × região e domínio não convexo no recorte
ErrorInfo,struct,vmm::error::detail,VMMLib/include/VoronoiMeshMaker/ErrorHandling/CoreErrors.h,,ErrorHandling,Core error domain (example). */,,ErrorHandling,não,,100.0% linhas / None% ramos,não,,,,ativo (interno ao arquivo),APROVEITAR,catálogo pt/en e códigos estáveis
ErrorConfig,struct,vmm::error,VMMLib/include/VoronoiMeshMaker/ErrorHandling/ErrorConfig.h,VMMLib/src/Boundary2D/ErrorHandling/ErrorConfig.cpp,ErrorHandling,Immutable configuration blob (shared). */,,ErrorHandling,não,ut_Connectivity.cpp ut_Exception.cpp ut_Integration.cpp ut_Language.cpp ut_Macros.cpp ut_Manager.cpp ut_MessageCatalog.cpp,arquivo compartilhado (2 tipos): 100.0% linhas,não,,Policy,,ativo,REFATORAR,enum Policy de despacho fechado
Config,class,vmm::error,VMMLib/include/VoronoiMeshMaker/ErrorHandling/ErrorConfig.h,VMMLib/src/Boundary2D/ErrorHandling/ErrorConfig.cpp,ErrorHandling,Global config handle (atomic shared_ptr swap).,,ErrorHandling,sim,ut_Config.cpp ut_Connectivity.cpp ut_Exception.cpp ut_Integration.cpp ut_Language.cpp ut_Macros.cpp ut_Manager.cpp ut_MessageCatalog.cpp,arquivo compartilhado (2 tipos): 100.0% linhas,não,,,,ativo,APROVEITAR,catálogo pt/en e códigos estáveis
ThreadLocalBufferLogger,class,vmm::error,VMMLib/include/VoronoiMeshMaker/ErrorHandling/ErrorManager.h,,ErrorHandling,A default logger implementation that stores error records in a,,ErrorHandling,não,,arquivo compartilhado (2 tipos): 91.7% linhas,não,: public IErrorLogger,,,ativo,REFATORAR,deriva de IErrorLogger (R3)
ErrorManager,class,vmm::error,VMMLib/include/VoronoiMeshMaker/ErrorHandling/ErrorManager.h,,ErrorHandling,Global facade for the error handling system.,,ErrorHandling,não,ut_Connectivity.cpp ut_Macros.cpp ut_Manager.cpp,arquivo compartilhado (2 tipos): 91.7% linhas,não,,,,ativo,APROVEITAR,catálogo pt/en e códigos estáveis
ErrorRecord,struct,vmm::error,VMMLib/include/VoronoiMeshMaker/ErrorHandling/ErrorRecord.h,,ErrorHandling,Holds all information about a single logged error event. */,,ErrorHandling,não,ut_Manager.cpp,não instrumentado,não,,,,ativo,APROVEITAR,catálogo pt/en e códigos estáveis
FileErrorInfo,struct,vmm::error::detail,VMMLib/include/VoronoiMeshMaker/ErrorHandling/FileErrors.h,,ErrorHandling,Static metadata for FileErr messages. */,,ErrorHandling,não,,não instrumentado,não,,,,morto (não alcançado pelo build),DESCARTAR,"não alcançado por nenhum alvo compilado (biblioteca, testes, exemplos, paper)"
IErrorLogger,class,vmm::error,VMMLib/include/VoronoiMeshMaker/ErrorHandling/IErrorLogger.h,,ErrorHandling,Interface for error logging destinations (sinks).,,ErrorHandling,não,,não instrumentado,sim,,,,ativo,REFATORAR,interface virtual (R3); trocar por callback
Status,class,vmm::error,VMMLib/include/VoronoiMeshMaker/ErrorHandling/Status.h,,ErrorHandling,Lightweight status types to avoid exceptions when desired.,,ErrorHandling,sim,ut_Status.cpp,arquivo compartilhado (2 tipos): 100.0% linhas,não,,,,ativo,APROVEITAR,catálogo pt/en e códigos estáveis
StatusOr,class,vmm::error,VMMLib/include/VoronoiMeshMaker/ErrorHandling/Status.h,,ErrorHandling,Lightweight status types to avoid exceptions when desired.,,ErrorHandling,não,ut_Status.cpp,arquivo compartilhado (2 tipos): 100.0% linhas,não,,,,ativo,APROVEITAR,catálogo pt/en e códigos estáveis
VMMException,class,vmm::error,VMMLib/include/VoronoiMeshMaker/ErrorHandling/VMMException.h,,ErrorHandling,"Minimal exception that stores code, severity and messages.",,ErrorHandling,não,ut_Boundary2DBuilder.cpp ut_Boundary2DShapes.cpp ut_CVT.cpp ut_Cells.cpp ut_Clipping.cpp ut_Connectivity.cpp ut_Delaunay.cpp ut_Exception.cpp ut_Macros.cpp ut_Rectangle.cpp ut_Ring2D.cpp ut_SiteFactory.cpp ut_SiteSet.cpp ut_SiteValidation.cpp ut_VTK_Legacy.cpp,100.0% linhas / 100.0% ramos,não,: public std::exception,,,ativo,REFATORAR,deriva de std::exception; política de exceções no P05
PolyLinesView,class,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Boundary2D/Topology/PolyLinesView.hpp,,IO,,,Boundary2D Core,não,,100.0% linhas / None% ramos,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
TriangulatedView,class,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Boundary2D/Topology/TriangulatePolicy.hpp,,IO,Fornece uma visão triangulada sobre um Boundary2DData para exportação como polígonos.,,Boundary2D Core,não,,não instrumentado,não,,,,morto (não alcançado pelo build),DESCARTAR,"não alcançado por nenhum alvo compilado (biblioteca, testes, exemplos, paper)"
VTK_Legacy_PolyLinesWriter,struct,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Boundary2D/Writers/VTK_Legacy_PolyLines.hpp,,IO,Functor writer: VTK Legacy (.vtk) POLYDATA with closed LINES (ASCII).,,Core ErrorHandling IO,não,ut_VTK_Legacy.cpp,93.3% linhas / 87.7% ramos,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
VTK_Legacy_PolysWriter,struct,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Boundary2D/Writers/VTK_Legacy_Polys.hpp,,IO,Functor que escreve uma fronteira triangulada no formato VTK Legacy (ASCII).,,Core ErrorHandling IO,não,,não instrumentado,não,,,,morto (não alcançado pelo build),DESCARTAR,"não alcançado por nenhum alvo compilado (biblioteca, testes, exemplos, paper)"
VtkBoundaryWriter,class,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Boundary2D/Writers/Vtk.hpp,,IO,Classe responsável por escrever dados Boundary2D no formato VTK.,,Boundary2D Core IO,não,,não instrumentado,não,,,,morto (não alcançado pelo build),DESCARTAR,"não alcançado por nenhum alvo compilado (biblioteca, testes, exemplos, paper)"
PrintLine,class,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/IOHelpers.hpp,,IO,Manipulador personalizado para imprimir linhas no stream de saída,,Core,não,ut_IOHelpers.cpp,arquivo compartilhado (3 tipos): 100.0% linhas,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
HeaderPrinter,class,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/IOHelpers.hpp,,IO,Classe para impressão de cabeçalhos formatados,,Core,não,ut_IOHelpers.cpp,arquivo compartilhado (3 tipos): 100.0% linhas,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
CommentPrinter,class,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/IOHelpers.hpp,,IO,Classe para impressão de textos formatados como comentários,,Core,não,ut_IOHelpers.cpp,arquivo compartilhado (3 tipos): 100.0% linhas,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
VtkOptions,struct,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Options.hpp,,IO,Agrega todas as opções de exportação para o formato VTK.,,Core,não,ut_Boundary2DShapes.cpp ut_Boundary2DTransformExport.cpp ut_Delaunay.cpp ut_Diagram.cpp ut_IO.cpp ut_VTK_Legacy.cpp,não instrumentado,não,,Dialect Encoding Topology,,ativo,REFATORAR,enums Dialect/Topology/Encoding de despacho
OStreamSink,struct,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Sinks.hpp,,IO,Sink que escreve em um `std::ostream` existente (não toma posse).,,Core ErrorHandling,não,,arquivo compartilhado (2 tipos): 88.9% linhas,não,,,,ativo (interno ao arquivo),REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
FileSink,class,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Sinks.hpp,,IO,Sink que gerencia um arquivo próprio (abre/fecha).,,Core ErrorHandling,não,ut_VTK_Legacy.cpp,arquivo compartilhado (2 tipos): 88.9% linhas,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
VTK_Legacy_SitesWithBoundaryWriter,struct,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Sites2D/Writers/VTK_Legacy_SitesWithBoundary.hpp,,IO,Functor writer: VTK Legacy POLYDATA with boundary lines and site points.,,Boundary2D Core ErrorHandling IO Sites2D,não,,96.5% linhas / 93.6% ramos,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
VtkVolumeScalarField,struct,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Voronoi2D/Writers/VTK_Legacy_ClippedVoronoi2D.hpp,,IO,"Writes Boundary2D, Site2D, Delaunay and Voronoi cells to POLYDATA.",CGAL,Boundary2D IO Voronoi2D,não,ut_Diagram.cpp,arquivo compartilhado (4 tipos): 94.8% linhas,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
VtkVector3D,struct,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Voronoi2D/Writers/VTK_Legacy_ClippedVoronoi2D.hpp,,IO,"Writes Boundary2D, Site2D, Delaunay and Voronoi cells to POLYDATA.",CGAL,Boundary2D IO Voronoi2D,não,ut_Diagram.cpp,arquivo compartilhado (4 tipos): 94.8% linhas,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
VtkVolumeVectorField,struct,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Voronoi2D/Writers/VTK_Legacy_ClippedVoronoi2D.hpp,,IO,"Writes Boundary2D, Site2D, Delaunay and Voronoi cells to POLYDATA.",CGAL,Boundary2D IO Voronoi2D,não,ut_Diagram.cpp,arquivo compartilhado (4 tipos): 94.8% linhas,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
VTK_Legacy_ClippedVoronoi2DWriter,struct,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Voronoi2D/Writers/VTK_Legacy_ClippedVoronoi2D.hpp,,IO,Functor writer: VTK Legacy POLYDATA for a complete 2D diagram.,CGAL,Boundary2D IO Voronoi2D,não,,arquivo compartilhado (4 tipos): 94.8% linhas,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
VTK_Legacy_Delaunay2DWriter,struct,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Voronoi2D/Writers/VTK_Legacy_Delaunay2D.hpp,,IO,"Functor writer: VTK Legacy POLYDATA with boundary, sites and Delaunay.",CGAL,Boundary2D Core ErrorHandling IO Sites2D Voronoi2D,não,,92.1% linhas / 89.8% ramos,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
VtkXmlClippedOptions2D,struct,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Voronoi2D/Writers/VTK_XML_ClippedVoronoi2D.hpp,VMMLib/src/VTK_XML_ClippedVoronoi2D.cpp,IO,"Shared-vertex, polygon-only XML unstructured grid export.",,Voronoi2D,não,,não instrumentado,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
VtkXmlClippedSummary2D,struct,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Voronoi2D/Writers/VTK_XML_ClippedVoronoi2D.hpp,VMMLib/src/VTK_XML_ClippedVoronoi2D.cpp,IO,"Shared-vertex, polygon-only XML unstructured grid export.",,Voronoi2D,não,,não instrumentado,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
VtkXmlCellDataArray2D,struct,vmm::io,VMMLib/include/VoronoiMeshMaker/IO/Voronoi2D/Writers/VTK_XML_ClippedVoronoi2D.hpp,VMMLib/src/VTK_XML_ClippedVoronoi2D.cpp,IO,"Shared-vertex, polygon-only XML unstructured grid export.",,Voronoi2D,não,ut_Release.cpp,não instrumentado,não,,,,ativo,REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
CartesianGrid2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp,,Sites2D,Deterministic Cartesian grid distribution over a Boundary2D box.,,Boundary2D ErrorHandling Sites2D,não,ut_Diagram.cpp ut_SiteFactory.cpp,arquivo compartilhado (7 tipos): 96.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/SiteFactory.hpp,ativo,REFATORAR,arquivo monolítico (1 053 linhas); distribuição aleatória não portável
CartesianGridCount2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp,,Sites2D,Deterministic Cartesian grid distribution over a Boundary2D box.,,Boundary2D ErrorHandling Sites2D,não,ut_CVT.cpp ut_Connectivity.cpp ut_Diagram.cpp ut_Release.cpp ut_SiteFactory.cpp,arquivo compartilhado (7 tipos): 96.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/SiteFactory.hpp,ativo,REFATORAR,arquivo monolítico (1 053 linhas); distribuição aleatória não portável
SiteGenerationBox2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp,,Sites2D,Axis-aligned generation box used by site patterns before clipping.,,Boundary2D ErrorHandling Sites2D,não,ut_SiteFactory.cpp,arquivo compartilhado (7 tipos): 96.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/SiteFactory.hpp,ativo,REFATORAR,arquivo monolítico (1 053 linhas); distribuição aleatória não portável
TriangularIIGrid2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp,,Sites2D,Triangular II pattern from the reference paper.,,Boundary2D ErrorHandling Sites2D,não,ut_SiteFactory.cpp,arquivo compartilhado (7 tipos): 96.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/SiteFactory.hpp,ativo,REFATORAR,arquivo monolítico (1 053 linhas); distribuição aleatória não portável
TriangularIVGrid2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp,,Sites2D,Triangular IV pattern from the reference paper.,,Boundary2D ErrorHandling Sites2D,não,ut_SiteFactory.cpp,arquivo compartilhado (7 tipos): 96.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/SiteFactory.hpp,ativo,REFATORAR,arquivo monolítico (1 053 linhas); distribuição aleatória não portável
HexagonalGrid2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp,,Sites2D,Staggered lattice that produces hexagonal Voronoi cells.,,Boundary2D ErrorHandling Sites2D,não,ut_Diagram.cpp ut_Release.cpp ut_SiteFactory.cpp,arquivo compartilhado (7 tipos): 96.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/SiteFactory.hpp,ativo,REFATORAR,arquivo monolítico (1 053 linhas); distribuição aleatória não portável
UniformRandom2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp,,Sites2D,Uniform random rejection sampler inside Boundary2D.,,Boundary2D ErrorHandling Sites2D,não,ut_Connectivity.cpp ut_Diagram.cpp ut_Release.cpp ut_SiteFactory.cpp,arquivo compartilhado (7 tipos): 96.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/SiteFactory.hpp,ativo,REFATORAR,arquivo monolítico (1 053 linhas); distribuição aleatória não portável
SiteId,struct,vmm,VMMLib/include/VoronoiMeshMaker/Sites2D/Site2D.hpp,,Sites2D,Strong identifier for a 2D Voronoi generator site.,,Boundary2D Core,não,ut_CVT.cpp ut_Cells.cpp ut_Clipping.cpp ut_Connectivity.cpp ut_Delaunay.cpp ut_Diagram.cpp ut_Site2D.cpp ut_SiteFactory.cpp ut_SiteSet.cpp ut_SiteValidation.cpp,arquivo compartilhado (2 tipos): 83.3% linhas,não,,,VMMLib/include/VoronoiMeshMaker/Site2D.hpp,ativo,APROVEITAR,tipos simples e testados; Site2D já reserva weight
Site2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Sites2D/Site2D.hpp,,Sites2D,A 2D Voronoi generator site.,,Boundary2D Core,sim,ut_Delaunay.cpp ut_IO.cpp ut_Site2D.cpp ut_SiteSet.cpp ut_SiteValidation.cpp,arquivo compartilhado (2 tipos): 83.3% linhas,não,,,VMMLib/include/VoronoiMeshMaker/Site2D.hpp,ativo,APROVEITAR,tipos simples e testados; Site2D já reserva weight
SiteSet,struct,vmm,VMMLib/include/VoronoiMeshMaker/Sites2D/SiteSet.hpp,,Sites2D,Contiguous collection of 2D Voronoi generator sites.,,ErrorHandling Sites2D,sim,ut_CVT.cpp ut_Cells.cpp ut_Clipping.cpp ut_Connectivity.cpp ut_Delaunay.cpp ut_Diagram.cpp ut_IO.cpp ut_SiteSet.cpp ut_SiteValidation.cpp,97.4% linhas / 80.0% ramos,não,,,VMMLib/include/VoronoiMeshMaker/SiteSet.hpp,ativo,APROVEITAR,tipos simples e testados; Site2D já reserva weight
SiteValidationOptions,struct,vmm,VMMLib/include/VoronoiMeshMaker/Sites2D/SiteValidation.hpp,,Sites2D,Options controlling validation of generator sites.,,Boundary2D ErrorHandling Sites2D,não,ut_CVT.cpp ut_Diagram.cpp ut_IO.cpp ut_SiteFactory.cpp ut_SiteValidation.cpp,arquivo compartilhado (5 tipos): 97.9% linhas,não,,,VMMLib/include/VoronoiMeshMaker/SiteValidation.hpp,ativo,APROVEITAR,tipos simples e testados; Site2D já reserva weight
SiteValidationReport,struct,vmm,VMMLib/include/VoronoiMeshMaker/Sites2D/SiteValidation.hpp,,Sites2D,Options controlling validation of generator sites.,,Boundary2D ErrorHandling Sites2D,não,,arquivo compartilhado (5 tipos): 97.9% linhas,não,,,VMMLib/include/VoronoiMeshMaker/SiteValidation.hpp,ativo (interno ao arquivo),APROVEITAR,tipos simples e testados; Site2D já reserva weight
SiteSpacingCell2D,struct,vmm::detail,VMMLib/include/VoronoiMeshMaker/Sites2D/SiteValidation.hpp,,Sites2D,,,Boundary2D ErrorHandling Sites2D,não,,arquivo compartilhado (5 tipos): 97.9% linhas,não,,,VMMLib/include/VoronoiMeshMaker/SiteValidation.hpp,ativo (interno ao arquivo),APROVEITAR,tipos simples e testados; Site2D já reserva weight
SiteSpacingCellHash2D,struct,vmm::detail,VMMLib/include/VoronoiMeshMaker/Sites2D/SiteValidation.hpp,,Sites2D,,,Boundary2D ErrorHandling Sites2D,não,,arquivo compartilhado (5 tipos): 97.9% linhas,não,,,VMMLib/include/VoronoiMeshMaker/SiteValidation.hpp,ativo (interno ao arquivo),APROVEITAR,tipos simples e testados; Site2D já reserva weight
SiteSpacingIndex2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Sites2D/SiteValidation.hpp,,Sites2D,Spatial index for minimum-distance checks between sites.,,Boundary2D ErrorHandling Sites2D,não,,arquivo compartilhado (5 tipos): 97.9% linhas,não,,,VMMLib/include/VoronoiMeshMaker/SiteValidation.hpp,ativo,APROVEITAR,tipos simples e testados; Site2D já reserva weight
LloydOptions2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/CVT/LloydOptimizer2D.hpp,,Voronoi2D,Options controlling Lloyd relaxation.,,Boundary2D ErrorHandling Sites2D Voronoi2D,não,ut_CVT.cpp ut_Release.cpp,arquivo compartilhado (3 tipos): 94.2% linhas,não,,,VMMLib/include/VoronoiMeshMaker/LloydOptimizer2D.hpp,ativo,APROVEITAR,funciona e é testado; adaptar ao novo modelo
LloydIterationStats2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/CVT/LloydOptimizer2D.hpp,,Voronoi2D,Diagnostic data for one Lloyd iteration.,,Boundary2D ErrorHandling Sites2D Voronoi2D,não,,arquivo compartilhado (3 tipos): 94.2% linhas,não,,,VMMLib/include/VoronoiMeshMaker/LloydOptimizer2D.hpp,ativo (interno ao arquivo),APROVEITAR,funciona e é testado; adaptar ao novo modelo
LloydResult2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/CVT/LloydOptimizer2D.hpp,,Voronoi2D,Result of a Lloyd relaxation run.,,Boundary2D ErrorHandling Sites2D Voronoi2D,não,,arquivo compartilhado (3 tipos): 94.2% linhas,não,,,VMMLib/include/VoronoiMeshMaker/LloydOptimizer2D.hpp,ativo (interno ao arquivo),APROVEITAR,funciona e é testado; adaptar ao novo modelo
BoundaryConditionPoint2D,class,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Cells/BoundaryConditionPoint2D.hpp,,Voronoi2D,Immutable boundary-condition geometry constructed from a support line.,,Boundary2D Core,não,ut_Connectivity.cpp,100.0% linhas / 87.5% ramos,não,,,,ativo,REFATORAR,"migrar para o modelo Mesh2D (owner/neighbour, CSR)"
AreaCentroid2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Cells/VoronoiCell2D.hpp,,Voronoi2D,Result of a single-pass signed-area and centroid computation.,,Boundary2D Core Sites2D Voronoi2D,não,,arquivo compartilhado (3 tipos): 98.5% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiCell2D.hpp,ativo (interno ao arquivo),REFATORAR,"migrar para o modelo Mesh2D (owner/neighbour, CSR)"
VoronoiCellEdge2D,class,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Cells/VoronoiCell2D.hpp,,Voronoi2D,Metadata for one edge of a clipped Voronoi cell.,,Boundary2D Core Sites2D Voronoi2D,não,ut_Connectivity.cpp ut_Diagram.cpp ut_Release.cpp,arquivo compartilhado (3 tipos): 98.5% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiCell2D.hpp,ativo,REFATORAR,"migrar para o modelo Mesh2D (owner/neighbour, CSR)"
VoronoiCell2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Cells/VoronoiCell2D.hpp,,Voronoi2D,One clipped Voronoi cell associated with a generator site.,,Boundary2D Core Sites2D Voronoi2D,não,ut_Cells.cpp ut_Connectivity.cpp ut_Diagram.cpp ut_Release.cpp,arquivo compartilhado (3 tipos): 98.5% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiCell2D.hpp,ativo,REFATORAR,"migrar para o modelo Mesh2D (owner/neighbour, CSR)"
VoronoiCellBuildOptions2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Cells/VoronoiCellBuilder2D.hpp,,Voronoi2D,Options controlling how one Voronoi cell is built.,,Boundary2D ErrorHandling Sites2D Voronoi2D,não,,arquivo compartilhado (2 tipos): 93.5% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiCellBuilder2D.hpp,ativo,REFATORAR,"migrar para o modelo Mesh2D (owner/neighbour, CSR)"
VoronoiCellBuilder2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Cells/VoronoiCellBuilder2D.hpp,,Voronoi2D,Options controlling how one Voronoi cell is built.,,Boundary2D ErrorHandling Sites2D Voronoi2D,não,ut_Cells.cpp ut_Clipping.cpp ut_Connectivity.cpp ut_Release.cpp,arquivo compartilhado (2 tipos): 93.5% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiCellBuilder2D.hpp,ativo,REFATORAR,só aceita domínio convexo de um anel; recorte do domínio a reescrever
BisectorHalfplane,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/BisectorHalfplane.hpp,,Voronoi2D,Construct the perpendicular-bisector halfplane for a site pair.,,ErrorHandling Sites2D Voronoi2D,não,ut_Clipping.cpp,100.0% linhas / 100.0% ramos,não,,,VMMLib/include/VoronoiMeshMaker/BisectorHalfplane.hpp,ativo,APROVEITAR,base da célula convexa por semiplanos (DEC-006)
BoundaryShortEdgeCollapseOptions2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/BoundaryShortEdgeCollapse2D.hpp,,Voronoi2D,Optional clean-up for very short Voronoi edges on the clipped boundary.,,Boundary2D Core ErrorHandling,não,ut_Clipping.cpp,arquivo compartilhado (2 tipos): 94.5% linhas,não,,BoundaryShortEdgePolicy2D,VMMLib/include/VoronoiMeshMaker/BoundaryShortEdgeCollapse2D.hpp,ativo,REFATORAR,enum de política de despacho; revisar para multirregião
BoundaryShortEdgeCollapseResult2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/BoundaryShortEdgeCollapse2D.hpp,,Voronoi2D,Optional clean-up for very short Voronoi edges on the clipped boundary.,,Boundary2D Core ErrorHandling,não,,arquivo compartilhado (2 tipos): 94.5% linhas,não,,,VMMLib/include/VoronoiMeshMaker/BoundaryShortEdgeCollapse2D.hpp,ativo (interno ao arquivo),REFATORAR,enum de política de despacho; revisar para multirregião
ClippingWorkspace2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/ClippingWorkspace2D.hpp,,Voronoi2D,Scratch buffers for repeated polygon clipping.,,Sites2D,não,ut_Clipping.cpp ut_Release.cpp,não instrumentado,não,,,VMMLib/include/VoronoiMeshMaker/ClippingWorkspace2D.hpp,ativo,APROVEITAR,base da célula convexa por semiplanos (DEC-006)
Halfplane2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/Halfplane2D.hpp,,Voronoi2D,Closed 2D halfplane represented by `a*x + b*y <= c`.,,Core Sites2D,não,ut_Clipping.cpp,100.0% linhas / 62.5% ramos,não,,,VMMLib/include/VoronoiMeshMaker/Halfplane2D.hpp,ativo,APROVEITAR,base da célula convexa por semiplanos (DEC-006)
HalfplaneClipper2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/HalfplaneClipper2D.hpp,,Voronoi2D,Clips a polygon by `a*x + b*y <= c`.,,Core ErrorHandling Voronoi2D,não,ut_Clipping.cpp,95.9% linhas / 78.6% ramos,não,,,VMMLib/include/VoronoiMeshMaker/HalfplaneClipper2D.hpp,ativo,APROVEITAR,base da célula convexa por semiplanos (DEC-006)
SegmentEndpointEvent2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/VoronoiBoundaryCellDetector2D.hpp,,Voronoi2D,Detects boundary Voronoi cells using the Yan et al. FIFO propagation.,,Boundary2D Core ErrorHandling Sites2D Voronoi2D,não,,arquivo compartilhado (9 tipos): 90.1% linhas,não,,,VMMLib/include/VoronoiMeshMaker/BoundaryCellDetector2D.hpp,ativo (interno ao arquivo),REFATORAR,lógica reaproveitável; adaptar a domínio não convexo e multirregião
BoundaryCellIncidentEdge2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/VoronoiBoundaryCellDetector2D.hpp,,Voronoi2D,Detects boundary Voronoi cells using the Yan et al. FIFO propagation.,,Boundary2D Core ErrorHandling Sites2D Voronoi2D,não,,arquivo compartilhado (9 tipos): 90.1% linhas,não,,,VMMLib/include/VoronoiMeshMaker/BoundaryCellDetector2D.hpp,ativo (interno ao arquivo),REFATORAR,lógica reaproveitável; adaptar a domínio não convexo e multirregião
BoundaryCellDetection2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/VoronoiBoundaryCellDetector2D.hpp,,Voronoi2D,,,Boundary2D Core ErrorHandling Sites2D Voronoi2D,não,,arquivo compartilhado (9 tipos): 90.1% linhas,não,,,VMMLib/include/VoronoiMeshMaker/BoundaryCellDetector2D.hpp,ativo (interno ao arquivo),REFATORAR,lógica reaproveitável; adaptar a domínio não convexo e multirregião
BoundaryCellDetector2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/VoronoiBoundaryCellDetector2D.hpp,,Voronoi2D,,,Boundary2D Core ErrorHandling Sites2D Voronoi2D,não,ut_Clipping.cpp,arquivo compartilhado (9 tipos): 90.1% linhas,não,,,VMMLib/include/VoronoiMeshMaker/BoundaryCellDetector2D.hpp,ativo,REFATORAR,lógica reaproveitável; adaptar a domínio não convexo e multirregião
Options,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/VoronoiBoundaryCellDetector2D.hpp,,Voronoi2D,Detects Voronoi cells that intersect Boundary2D edges.,,Boundary2D Core ErrorHandling Sites2D Voronoi2D,não,ut_VTK_Legacy.cpp,arquivo compartilhado (9 tipos): 90.1% linhas,não,,,VMMLib/include/VoronoiMeshMaker/BoundaryCellDetector2D.hpp,ativo,REFATORAR,lógica reaproveitável; adaptar a domínio não convexo e multirregião
BoundaryEdge,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/VoronoiBoundaryCellDetector2D.hpp,,Voronoi2D,,,Boundary2D Core ErrorHandling Sites2D Voronoi2D,não,,arquivo compartilhado (9 tipos): 90.1% linhas,não,,,VMMLib/include/VoronoiMeshMaker/BoundaryCellDetector2D.hpp,ativo (interno ao arquivo),REFATORAR,lógica reaproveitável; adaptar a domínio não convexo e multirregião
CellEdgePair,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/VoronoiBoundaryCellDetector2D.hpp,,Voronoi2D,,,Boundary2D Core ErrorHandling Sites2D Voronoi2D,não,,arquivo compartilhado (9 tipos): 90.1% linhas,não,,,VMMLib/include/VoronoiMeshMaker/BoundaryCellDetector2D.hpp,ativo (interno ao arquivo),REFATORAR,lógica reaproveitável; adaptar a domínio não convexo e multirregião
CellHalfplane,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/VoronoiBoundaryCellDetector2D.hpp,,Voronoi2D,,,Boundary2D Core ErrorHandling Sites2D Voronoi2D,não,,arquivo compartilhado (9 tipos): 90.1% linhas,não,,,VMMLib/include/VoronoiMeshMaker/BoundaryCellDetector2D.hpp,ativo (interno ao arquivo),REFATORAR,lógica reaproveitável; adaptar a domínio não convexo e multirregião
ClippedSegment,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/VoronoiBoundaryCellDetector2D.hpp,,Voronoi2D,,,Boundary2D Core ErrorHandling Sites2D Voronoi2D,não,,arquivo compartilhado (9 tipos): 90.1% linhas,não,,,VMMLib/include/VoronoiMeshMaker/BoundaryCellDetector2D.hpp,ativo (interno ao arquivo),REFATORAR,lógica reaproveitável; adaptar a domínio não convexo e multirregião
DelaunayBuildOptions2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayBuilder2D.hpp,,Voronoi2D,Options controlling Delaunay construction from SiteSet.,,ErrorHandling Sites2D Voronoi2D,não,ut_Delaunay.cpp,arquivo compartilhado (2 tipos): 88.2% linhas,não,,,VMMLib/include/VoronoiMeshMaker/DelaunayBuilder2D.hpp,ativo,REFATORAR,devolve triangulação do CGAL em header público; vai para o backend interno
DelaunayBuilder2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayBuilder2D.hpp,,Voronoi2D,Options controlling Delaunay construction from SiteSet.,,ErrorHandling Sites2D Voronoi2D,não,ut_Cells.cpp ut_Clipping.cpp ut_Delaunay.cpp,arquivo compartilhado (2 tipos): 88.2% linhas,não,,,VMMLib/include/VoronoiMeshMaker/DelaunayBuilder2D.hpp,ativo,REFATORAR,devolve triangulação do CGAL em header público; vai para o backend interno
DelaunayNeighborProvider2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayNeighborProvider2D.hpp,,Voronoi2D,Neighbor query helper for a Delaunay triangulation.,,Voronoi2D,não,ut_Delaunay.cpp,93.8% linhas / 94.4% ramos,não,,,VMMLib/include/VoronoiMeshMaker/DelaunayNeighborProvider2D.hpp,ativo,REFATORAR,devolve triangulação do CGAL em header público; vai para o backend interno
DelaunaySiteIndex,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunaySiteIndex.hpp,,Voronoi2D,Compact index from sequential SiteId values to CGAL vertex handles.,,ErrorHandling Voronoi2D,não,ut_Cells.cpp ut_Clipping.cpp ut_Delaunay.cpp ut_Release.cpp,90.3% linhas / 78.8% ramos,não,,,VMMLib/include/VoronoiMeshMaker/DelaunaySiteIndex.hpp,ativo,REFATORAR,devolve triangulação do CGAL em header público; vai para o backend interno
ClippedVoronoiBuildOptions2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiBuilder2D.hpp,,Voronoi2D,,tbb,Boundary2D ErrorHandling Sites2D Voronoi2D,não,ut_Connectivity.cpp ut_Diagram.cpp ut_Release.cpp,arquivo compartilhado (2 tipos): 97.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/ClippedVoronoiBuilder2D.hpp,ativo,REFATORAR,inclui TBB diretamente em header público; base do novo pipeline
ClippedVoronoiBuilder2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiBuilder2D.hpp,,Voronoi2D,,tbb,Boundary2D ErrorHandling Sites2D Voronoi2D,não,ut_CVT.cpp ut_Connectivity.cpp ut_Diagram.cpp ut_Release.cpp,arquivo compartilhado (2 tipos): 97.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/ClippedVoronoiBuilder2D.hpp,ativo,REFATORAR,inclui TBB diretamente em header público; base do novo pipeline
VolumeNumberingState2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiDiagram2D.hpp,,Voronoi2D,Tracks which renumbering method was last applied to the diagram.,,Boundary2D ErrorHandling Sites2D Voronoi2D,não,,arquivo compartilhado (2 tipos): 94.4% linhas,não,,VolumeNumberingMethod2D,VMMLib/include/VoronoiMeshMaker/ClippedVoronoiDiagram2D.hpp,ativo,APROVEITAR,transformações do diagrama; adaptar ao novo modelo
ClippedVoronoiDiagram2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiDiagram2D.hpp,,Voronoi2D,Complete clipped 2D Voronoi diagram for one Boundary2D / SiteSet pair.,,Boundary2D ErrorHandling Sites2D Voronoi2D,não,ut_Connectivity.cpp ut_Diagram.cpp ut_Release.cpp,arquivo compartilhado (2 tipos): 94.4% linhas,não,,VolumeNumberingMethod2D,VMMLib/include/VoronoiMeshMaker/ClippedVoronoiDiagram2D.hpp,ativo,REFATORAR,guarda a triangulação do CGAL no modelo; enum de numeração
VoronoiFaceLengthLimits2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Diagram/VoronoiFaceConnectivity2D.hpp,,Voronoi2D,Transactional pairing of physical Voronoi faces.,,Sites2D Voronoi2D,não,ut_Connectivity.cpp,arquivo compartilhado (5 tipos): 94.0% linhas,não,,,,ativo,REFATORAR,"faces casadas por proximidade geométrica com tolerância; preferir chave combinatória (i, j)"
VoronoiFaceConnectivity2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Diagram/VoronoiFaceConnectivity2D.hpp,,Voronoi2D,"Pair the final internal segments, independently of Delaunay links.",,Sites2D Voronoi2D,não,ut_Connectivity.cpp,arquivo compartilhado (5 tipos): 94.0% linhas,não,,,,ativo,REFATORAR,"faces casadas por proximidade geométrica com tolerância; preferir chave combinatória (i, j)"
Reference,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Diagram/VoronoiFaceConnectivity2D.hpp,,Voronoi2D,,,Sites2D Voronoi2D,não,,arquivo compartilhado (5 tipos): 94.0% linhas,não,,,,ativo,REFATORAR,"faces casadas por proximidade geométrica com tolerância; preferir chave combinatória (i, j)"
Bucket,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Diagram/VoronoiFaceConnectivity2D.hpp,,Voronoi2D,,,Sites2D Voronoi2D,não,,arquivo compartilhado (5 tipos): 94.0% linhas,não,,,,ativo (interno ao arquivo),REFATORAR,"faces casadas por proximidade geométrica com tolerância; preferir chave combinatória (i, j)"
BucketHash,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Diagram/VoronoiFaceConnectivity2D.hpp,,Voronoi2D,,,Sites2D Voronoi2D,não,,arquivo compartilhado (5 tipos): 94.0% linhas,não,,,,ativo (interno ao arquivo),REFATORAR,"faces casadas por proximidade geométrica com tolerância; preferir chave combinatória (i, j)"
VoronoiBandwidth2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Metrics/VoronoiBandwidth2D.hpp,,Voronoi2D,Bandwidth information for the sparse matrix associated with a mesh.,,Voronoi2D,não,ut_Diagram.cpp ut_Release.cpp,88.0% linhas / 96.2% ramos,não,,AdjacencyGraph2D,VMMLib/include/VoronoiMeshMaker/VoronoiBandwidth2D.hpp,ativo,REFATORAR,enum AdjacencyGraph2D de despacho
ShortVoronoiEdge2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Metrics/VoronoiEdgeLengthDiagnostics2D.hpp,,Voronoi2D,One Voronoi edge whose length is below the requested threshold.,,ErrorHandling Voronoi2D,não,,arquivo compartilhado (2 tipos): 80.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiEdgeLengthDiagnostics2D.hpp,ativo (interno ao arquivo),APROVEITAR,funciona e é testado; adaptar ao novo modelo
VoronoiEdgeLengthDiagnostics2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Metrics/VoronoiEdgeLengthDiagnostics2D.hpp,,Voronoi2D,Summary of edge-length checks over a clipped Voronoi diagram.,,ErrorHandling Voronoi2D,não,ut_Diagram.cpp,arquivo compartilhado (2 tipos): 80.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiEdgeLengthDiagnostics2D.hpp,ativo,APROVEITAR,funciona e é testado; adaptar ao novo modelo
VoronoiGenerationTimings2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Metrics/VoronoiGenerationTimer2D.hpp,,Voronoi2D,Generation times in milliseconds.,,Core,não,,arquivo compartilhado (3 tipos): 100.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiGenerationTimer2D.hpp,ativo,APROVEITAR,funciona e é testado; adaptar ao novo modelo
TimedVoronoiGeneration2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Metrics/VoronoiGenerationTimer2D.hpp,,Voronoi2D,Result of a timed 2D generation pipeline.,,Core,não,,arquivo compartilhado (3 tipos): 100.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiGenerationTimer2D.hpp,ativo (interno ao arquivo),APROVEITAR,funciona e é testado; adaptar ao novo modelo
VoronoiGenerationTimer2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Metrics/VoronoiGenerationTimer2D.hpp,,Voronoi2D,Generation times in milliseconds.,,Core,não,ut_Diagram.cpp,arquivo compartilhado (3 tipos): 100.0% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiGenerationTimer2D.hpp,ativo,APROVEITAR,funciona e é testado; adaptar ao novo modelo
VolumeRenumbering2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Ordering/VoronoiVolumeOrdering2D.hpp,,Voronoi2D,Bidirectional mapping produced by renumber_volumes().,,ErrorHandling Voronoi2D,não,,arquivo compartilhado (6 tipos): 96.3% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiVolumeOrdering2D.hpp,ativo (interno ao arquivo),APROVEITAR,funciona e é testado; adaptar ao novo modelo
VolumeRenumberingOptions2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Ordering/VoronoiVolumeOrdering2D.hpp,,Voronoi2D,Bidirectional mapping produced by renumber_volumes().,,ErrorHandling Voronoi2D,não,ut_Release.cpp,arquivo compartilhado (6 tipos): 96.3% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiVolumeOrdering2D.hpp,ativo,APROVEITAR,funciona e é testado; adaptar ao novo modelo
InputVolumeOrdering2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Ordering/VoronoiVolumeOrdering2D.hpp,,Voronoi2D,Ordering policy that keeps the current volume order unchanged.,,ErrorHandling Voronoi2D,não,ut_Release.cpp,arquivo compartilhado (6 tipos): 96.3% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiVolumeOrdering2D.hpp,ativo,APROVEITAR,funciona e é testado; adaptar ao novo modelo
LexicographicVolumeOrdering2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Ordering/VoronoiVolumeOrdering2D.hpp,,Voronoi2D,"Sorts volumes by centroid x-coordinate, then by y-coordinate.",,ErrorHandling Voronoi2D,não,ut_Release.cpp,arquivo compartilhado (6 tipos): 96.3% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiVolumeOrdering2D.hpp,ativo,APROVEITAR,funciona e é testado; adaptar ao novo modelo
HilbertVolumeOrdering2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Ordering/VoronoiVolumeOrdering2D.hpp,,Voronoi2D,Sorts volumes along a Hilbert space-filling curve over cell centroids.,,ErrorHandling Voronoi2D,não,ut_Connectivity.cpp ut_Diagram.cpp ut_Release.cpp,arquivo compartilhado (6 tipos): 96.3% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiVolumeOrdering2D.hpp,ativo,APROVEITAR,funciona e é testado; adaptar ao novo modelo
CentroidBox,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Ordering/VoronoiVolumeOrdering2D.hpp,,Voronoi2D,,,ErrorHandling Voronoi2D,não,,arquivo compartilhado (6 tipos): 96.3% linhas,não,,,VMMLib/include/VoronoiMeshMaker/VoronoiVolumeOrdering2D.hpp,ativo (interno ao arquivo),APROVEITAR,funciona e é testado; adaptar ao novo modelo
CgalKernelTraits2D,struct,vmm,VMMLib/include/VoronoiMeshMaker/Voronoi2D/Traits/CgalKernelTraits2D.hpp,,Voronoi2D,Canonical CGAL traits for unweighted 2D Voronoi generation.,CGAL,Core Sites2D,não,ut_Delaunay.cpp ut_Release.cpp,100.0% linhas / 87.5% ramos,não,,,VMMLib/include/VoronoiMeshMaker/CgalKernelTraits2D.hpp,ativo,REFATORAR,traits do CGAL em header público; vai para o backend interno
Array,struct,vmm::io,VMMLib/src/VTK_XML_ClippedVoronoi2D.cpp,,src,Deterministic XML mesh export with shared vertices and lossless encoding.,,IO,não,,não instrumentado,não,,,,ativo (interno ao arquivo),REFATORAR,escritores úteis; consolidar VTK legacy/XML e separar visualização de persistência
```
---
## 5. P03_experimento_ordem_insercao.cpp
```cpp
// Experimento P03: dependencia da ordem de insercao dos sitios (nao faz parte do repositorio).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <random>
#include <set>
#include <vector>
#include <VoronoiMeshMaker/Boundary2D/Builders/Boundary2DBuilder.hpp>
#include <VoronoiMeshMaker/Boundary2D/Shapes/Rectangle.hpp>
#include <VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp>
#include <VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiBuilder2D.hpp>
namespace b2d=::vmm::b2d; namespace s2d=::vmm::s2d; namespace vd2d=::vmm::vd2d;
using Key=std::vector<std::pair<long long,long long>>;
static Key key(const std::vector<b2d::Point2>& poly){ Key k; for(auto&p:poly){ k.emplace_back(std::llround(p.x*1e9),std::llround(p.y*1e9)); } std::sort(k.begin(),k.end()); k.erase(std::unique(k.begin(),k.end()),k.end()); return k; }
template<class Pattern> void run(const char* name, const b2d::Boundary2DData& bd, Pattern pat, bool parallel){
  s2d::SiteValidationOptions v{.min_distance_to_boundary=1e-3,.min_distance_between_sites=1e-6,.require_sequential_ids=true};
  auto A=s2d::make_sites(bd,pat,v,s2d::RegionId{1});
  vd2d::ClippedVoronoiBuildOptions2D o{}; o.allow_parallel_cell_build=parallel;
  auto dA=vd2d::ClippedVoronoiBuilder2D::build(A,bd,o);
  const std::size_t n=A.size(); int worst_area=0; std::size_t geom_diff=0, nb_diff=0; double maxdA=0;
  for(unsigned trial=0;trial<5;++trial){
    std::vector<std::size_t> perm(n); std::iota(perm.begin(),perm.end(),0); std::mt19937 g(1234+trial); std::shuffle(perm.begin(),perm.end(),g);
    s2d::SiteSet B; for(auto i:perm) B.add(A.sites[i].point,s2d::RegionId{1});
    auto dB=vd2d::ClippedVoronoiBuilder2D::build(B,bd,o);
    for(std::size_t j=0;j<n;++j){ std::size_t i=perm[j];
      const auto& ca=dA.cells[dA.site_to_cell_index[i]]; const auto& cb=dB.cells[dB.site_to_cell_index[j]];
      maxdA=std::max(maxdA,std::abs(ca.area()-cb.area()));
      if(key(ca.polygon)!=key(cb.polygon)) ++geom_diff;
      std::set<std::size_t> na,nbb; for(auto id:ca.neighbor_ids) na.insert(id.value); for(auto id:cb.neighbor_ids) nbb.insert(perm[id.value]);
      if(na!=nbb) ++nb_diff; }
  }
  std::printf("%-22s n=%zu parallel=%d | celulas com poligono diferente=%zu | vizinhos Delaunay diferentes=%zu | max|dA|=%.3e\n",name,n,(int)parallel,geom_diff,nb_diff,maxdA);
}
int main(){
  auto bd=b2d::make_boundary(b2d::Rectangle(b2d::Point2{0.0,0.0},4.0,3.0),b2d::PolygonizePolicy{},b2d::RegionId{1});
  run("aleatorio(2000)",bd,s2d::UniformRandom2D{2000,7u},true);
  run("aleatorio(2000)",bd,s2d::UniformRandom2D{2000,7u},false);
  run("cartesiano(cocircular)",bd,s2d::CartesianGridCount2D{40,30},true);
  run("hexagonal",bd,s2d::HexagonalGrid2D{24},true);
  // repeticao identica (mesma ordem) para checar reprodutibilidade simples
  s2d::SiteValidationOptions v{.min_distance_to_boundary=1e-3,.min_distance_between_sites=1e-6,.require_sequential_ids=true};
  auto S=s2d::make_sites(bd,s2d::UniformRandom2D{2000,7u},v,s2d::RegionId{1});
  auto d1=vd2d::ClippedVoronoiBuilder2D::build(S,bd); auto d2=vd2d::ClippedVoronoiBuilder2D::build(S,bd); std::size_t diff=0;
  for(std::size_t i=0;i<S.size();++i) if(d1.cells[i].polygon.size()!=d2.cells[i].polygon.size()||!std::equal(d1.cells[i].polygon.begin(),d1.cells[i].polygon.end(),d2.cells[i].polygon.begin(),[](auto&a,auto&b){return a.x==b.x&&a.y==b.y;})) ++diff;
  std::printf("mesma entrada 2x (paralelo): celulas diferentes bit a bit=%zu\n",diff);
  std::printf("primeiro sitio aleatorio seed=7: (%.17g, %.17g)\n",S.sites[0].point.x,S.sites[0].point.y);
}
```
---
## 6. Trechos de código citados (commit 7402178)

### VMMLib/include/VoronoiMeshMaker/Core/type.h (linhas 60–120)
```cpp
   60  
   61  /// Kernel used in CGAL computations (robust predicates, fast constructions).
   62  using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
   63  
   64  //--- 2D primitives ------------------------------------------------------------
   65  
   66  using Point2D   = Kernel::Point_2;     ///< 2D point.
   67  using Segment2D = Kernel::Segment_2;   ///< 2D directed segment.
   68  using Ray2D     = Kernel::Ray_2;       ///< 2D ray.
   69  using Vector2D  = Kernel::Vector_2;    ///< 2D vector.
   70  
   71  //--- 3D primitives ------------------------------------------------------------
   72  
   73  using Point3D   = Kernel::Point_3;     ///< 3D point.
   74  using Segment3D = Kernel::Segment_3;   ///< 3D directed segment.
   75  using Ray3D     = Kernel::Ray_3;       ///< 3D ray.
   76  using Vector3D  = Kernel::Vector_3;    ///< 3D vector.
   77  
   78  //--- Composite shapes ---------------------------------------------------------
   79  
   80  using Polygon2D    = CGAL::Polygon_2<Kernel>; ///< Simple 2D polygon.
   81  // 3D polyhedral storage will be restored with the future 3D module.
   82  // using Polyhedron3D = CGAL::Polyhedron_3<Kernel>;
   83  
   84  //--- Smart pointers -----------------------------------------------------------
   85  
   86  using PtrPolygon2DUnique      = std::unique_ptr<Polygon2D>;
   87  using PtrConstPolygon2DUnique = std::unique_ptr<const Polygon2D>;
   88  
   89  using PtrPolygon2DShared      = std::shared_ptr<Polygon2D>;
   90  using PtrConstPolygon2DShared = std::shared_ptr<const Polygon2D>;
   91  
   92  //--- Vector containers (preferred) -------------------------------------------
   93  
   94  using VecPoint2D = std::vector<Point2D>; ///< Contiguous collection of 2D points.
   95  using VecPoint3D = std::vector<Point3D>; ///< Contiguous collection of 3D points.
   96  
   97  //--- List containers (deprecated — prefer Vec* for cache efficiency) ----------
   98  
   99  // /** @deprecated Use `VecPoint2D` instead. `std::list` has poor cache locality
  100  //  *              for geometric data and is rarely the right container. */
  101  // [[deprecated("Use VecPoint2D — std::list has poor cache locality for geometric data")]]
  102  // using LstPoint2D = std::list<Point2D>;
  103  
  104  // /** @deprecated Use `VecPoint3D` instead. */
  105  // [[deprecated("Use VecPoint3D — std::list has poor cache locality for geometric data")]]
  106  // using LstPoint3D = std::list<Point3D>;
  107  
  108  GEOTYPES_NAMESPACE_CLOSE
  109  
  110  
  111  //------------------------------------------------------------------------------
  112  // Numeric types at the vmm level
  113  //------------------------------------------------------------------------------
  114  
  115  using Real = vmm::gtp::Kernel::FT;   ///< Floating-point type matching the CGAL kernel.
  116  using Int  = vmm::gtp::Kernel::RT;   ///< Integer / exact ring type of the CGAL kernel.
  117  
  118  //--- Numeric containers (preferred) ------------------------------------------
  119  
  120  using VecInt  = std::vector<Int>;   ///< Contiguous collection of integers.
```

### VMMLib/include/VoronoiMeshMaker/Core/constants.h (linhas 55–80)
```cpp
   55   * @brief Numerical zero tolerance (very small threshold).
   56   *
   57   * Used as the default lower bound when testing whether a floating-point value
   58   * is "effectively zero". For IEEE double, 1e-12 is well above the rounding
   59   * floor of typical CGAL inexact constructions while still representing a
   60   * physically negligible length.
   61   */
   62  inline constexpr Real kZeroTol = static_cast<Real>(1e-12);
   63  
   64  /**
   65   * @brief Small limit threshold for conservative comparisons.
   66   *
   67   * Intended for clamping borderline-negative areas or volumes to zero without
   68   * treating genuine errors as valid results (e.g. `area = std::max(kLimit, raw_area)`).
   69   */
   70  inline constexpr Real kLimit = static_cast<Real>(1e-30);
   71  
   72  /**
   73   * @brief Epsilon for equality checks in non-critical geometric predicates.
   74   *
   75   * Use tighter or looser values where the algorithm warrants it; this default
   76   * is suitable for containment and clipping tests at unit-scale geometry.
   77   */
   78  inline constexpr Real kEpsilon = static_cast<Real>(1e-6);
   79  
   80  /**
```

### VMMLib/include/VoronoiMeshMaker/Boundary2D/Boundary2DTypes.hpp (linhas 25–45)
```cpp
   25  //------------------------------------------------------------------------------
   26  //      VoronoiMeshMaker includes
   27  //------------------------------------------------------------------------------
   28  #include <VoronoiMeshMaker/Core/constants.h>   // sanity checks on Real
   29  
   30  VORMAKER_NAMESPACE_OPEN
   31  BOUNDARY2D_NAMESPACE_OPEN 
   32  
   33  //------------------------------------------------------------------------------
   34  // Numeric & index aliases (compact, cache-friendly)
   35  //------------------------------------------------------------------------------
   36  
   37  /**
   38   * @brief Real used by Boundary2D. Delegates to the project-wide Real.
   39   *        Keep this alias here for a single include point across the module.
   40   */
   41  using Real = ::vmm::Real;
   42  
   43  /** @brief Signed index for CSR offsets, vertex indices, etc. */
   44  using Index  = std::int32_t;
   45  
```

### VMMLib/include/VoronoiMeshMaker/Voronoi2D/Delaunay/DelaunayBuilder2D.hpp (linhas 30–60)
```cpp
   30      bool require_sequential_ids{true};
   31      bool require_unique_points{true};
   32      bool require_two_dimensional{true};
   33  };
   34  
   35  using DelaunayTriangulation2D = CgalKernelTraits2D::DelaunayTriangulation;
   36  
   37  /**
   38   * @brief Value-type builder for CGAL Delaunay triangulations.
   39   *
   40   * The builder inserts every Site2D point into a CGAL triangulation and stores
   41   * the corresponding SiteId in the CGAL vertex info field. The returned
   42   * triangulation is the first CGAL-backed data structure in the Voronoi pipeline.
   43   */
   44  struct DelaunayBuilder2D {
   45      [[nodiscard]] static DelaunayTriangulation2D build(
   46          const ::vmm::s2d::SiteSet& sites,
   47          const DelaunayBuildOptions2D& options = DelaunayBuildOptions2D{})
   48      {
   49          if (sites.size() < 3U) {
   50              VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
   51                        {{"where", "DelaunayBuilder2D"},
   52                         {"reason", "at_least_three_sites_required"}});
   53          }
   54          if (options.require_sequential_ids && !sites.ids_are_sequential()) {
   55              VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
   56                        {{"where", "DelaunayBuilder2D"},
   57                         {"reason", "non_sequential_site_ids"}});
   58          }
   59  
   60          DelaunayTriangulation2D triangulation;
```

### VMMLib/include/VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiDiagram2D.hpp (linhas 150–165)
```cpp
  150      using SiteId = ::vmm::s2d::SiteId;
  151  
  152      //--------------------------------------------------------------------------
  153      // Primary data
  154      //--------------------------------------------------------------------------
  155  
  156      ::vmm::b2d::Boundary2DData boundary{};          ///< Domain boundary.
  157      ::vmm::s2d::SiteSet        sites{};             ///< Generator sites.
  158      DelaunayTriangulation2D    delaunay{};          ///< Underlying triangulation.
  159      std::vector<VoronoiCell2D> cells{};             ///< One cell per site, in volume_id order.
  160  
  161      //--------------------------------------------------------------------------
  162      // Index structures rebuilt by rebuild_indices()
  163      //--------------------------------------------------------------------------
  164  
  165      std::vector<std::size_t> all_indices{};         ///< `[0, 1, ..., n-1]` in current order.
```

### VMMLib/include/VoronoiMeshMaker/Voronoi2D/Cells/VoronoiCellBuilder2D.hpp (linhas 108–136)
```cpp
  108      using Index  = ::vmm::s2d::Index;
  109  
  110      /** @brief Validate the single convex outer ring supported by this builder. */
  111      static void validate_domain(const ::vmm::b2d::Boundary2DData& boundary) {
  112          if (!boundary.invariant_ok() || boundary.ring_count() == 0) {
  113              VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
  114                        {{"name", "invalid boundary storage"}});
  115          }
  116          if (boundary.ring_count() != 1 ||
  117              boundary.kinds.front() != ::vmm::b2d::LoopKind::Outer) {
  118              VMM_THROW(::vmm::error::CoreErr::NotImplemented,
  119                        {{"reason", "multiple rings and holes require multi-component cells"}});
  120          }
  121          const auto ring = boundary.ring(0);
  122          for (std::size_t i = 0; i < ring.size(); ++i) {
  123              const auto a = ring[i];
  124              const auto b = ring[(i + 1U) % ring.size()];
  125              if (!std::isfinite(a.x) || !std::isfinite(a.y) ||
  126                  (a.x == b.x && a.y == b.y)) {
  127                  VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
  128                            {{"name", "non-finite or repeated boundary vertex"}});
  129              }
  130          }
  131          if (!CgalKernelTraits2D::is_simple_ccw_convex(ring)) {
  132              VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
  133                        {{"name", "boundary must be simple, counter-clockwise and convex"}});
  134          }
  135      }
  136  
```

### VMMLib/include/VoronoiMeshMaker/Voronoi2D/Diagram/VoronoiFaceConnectivity2D.hpp (linhas 100–160)
```cpp
  100                          "; mesh rejected, no connectivity returned");
  101                  }
  102                  scale = std::max({scale, std::abs(edge.a.x), std::abs(edge.a.y),
  103                                   std::abs(edge.b.x), std::abs(edge.b.y)});
  104              }
  105          }
  106  
  107          const Real tolerance = Real{64} * std::numeric_limits<Real>::epsilon() * scale;
  108          std::vector<Reference> references;
  109          std::unordered_map<Bucket, std::vector<std::size_t>, BucketHash> buckets;
  110          std::vector<std::vector<VoronoiCellEdge2D>> staged;
  111          staged.reserve(cells.size());
  112          for (std::size_t c = 0; c < cells.size(); ++c) {
  113              staged.push_back(cells[c].edges);
  114              for (std::size_t e = 0; e < staged.back().size(); ++e) {
  115                  auto& edge = staged.back()[e];
  116                  edge.length = std::hypot(edge.b.x - edge.a.x, edge.b.y - edge.a.y);
  117                  edge.midpoint = {std::midpoint(edge.a.x, edge.b.x),
  118                                   std::midpoint(edge.a.y, edge.b.y)};
  119                  edge.neighbour_site_id = ::vmm::s2d::kInvalidSiteId;
  120                  if (edge.is_boundary_edge) continue;
  121                  if (edge.length <= Real{2} * tolerance) {
  122                      throw std::runtime_error("Internal face is below geometric matching resolution");
  123                  }
  124                  buckets[bucket(edge.midpoint, tolerance)].push_back(references.size());
  125                  references.push_back({c, e});
  126              }
  127          }
  128  
  129          std::vector<std::size_t> partners(references.size(), references.size());
  130          for (std::size_t i = 0; i < references.size(); ++i) {
  131              const auto ref = references[i];
  132              const auto& edge = staged[ref.cell][ref.edge];
  133              const auto key = bucket(edge.midpoint, tolerance);
  134              std::size_t matches = 0;
  135              for (std::int64_t dx = -1; dx <= 1; ++dx) {
  136                  for (std::int64_t dy = -1; dy <= 1; ++dy) {
  137                      const auto found = buckets.find({key.x + dx, key.y + dy});
  138                      if (found == buckets.end()) continue;
  139                      for (const auto j : found->second) {
  140                          const auto other = references[j];
  141                          if (other.cell == ref.cell) continue;
  142                          const auto& candidate = staged[other.cell][other.edge];
  143                          if (close(edge.a, candidate.b, tolerance) &&
  144                              close(edge.b, candidate.a, tolerance)) {
  145                              partners[i] = j;
  146                              ++matches;
  147                          }
  148                      }
  149                  }
  150              }
  151              if (matches != 1U) {
  152                  throw std::runtime_error("Physical face must have exactly one reciprocal segment: site " +
  153                      std::to_string(cells[ref.cell].site_id.value) + ", edge " +
  154                      std::to_string(ref.edge) + ", matches " + std::to_string(matches));
  155              }
  156          }
  157          for (std::size_t i = 0; i < references.size(); ++i) {
  158              if (partners[partners[i]] != i) {
  159                  throw std::runtime_error("Non-reciprocal geometric face pairing");
  160              }
```

### VMMLib/include/VoronoiMeshMaker/Voronoi2D/Diagram/ClippedVoronoiBuilder2D.hpp (linhas 40–50)
```cpp
   40  
   41  //==============================================================================
   42  // TBB includes
   43  //==============================================================================
   44  #include <tbb/parallel_for.h>
   45  
   46  //==============================================================================
   47  // VoronoiMeshMaker includes
   48  //==============================================================================
   49  #include <VoronoiMeshMaker/Boundary2D/Boundary2DData.hpp>
   50  #include <VoronoiMeshMaker/Boundary2D/Queries/Boundary2DContains.hpp>
```

### VMMLib/include/VoronoiMeshMaker/Sites2D/Factory/SiteFactory.hpp (linhas 925–940)
```cpp
  925      RegionId region = RegionId{0},
  926      Real weight = Real{0})
  927  {
  928      validate_pattern_or_throw(pattern);
  929      validate_factory_inputs_or_throw(boundary, validation);
  930  
  931      const auto box = ::vmm::b2d::bounding_box(boundary);
  932      std::mt19937 rng(pattern.seed);
  933      std::uniform_real_distribution<Real> x_dist(box.min.x, box.max.x);
  934      std::uniform_real_distribution<Real> y_dist(box.min.y, box.max.y);
  935      const auto candidate_validation = candidate_options(validation);
  936      const auto max_attempts = effective_random_max_attempts(pattern);
  937  
  938      SiteSpacingIndex2D spacing_index(validation.min_distance_between_sites);
  939      spacing_index.reserve(sites.size() + pattern.target_count);
  940      if (spacing_index.enabled()) {
```

### VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/Halfplane2D.hpp (linhas 40–55)
```cpp
   40          return a * p.x + b * p.y - c;
   41      }
   42  
   43      [[nodiscard]] constexpr bool contains(
   44          Point2 p,
   45          Real eps = ::vmm::constants::kEpsilon) const noexcept
   46      {
   47          return evaluate(p) <= eps;
   48      }
   49  
   50      [[nodiscard]] bool is_valid(
   51          Real eps = ::vmm::constants::kEpsilon) const noexcept
   52      {
   53          return std::isfinite(a) &&
   54                 std::isfinite(b) &&
   55                 std::isfinite(c) &&
```

### VMMLib/include/VoronoiMeshMaker/Voronoi2D/Clipping/BisectorHalfplane.hpp (linhas 34–44)
```cpp
   34  
   35      [[nodiscard]] static Halfplane2D between(Point2 owner, Point2 neighbor) {
   36          const Real dx = neighbor.x - owner.x;
   37          const Real dy = neighbor.y - owner.y;
   38          const Real norm2 = dx * dx + dy * dy;
   39          if (norm2 <= ::vmm::constants::kEpsilon * ::vmm::constants::kEpsilon) {
   40              VMM_THROW(::vmm::error::CoreErr::InvalidArgument,
   41                        {{"where", "BisectorHalfplane"},
   42                         {"reason", "coincident_sites"}});
   43          }
   44  
```

### cmake/VerifyCGALHeaders.cmake (linhas 40–56)
```cpp
   40      if(NOT cgal_target)
   41          set(cgal_target CGAL::CGAL)
   42      endif()
   43      target_include_directories(${cgal_target} SYSTEM BEFORE INTERFACE "${include_overlay}")
   44  
   45      include(CheckCXXSourceCompiles)
   46      set(CMAKE_REQUIRED_LIBRARIES CGAL::CGAL)
   47      unset(VMM_CGAL_HEADERS_MATCH CACHE)
   48      check_cxx_source_compiles(
   49          "#include <CGAL/version.h>\nstatic_assert(CGAL_VERSION_NR == ${version_number});\nint main() {}"
   50          VMM_CGAL_HEADERS_MATCH)
   51      if(NOT VMM_CGAL_HEADERS_MATCH)
   52          message(FATAL_ERROR
   53              "CGAL package/header mismatch. Remove conflicting CGAL include flags; selected ${version_header}")
   54      endif()
   55      message(STATUS "VMM CGAL: ${CGAL_VERSION}; headers ${cgal_headers}; version number ${version_number}")
   56  endfunction()
```

### cmake/ConfigDependencies.cmake (linhas 1–30)
```cpp
    1  # Dependencies configuration
    2  
    3  if(POLICY CMP0167)
    4      cmake_policy(SET CMP0167 NEW)
    5  endif()
    6  set(CMAKE_POLICY_DEFAULT_CMP0167 NEW)
    7  
    8  find_package(CGAL REQUIRED COMPONENTS Core)
    9  include("${CMAKE_CURRENT_LIST_DIR}/VerifyCGALHeaders.cmake")
   10  vmm_verify_cgal_headers()
   11  
   12  find_package(TBB REQUIRED)
   13  
   14  if(VMM_BUILD_EXAMPLES)
   15      find_package(yaml-cpp QUIET)
   16      if(yaml-cpp_FOUND)
   17          message(STATUS "yaml-cpp found: YAML-based examples enabled")
   18      else()
   19          message(STATUS "yaml-cpp not found: YAML-based examples will be skipped")
   20      endif()
   21  endif()
   22  
   23  if(VMM_BUILD_TESTS)
   24      find_package(GTest REQUIRED)
   25      find_package(Threads REQUIRED)
   26  endif()
```

### tests/CMakeLists.txt (linhas 165–200)
```cpp
  165    endif()
  166  
  167    # Coverage flags - aplicado apenas se gcovr estiver disponível
  168    if(_HAVE_GCOVR AND _COV_FLAGS)
  169      target_compile_options(${target_name} PRIVATE ${_COV_FLAGS})
  170      target_link_options(${target_name}    PRIVATE ${_COV_FLAGS})
  171    endif()
  172  
  173    target_link_libraries(${target_name} PRIVATE
  174      GTest::gtest
  175      GTest::gtest_main
  176      Threads::Threads
  177    )
  178    if(TARGET TBB::tbb)
  179      target_link_libraries(${target_name} PRIVATE TBB::tbb)
  180    endif()
  181    if(TARGET GTest::gmock)
  182      target_link_libraries(${target_name} PRIVATE GTest::gmock)
  183    endif()
  184  
  185    # Link na lib principal, se presente (resolve símbolos como ErrorConfig::ErrorConfig)
  186    if(TARGET ${VMM_CORE_LIB})
  187      target_link_libraries(${target_name} PRIVATE ${VMM_CORE_LIB})
  188    endif()
  189  
  190    # Binário sai dentro da própria pasta do driver
  191    set(test_output_dir "${CMAKE_CURRENT_BINARY_DIR}/${test_dir_rel}")
  192    file(MAKE_DIRECTORY "${test_output_dir}")
  193    set_target_properties(${target_name} PROPERTIES
  194      RUNTIME_OUTPUT_DIRECTORY "${test_output_dir}"
  195      OUTPUT_NAME              "${test_name_we}"
  196    )
  197  
  198    # Registro no CTest (descobre os TEST() dentro do driver)
  199    gtest_discover_tests(${target_name}
  200      WORKING_DIRECTORY "${test_output_dir}"
```

### examples/CMakeLists.txt (linhas 118–130)
```cpp
  118    # Output ao lado do .cpp
  119    if(_n_src EQUAL 1)
  120      set(out_name "${stem}")
  121    else()
  122      set(out_name "${target_name}")
  123    endif()
  124    set_target_properties(${target_name} PROPERTIES
  125      RUNTIME_OUTPUT_DIRECTORY "${dir_abs}"
  126      OUTPUT_NAME "${out_name}"
  127    )
  128  
  129    # Otimizações leves em Release
  130    if(CMAKE_BUILD_TYPE STREQUAL "Release")
```

### paper/CMakeLists.txt (linhas 70–80)
```cpp
   70      target_compile_definitions(${target_name}
   71          PRIVATE
   72              "VMM_PAPER_CASE_SOURCE_DIR=\"${dir_abs}\""
   73      )
   74  
   75      set_target_properties(${target_name} PROPERTIES
   76          RUNTIME_OUTPUT_DIRECTORY "${dir_abs}"
   77          OUTPUT_NAME "${output_name}"
   78      )
   79  
   80      add_dependencies(paper_programs ${target_name})
```
