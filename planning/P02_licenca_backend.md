# P02 — Licença, dependências e backend geométrico

- **Data:** 2026-09-28
- **Sequência:** v4, prompt P02 — **versão 1.1 (congelada)**, com as correções do João de 28/09
- **Destino no repositório:** `planning/P02_licenca_backend.md`
- **Convenção:** **[F]** fato verificado (com fonte) · **[I]** inferência · **[R]** recomendação · **[NV]** não verificado.
- **Decisões vigentes:**
  - DEC-001 (N1: domínio + sítios → modelo de malha de VF);
  - DEC-002 (backend atrás de policy; o solver só vê o modelo do VMM);
  - DEC-004 (mínimo de dependências externas);
  - DEC-005 a DEC-009, aprovadas ao final deste prompt (§8).
- **Limitação:** a versão do CGAL instalada no WSL não pôde ser verificada daqui, porque a pasta não está conectada a esta sessão. As licenças foram lidas no código-fonte do CGAL `main` de 2026-09-21 (ramo 6.2.x). Confirmar a versão local no P03.
- **Aviso:** isto não é aconselhamento jurídico. Para a decisão final de licença, vale uma consulta ao setor de inovação ou jurídico da UERJ.

---

## 1. Licenças do CGAL, pacote a pacote

**[F]** O CGAL tem licença dupla, open source ou comercial (GeometryFactory). "The Kernel and Support libraries are under the LGPL, and most geometric algorithms and data structures are under the GPL" ([cgal.org/license](https://www.cgal.org/license.html)).

A tabela abaixo foi extraída dos cabeçalhos `SPDX-License-Identifier` de cada pacote no código-fonte ([github.com/CGAL/cgal](https://github.com/CGAL/cgal), commit de 2026-09-21). Os identificadores SPDX exatos são `LGPL-3.0-or-later OR LicenseRef-Commercial` ("LGPL-3.0+" na tabela) e `GPL-3.0-or-later OR LicenseRef-Commercial` ("GPL-3.0+"). A licença comercial é vendida pela [GeometryFactory](https://geometryfactory.com/).

| Pacote | Uso no VMM | Licença |
|---|---|---|
| Kernel_23 | pontos, predicados, EPICK/EPECK | **LGPL-3.0+** ou comercial |
| Number_types | aritmética exata | **LGPL-3.0+** ou comercial |
| STL_Extension | utilitários | **LGPL-3.0+** ou comercial (5 arquivos BSL-1.0) |
| Spatial_sorting | ordenação de inserção | **LGPL-3.0+** ou comercial |
| Polygon | `Polygon_2` | **LGPL-3.0+** ou comercial |
| HalfedgeDS | estrutura half-edge (base do `Polyhedron_3`) | **LGPL-3.0+** ou comercial |
| Polyhedron | `Polyhedron_3` (já usado na VMMLib) | **GPL-3.0+** ou comercial (22 arquivos; 1 arquivo LGPL) |
| Triangulation_2 | Delaunay 2D, CDT (já usado na VMMLib) | **GPL-3.0+** ou comercial |
| Triangulation_3 | Delaunay 3D | **GPL-3.0+** ou comercial |
| Periodic_2/3_triangulation | periodicidade | **GPL-3.0+** ou comercial |
| Voronoi_diagram_2 | adaptador de Voronoi 2D | **GPL-3.0+** ou comercial |
| Boolean_set_operations_2 | booleanas 2D | **GPL-3.0+** ou comercial |
| Convex_hull_3 | `halfspace_intersection_3` | **GPL-3.0+** ou comercial |
| Polygon_mesh_processing | recorte/booleanas 3D, reparo, extrusão | **GPL-3.0+** ou comercial |
| AABB_tree | consultas de distância e interseção | **GPL-3.0+** ou comercial |
| Surface_mesh | estrutura de malha de superfície | **GPL-3.0+** ou comercial |
| Mesh_2 / Mesh_3 | malhagem, CVT (`lloyd_optimize`) | **GPL-3.0+** ou comercial |
| Nef_3 | booleanas exatas | **GPL-3.0+** ou comercial |
| Alpha_wrap_3, Polygon_repair | reparo de geometria | **GPL-3.0+** ou comercial |

**[I] Consequência:** qualquer triangulação (2D ou 3D) do CGAL é GPL. A VMMLib já usa `Delaunay_triangulation_2`, `Constrained_Delaunay_triangulation_2` e `Polyhedron_3`. Portanto, **todo binário que inclua o backend CGAL já é regido pela GPL-3.0+**, a menos que o usuário compre a licença comercial do CGAL.

## 2. Dependências do CGAL

**[F]**
- Header-only desde a versão 5.3: "The support for the compiled version of CGAL is dropped. Only the header-only version is supported" (Installation/CHANGES.md, Release 5.3, julho de 2021).
- Dependências essenciais ([manual 6.2.1](https://doc.cgal.org/latest/Manual/thirdparty.html)):
  - **Boost ≥ 1.74**, só os headers;
  - **GMP e MPFR, ou Boost.Multiprecision**, para os números de precisão múltipla.
- Opcionais: TBB (paralelismo), Eigen etc.

**[I]** Com o Boost.Multiprecision, o CGAL fica com **zero bibliotecas compiladas obrigatórias**: só headers do CGAL e do Boost. O GMP/MPFR tende a deixar a aritmética exata mais rápida; é opcional e recomendado para builds de produção, mas o padrão do VMM é o Boost.Multiprecision (DEC-009).

## 3. Afirmações anteriores: verificação

| Afirmação feita antes | Resultado | Fonte |
|---|---|---|
| As triangulações periódicas do CGAL exigem domínio cúbico/quadrado | **Confirmada.** O domínio original é "the cube [α,α+c)×[β,β+c)×[γ,γ+c)" (3D), e o análogo "square" em 2D; há uma asserção de que os pontos estão dentro do domínio | docs dos pacotes Periodic_3_triangulation_3 e Periodic_2_triangulation_2 |
| O sphinx-gallery aceita exemplos em C++ | **Confirmada, com limite.** Renderiza exemplos em outras linguagens, que "are not currently executed or scanned for output". O cabeçalho é o primeiro bloco de comentário, com título reST; separadores `//%%` ou `// %%` | [sphinx-gallery syntax](https://sphinx-gallery.github.io/stable/syntax.html); [PR #1192](https://github.com/sphinx-gallery/sphinx-gallery/pull/1192) |
| `PMP::extrude_mesh` existe | **Confirmada.** Adicionada na release de outubro de 2018 (CGAL 4.13) | Installation/CHANGES.md |
| (Implícita) o CGAL precisa ser compilado | **Refutada.** Header-only desde a 5.3 | Installation/CHANGES.md |

## 4. CGAL × Geogram (DEC-002) e VoroCrust

A comparação foi feita, e ela fundamenta a decisão "sem Geogram".

| Critério | CGAL | Geogram |
|---|---|---|
| Já está no projeto | **sim** | não (seria uma segunda biblioteca geométrica, contra o DEC-004) |
| Voronoi recortado 3D pronto | **não**: montar com Delaunay_3 + `halfspace_intersection_3` + PMP | **sim**: `RestrictedVoronoiDiagram::compute_RVD` em modo volumétrico ([RVD.h](https://github.com/BrunoLevy/geogram/blob/main/src/lib/geogram/voronoi/RVD.h)) |
| Domínio STL arbitrário | PMP (clip/corefine) + AABB_tree | exige malha tetraédrica do domínio; `mesh_tetrahedralize` usa o **TetGen embutido** (mesh_tetrahedralize.cpp) |
| Licença efetiva | GPL-3.0+ (algoritmos) | BSD-3. Embute TetGen (AGPL-3.0) e Triangle (uso comercial só "by direct arrangement"), ligados por padrão, mas **desligáveis** por opções de build (`GEOGRAM_WITH_TETGEN=OFF`, `GEOGRAM_WITH_TRIANGLE=OFF`); sem o TetGen, o recorte a partir de uma superfície exige que a malha tetraédrica venha de fora |
| Estilo de API | templates e traits: um adaptador sem herança é natural (R3) | callbacks virtuais (`RVDPolyhedronCallback::begin_polyhedron(seed, tet)` etc.) e factories por string: o adaptador teria de derivar classes |
| Robustez | predicados exatos; construções exatas com EPECK | predicados exatos (PCK) com perturbação simbólica |
| Paralelismo | `Parallel_tag` com TBB (opcional) | Delaunay 3D paralelo nativo |

**[F] VoroCrust:** binários públicos para Windows 10, RHEL 7, Ubuntu 18.04–22.10 e macOS 12; código-fonte e repositório "Coming Soon"; licença BSD-3 ([downloads](https://vorocrust.sandia.gov/vorocrust-meshing-downloads/)).

**[R]**
- **Backend único: CGAL.**
- **Geogram excluído** (DEC-005). Justificativa suficiente: DEC-004, que manda evitar uma segunda biblioteca geométrica quando o CGAL já é o backend. As dependências embutidas não são o motivo da exclusão, porque podem ser desligadas no build.
- **VoroCrust:** fica **só como referência técnica e benchmark externo** (executável) para a prova de conceito 3D (P15a) e os testes de comparação. Não é backend.

## 5. O que o VMM implementa e o que delega ao backend

Esta é a questão arquitetural central do P02. A linha divisória é: **o backend responde a perguntas geométricas; o VMM constrói e possui a malha.**

### 5.1 Delegado ao backend (CGAL, atrás do concept)

| Serviço | Por que delegar | Pacote CGAL |
|---|---|---|
| **Triangulação dos sítios** → lista de pares vizinhos (i, j) | algoritmo difícil de fazer robusto; o CGAL é referência | Triangulation_2/3 (Regular_* para pesos) |
| **Predicados exatos**: orientação, in-circle/in-sphere, lado do plano, comparação de distâncias | robustez numérica | Kernel_23 (LGPL) |
| **Consultas ao domínio**: dentro/fora, distância, ponto mais próximo, interseção segmento/plano–superfície | exigem estrutura espacial (AABB) e precisão | AABB_tree, PMP (`Side_of_triangle_mesh`) |
| **Recorte exato de uma célula convexa por um domínio não convexo** (só células de contorno, em 3D) | é uma booleana exata; fazê-la à mão é o maior risco de robustez | PMP (`clip` / `corefine_and_compute_intersection`, EPECK) |
| **Leitura, reparo e validação de superfícies** (STL/OBJ) | já existe e é robusto | PMP, IO |

### 5.2 Implementado no VMM (sem backend)

| Componente | Observação |
|---|---|
| **Construção da célula convexa** por interseção de semiespaços a partir dos pares vizinhos | algoritmo curto e conhecido (é o que Voro++ e Geogram fazem); a VMMLib já tem `HalfplaneClipper2D`; usa os predicados do backend |
| **Recorte 2D pelo domínio** | a VMMLib já faz |
| **Meio × região, precedência, conformidade** (sítios espelhados + corte) | é a lógica que diferencia o VMM (DEC-001) |
| **Topologia** (faces com chave canônica, owner/neighbour, CSR) | modelo de dados do VMM |
| **Classificação** (interna, contorno, interface), views, patches | idem |
| **Métricas de VF** (vetores área, d_ij, interseção, não-ortogonalidade) | idem |
| **Reordenação, validação, determinismo, IO** | idem |

### 5.3 O concept do backend (interno)

O `GeometryBackend` é uma **policy interna de implementação**, não um parâmetro da API do usuário. O usuário não conhece nem escolhe o `CGALBackend`. A API pública expõe apenas tipos do VMM. No futuro, o concept pode ser publicado como ponto de extensão, se isso se mostrar necessário, também usando só tipos do VMM.

```cpp
// Interno ao VMM. Só tipos do VMM atravessam a fronteira; nenhum tipo do CGAL aparece aqui.
template <class B, int D>
concept GeometryBackend = requires(const B& b,
                                   std::span<const Point<D>> sites,
                                   std::span<const Real> weights,
                                   const DomainHandle<D>& domain,
                                   const ConvexCell<D>& cell,
                                   const Point<D>& p, const Point<D>& q, const Point<D>& r) {
    { b.neighbor_pairs(sites) }            -> std::same_as<NeighborPairs>;
    { b.neighbor_pairs(sites, weights) }   -> std::same_as<NeighborPairs>;
    { b.contains(domain, p) }              -> std::same_as<bool>;
    { b.clip(cell, domain) }               -> std::same_as<ClippedCell<D>>;
    { b.orientation(p, q, r) }             -> std::same_as<Sign>;   // 2D; em 3D, orientation(p, q, r, s)
};
```

**Contratos a definir no P06:**
- `neighbor_pairs`: pares `(i, j)` com `i < j`, ordenados lexicograficamente e sem repetição; com pesos, são vizinhos no diagrama de potência, e sítios ocultos são reportados à parte;
- `ClippedCell<D>`: a representação de uma célula recortada não convexa ou com várias componentes (lista de polígonos ou poliedros com rótulos de patch por face, não decomposição em convexos);
- pré e pós-condições de cada método, incluindo tolerâncias e o comportamento em casos degenerados;
- o papel do backend na multirregião: fornece consultas por região (dentro/fora, recorte pela superfície de cada região), enquanto a conformidade continua sendo responsabilidade do VMM.

### 5.4 Regras para impedir a contaminação da API

1. **Alvos CMake separados:**
   - `vmm_core`: modelo de malha, topologia, métricas, regiões, IO. **Sem nenhuma dependência do CGAL.**
   - `vmm_backend_cgal`: o adaptador. Os headers do CGAL só aparecem em `src/Backend/CGAL/*.cpp`.
2. **Instanciação explícita** dos templates para `D = 2, 3` e `Real = double`. Se outros tipos escalares forem suportados, as instanciações são acrescentadas explicitamente.
3. **Testes de CI:**
   - compilar todos os headers públicos sem o CGAL no include path;
   - compilar e rodar um exemplo mínimo que usa só `vmm_core` com um backend de teste.
4. **Tipos de fronteira próprios:** `Point<D>`, `NeighborPairs`, `ConvexCell<D>`, `ClippedCell<D>` e `DomainHandle<D>` (identificador opaco). A conversão fica no adaptador.
5. **Determinismo independente do backend:** o VMM ordena os pares (i, j) e numera células e faces por ID de sítio. A ordem interna do CGAL nunca chega ao modelo.
6. **Backend de teste:** um backend mínimo, só com geometria trivial (caixa, sítios cartesianos), testa o núcleo sem o CGAL e prova que o concept é suficiente. Ele também serve de exemplo interno de implementação.

**Ganho extra [I]:** com o CGAL só em `.cpp`, o tempo de compilação dos usuários cai muito, porque os headers do CGAL são pesados.

## 6. Licença do VMM

### 6.1 Cenários avaliados

| Cenário | Descrição | Situação |
|---|---|---|
| (a) GPL-3.0+ em tudo | o projeto inteiro sob GPL | descartado: fecha a porta para um backend permissivo futuro |
| (b) permissivo com backend não GPL | Delaunay 3D e booleanas próprias | descartado: esforço muito alto; contradiz DEC-004/005 |
| **(c) divisão core × backend** | núcleo permissivo sem CGAL; backend CGAL separado | **adotado (DEC-008)** |
| (d) licença comercial do CGAL | quem precisa compra da GeometryFactory | continua disponível para os usuários |

### 6.2 Decisão (DEC-008)

- **`vmm_core`:** **BSD-3-Clause**, sem qualquer dependência do CGAL.
- **`vmm_backend_cgal`:** usa pacotes GPL do CGAL e fica sujeito às obrigações dessas licenças quando distribuído usando a licença open source do CGAL.
- **Aplicações ou distribuições combinadas** com o backend CGAL open source devem cumprir as obrigações GPL aplicáveis.
- **Documentação:**
  - README com seção "Licensing" explicando a divisão;
  - aviso do CMake ao habilitar o backend CGAL;
  - cabeçalhos SPDX em todos os arquivos;
  - DCO para contribuições externas.
  - Nada disso é apresentado como aconselhamento jurídico.
- **Finalidade:** preservar a possibilidade de, no futuro, usar um backend permissivo sem relicenciar o core do VMM.

## 7. Dependências (DEC-009)

| Dependência | Situação |
|---|---|
| CGAL (headers) | **obrigatória**, só em `vmm_backend_cgal` |
| Boost (headers) | **obrigatória** (exigida pelo CGAL) |
| Boost.Multiprecision | **backend multiprecisão padrão** |
| GMP, MPFR | **opcionais** (desempenho) |
| TBB | **opcional**, com opção CMake `VMM_WITH_TBB` |
| GoogleTest | **só para testes** |
| spdlog, yaml-cpp | **fora do núcleo** |
| Doxygen, Sphinx, PyVista | só para desenvolvimento e documentação |
| GIS (GDAL etc.) | fora (DEC-001) |

---

## 8. Decisões (registradas em `planning/DECISIONS.md`)

| Decisão | Status |
|---|---|
| DEC-005 — Sem Geogram (justificativa: DEC-004) | **APROVADA** (corrigida) |
| DEC-006 — Divisão de responsabilidades VMM × backend | **APROVADA** |
| DEC-007 — Firewall de compilação; backend como policy interna | **APROVADA** (modificada) |
| DEC-008 — `vmm_core` BSD-3 × `vmm_backend_cgal` | **APROVADA** (modificada) |
| DEC-009 — Dependências | **APROVADA** (com a configuração do §7) |
| DEC-010 — Sequência v4 | **APROVADA** |

## 9. Sequência v4

**P15a:** usa o **CGAL**. O **VoroCrust** (executável) serve apenas de referência externa, quando possível. O P15a valida a hipótese central do backend 3D: construir as células de Voronoi convexas por interseção de semiespaços e recortar apenas as células necessárias contra o domínio 3D.

Casos obrigatórios:
- domínio convexo;
- domínio não convexo;
- arestas vivas;
- células de fronteira complexas;
- domínio com múltiplos componentes;
- casos quase degenerados;
- determinismo em relação à ordem de inserção dos sítios;
- teste de estresse de desempenho do recorte exato (EPECK) em domínios grandes.

A arquitetura 3D definitiva **não** é desenhada antes do resultado do P15a.

## 10. Riscos e mitigação

| # | Risco | Mitigação |
|---|---|---|
| 1 | O recorte exato (PMP com EPECK) fica lento em domínios grandes | recortar só as células de contorno; teste de estresse no P15a; avaliar construções filtradas |
| 2 | Robustez em casos quase degenerados (faces minúsculas, sítios quase coesféricos) | predicados exatos; casos patológicos obrigatórios no P15a e nos testes |
| 3 | Complexidade da multirregião 3D | a conformidade fica no VMM; o backend só responde consultas por região; contratos no P06 |
| 4 | VoroCrust disponível apenas como binário | usá-lo só como benchmark externo; casos analíticos como referência principal |
| 5 | Confusão de licença para os usuários | README, aviso do CMake e SPDX (DEC-008) |
| 6 | Divergência entre a versão do CGAL local e a analisada (6.2.x) | confirmar a versão no P03 |

---

## Resumo

- **CGAL:** só Kernel, Number_types, STL_Extension, Spatial_sorting, Polygon e HalfedgeDS são LGPL. As triangulações, o `Polyhedron_3`, o Voronoi_2, a PMP, a AABB_tree, o Mesh_3 e o Nef_3 são GPL-3.0+ ou comerciais.
- **Afirmações antigas:** as três foram confirmadas (periódicas cúbicas/quadradas; sphinx-gallery renderiza C++ sem executar; `extrude_mesh` desde a 4.13).
- **Geogram:** excluído, pela regra de mínimo de dependências.
- **Arquitetura:** o backend é uma policy interna que responde perguntas geométricas; o VMM possui a malha; `vmm_core` não depende do CGAL.
- **Licença:** `vmm_core` sob BSD-3; o `vmm_backend_cgal` segue as obrigações da GPL quando distribuído com o CGAL open source.
- **Dependências:** CGAL e Boost (headers), com Boost.Multiprecision como padrão.

## Decisões pendentes para o João

Nenhuma. O P02 está congelado.
