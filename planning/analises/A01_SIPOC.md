# A01 — SIPOC do VoronoiMeshMaker

- **Data:** 2026-10-16
- **Objeto:** VMM no `main` do GitHub, commit `fb02bb7` (08/10/2026), versão 1.1.0 preparada e não publicada.
- **Ambiente:** sessão do Claude na nuvem, fora do WSL oficial (AGENTS.md). Ubuntu 24.04, GCC 14.2, CMake 3.28.3 e 4.4.4, CGAL 6.2.1, Boost 1.83, GoogleTest 1.14.
- **Convenção:** **[F]** fato verificado (arquivo, comando ou página) · **[I]** inferência · **[R]** recomendação.
- **Papel no conjunto:** fixar fronteiras e interfaces. As falhas de cada etapa ficam para a FMEA (A02), a priorização para o GUT (A03), e as correções para o consolidado (A05).

---

## 1. Visão geral

| S — Fornecedores | I — Entradas | P — Processo | O — Saídas | C — Clientes |
|---|---|---|---|---|
| Usuário da API C++ | declaração de regiões e buracos (formas 2D/3D, rótulos de patch, meios) | 1. Declarar o domínio | `Partition2D` / `Partition3D` validada | solver próprio do João |
| Usuário do `vmm-mesh` | arquivo `.cfg` (`chave = valor`) | 2. Preparar o domínio (recorte exato, reparo de STL) | `Mesh2D` / `Mesh3D` imutável, com métricas de volumes finitos | MohidNG (H01: consumidor) |
| CAD/terreno externos | STL (domínio 3D) | 3. Gerar os sítios por região | `LayeredMesh` (colunas e horizontes) | usuários externos (biblioteca pública, DEC-041) |
| Dados de terreno do usuário | `HorizonGrid` (z(x,y) por função ou cotas) | 4. Construir o Voronoi recortado e conforme | `.vmesh` v1, `.vlayers`, `.vtu`, `.stl` | visualização (ParaView/PyVista) |
| CGAL 6.2.1, Boost | Delaunay, recorte, PMP, AABB, aritmética exata | 5. Verificar os invariantes e a qualidade | `InvariantReport`, relatório de qualidade, `BuildStats` | revisão do João (aprovação) |
| João e agentes de IA | requisitos, DECs, prompts, relatórios | 6. Colunas e horizontes (opcional) | CSR de adjacência, padrão esparso, RCM | CI e Pages (garantia e publicação) |
| GitHub (Actions, Pages) | CI, publicação do manual | 7. CVT com partição fixa (opcional) | `CvtReport` (energia, resíduo, convergência) | revisores do JOSS (previsto no P05) |
| — | — | 8. Reordenar, gravar e documentar | manual pt/en e galeria | — |

**[F]** Fontes: `vmm/README.md`; `include/vmm/vmm.hpp`, `cvt.hpp`, `layered.hpp`; `src/app/registries.cpp`; `planning/horizontes/01_requisitos.md` (H01–H10); `.github/workflows/`.

---

## 2. Processo de geração da malha, etapa por etapa

| # | Etapa | Entradas | Saídas | API / ferramenta | Evidência de controle |
|---|---|---|---|---|---|
| 1 | Declarar o domínio | formas 2D (`rectangle`, `polygon` com `xy` em linha, `circle`, `ellipse`, `regular_ngon`); 3D (`cuboid`, `sphere`, `cylinder`, `extrusion`, `stl`); precedência; meios | `Declaration` / `Declaration3D` | `domain/declaration*.hpp`, `ShapeRegistry*` | validador de vazios, regiões anuladas e lascas (R15, DEC-018); `InvalidPolygon` para contorno com auto-interseção (DEC-033) |
| 2 | Preparar o domínio | declaração; STL | partição exata; `PreparedDomain3` opaco | `build_partition` (backend CGAL); `repair_surface`, `read_stl_surface` | firewall: nenhum header público inclui o CGAL (R19), verificado pelo alvo `vmm_firewall` |
| 3 | Gerar os sítios | fontes por região: `uniform`, `count`, `grid`, `hexagonal`, `explicit` (config); em C++ também `AdaptiveQuadtreeSource` / `AdaptiveOctreeSource3D` com `SpacingField`; `InterfacePairs` / `InterfacePairs3D` | `SiteSet` / `SiteSet3D` | `sites/sources*.hpp`, `generate_sites*` | determinismo por semente (R18); `validate_sites_3d` |
| 4 | Construir a malha | partição + sítios | `Mesh<D>`: owner/neighbour, vetores de área, d_ij, intervalos internos e de fronteira (DEC-029) | `build_mesh_2d/3d`, `generate_mesh_2d/3d` | topologia por rótulos, vértices canônicos (DEC-028); ortogonalidade estrutural (DEC-032); célula fragmentada fica inteira (DEC-035) |
| 5 | Verificar | malha + referência da partição | `InvariantReport`, `quality_report`, `BuildStats` | `mesh/invariants.hpp`, `metrics.hpp` | soma das medidas, fechamento, owner/neighbour, interfaces (R16) |
| 6 | Colunas e horizontes | `Mesh2D` base + `HorizonGrid` + frações por intervalo | `LayeredMesh` (região = região da base × intervalo), proveniência | `generate_layered_mesh`, `column_overlay` (backend) | H03–H07; testes de integração `Layered` |
| 7 | CVT | partição fixa + sítios; ou `cvt_domain(LayeredMesh)` | `Mesh<D>` + `CvtReport` | `optimize_cvt` (Lloyd, densidade uniforme) | `fixed_support` e invariantes a cada passo aceito (`src/facade/cvt.cpp:100,114`) |
| 8 | Reordenar e gravar | malha | RCM; `.vmesh` v1 (ida e volta exata), `.vlayers`, `.vtu`, `.stl` | `reorder`, `io/native`, `io/layered`, `io/vtu`, `io/stl` | testes de ida e volta; `.vmesh` v1 estável (DEC-041) |

**Fluxo sem C++:** `.cfg` → `MeshConfig` → `make_request_2d/3d` → `run_config` → `vmm-mesh` → `.vmesh` + `.vtu` (DEC-040). As chaves `horizons` e `cvt_*` entram na 1.1 (`src/app/config.cpp:96`, `src/app/run.cpp:294`).

---

## 3. Processo de desenvolvimento e publicação

| S | I | P | O | C |
|---|---|---|---|---|
| João (requisitos, aprovação, commits) | pedidos, respostas a questionários | prompt → iteração → relatório → revisão → commit | código, testes, relatórios em `planning/` | o próprio João; MohidNG; público |
| Agentes de IA (Claude, Codex) | `AGENTS.md`, `planning/`, `DECISIONS.md` | execução só no WSL `Ubuntu-26.04-Test` (AGENTS.md) | DECs PROPOSTA → APROVADA | — |
| GitHub Actions | push no `main` | `vmm-ci` (gcc-14 Debug/Release, clang-19 Release, sanitizadores, cobertura, verificação estática); `vmm-docs` (portão de testes, Doxygen, Sphinx pt/en, Pages) | estado da CI; site em GitHub Pages | usuários do manual |
| — | tags e releases | publicação de versão | **nenhuma tag nem release no repositório** [F] | usuários de `find_package(VoronoiMeshMaker 1.x)` |

**[F]** `git ls-remote --tags origin` não devolve nada. A página de releases do GitHub diz "There aren't any releases here". O site publicado já mostra "VoronoiMeshMaker 1.1", mas a 1.1.0 consta como "preparada, não publicada" (`vmm/CHANGELOG.md`).

---

## 4. Interfaces entre fornecedores, processo e clientes

| Interface | Contrato existente | Estado |
|---|---|---|
| Usuário C++ → VMM | headers `include/vmm`, `Result`/`value_or_throw`, semver 1.x (DEC-041) | definido |
| Usuário sem C++ → VMM | `.cfg` com chaves estáveis (DEC-040, DEC-041) | definido, mas sem fonte de sítios adaptativa e sem polígono 2D lido de arquivo [F] (`src/app/registries.cpp`: fontes `uniform`, `count`, `grid`, `hexagonal`, `explicit`; polígono só com `xy` em linha) |
| VMM → CGAL | backend interno (DEC-007); mínimo 6.2 | definido; a versão mais recente estável é 6.2.1 [F] |
| VMM → MohidNG | `.vmesh` v1, `.vlayers`, API C++ (H01, H02) | **parcial**: não há leitor de referência fora do C++, API em C nem especificação de leitura para o MohidNG no repositório [F] (busca por `fortran`, `extern "C"`, `hdf5` em `vmm/` sem resultado) |
| Colunas → CVT | `cvt_domain(LayeredMesh)` | definido, mas o CVT devolve `Mesh3D` **sem colunas** (`cvt.hpp`; `planning/cvt/01_entrega.md`), o que contraria o uso em colunas (H04) depois do CVT |
| Dados de terreno → VMM | `HorizonGrid` (z(x,y) por função ou cotas) | sem leitura de formatos de terreno (raster, XYZ) e sem sistema de coordenadas; a conversão de profundidade é do consumidor (`planning/horizontes/relatorios/P29_entrega.md`) |
| Desenvolvimento → público | CI, Pages, CHANGELOG | sem tags e releases; o manual publicado está à frente da versão publicada |
| Requisitos → decisões | `DECISIONS.md` (DEC-001–042) | as decisões dos pacotes de horizontes (Q01–Q10, P21) e do CVT (`planning/cvt/00_sequencia.md`, "Decisões") **não viraram DECs** [F] |

---

## 5. Achados do SIPOC

| # | Achado | Tipo | Evidência |
|---|---|---|---|
| S1 | O cliente principal (MohidNG) não tem contrato de leitura no repositório: formato, linguagem e convenções (profundidade, k) ficam implícitos | interface de saída | §4; H01/H02; `P29_entrega.md` |
| S2 | O CVT e as colunas não se compõem: depois do CVT a organização em colunas se perde | conflito entre etapas 6 e 7 | `cvt.hpp:52`; `cvt/01_entrega.md` |
| S3 | O fluxo sem C++ (`vmm-mesh`) não oferece espaçamento variável nem polígonos de arquivo, que são necessários em domínios reais | entrada | `src/app/registries.cpp` |
| S4 | Não há entradas geoespaciais (raster, XYZ, sistema de coordenadas); o terreno entra só como `HorizonGrid` | entrada | `geometry/horizons.hpp` |
| S5 | A publicação não fecha o ciclo: sem tags nem releases, `find_package(VoronoiMeshMaker 1.0)` não tem versão de referência; o site mostra 1.1 antes da publicação | saída do processo | §3 |
| S6 | As decisões das etapas 6 e 7 ficaram fora do registro único (DEC-013) | governança | §4 |
| S7 | A validação depende de um único ambiente (WSL do João); a CI valida só o que é publicado no `main`, e as falhas entram direto no `main` | controle do processo | `AGENTS.md`; página do Actions em 16/10: 11 falhas e 2 cancelamentos entre as 25 execuções mais recentes, de 30/09 a 08/10 |
| S8 | O CVT tem densidade uniforme: aplicado a uma malha graduada, tende a uniformizar os tamanhos dentro de cada região [I] (propriedade do Lloyd com densidade constante) | etapa 7 × etapa 3 | `cvt.hpp:21`; `cvt/01_entrega.md`, "Limitações" |

Esses achados entram na FMEA (A02) como modos de falha, junto com os vindos do build e dos testes.
