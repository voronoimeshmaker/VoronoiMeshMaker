# P01 — Estado da arte e nicho do VMM

- **Data:** 2026-09-28
- **Sequência:** v2, prompt P01
- **Destino no repositório:** `planning/P01_estado_da_arte.md`
- **Convenção:** **[F]** = fato verificado (com fonte) · **[I]** = inferência · **[R]** = recomendação · **[NV]** = não verificado.
- **Revisão:** v1.2. A v1.1 incorporou a primeira revisão (glossário, criticidade, riscos, determinismo). A v1.2 incorpora as correções do João: a lacuna fica restrita às ferramentas analisadas; binários do VoroCrust × código-fonte; statement of need restrita à difusão isotrópica; produto definido como modelo de malha de VF.
- **Nomes:** "VMM" é o projeto (VoronoiMeshMaker); "VMMLib" é a pasta `VMMLib/`, que contém a implementação atual.

## Glossário

- **Sítio (gerador):** ponto que origina uma célula de Voronoi e serve de ponto de referência do volume finito.
- **Voronoi recortado:** o diagrama de Voronoi intersectado com o domínio físico; as células que cruzam o contorno são cortadas por ele.
- **Interface conforme:** entre duas regiões, cada face da interface é compartilhada por exatamente uma célula de cada lado, com os mesmos vértices, sem nós pendentes nem sobreposição; a união das faces reproduz a superfície (3D) ou a curva (2D) da interface dentro da tolerância declarada.
- **Aresta viva (feature):** aresta (3D) ou vértice (2D) do contorno ou de uma interface em que a normal é descontínua acima de um ângulo limite; uma malha que a preserva tem arestas ou vértices de célula exatamente sobre ela.
- **Junção tripla:** curva (3D) ou ponto (2D) onde três ou mais regiões se encontram.
- **Determinismo:** a mesma entrada, a mesma configuração e a mesma semente produzem a mesma malha, bit a bit, na mesma plataforma, incluindo a numeração de células e faces.

Datas de última atividade e licenças foram conferidas clonando os repositórios em 2026-09-28 (último commit na branch principal e arquivo LICENSE).

---

## 1. Ferramentas analisadas

### 1.1 JIGSAW (Engwirda)

**[F]**
- **O que gera:** triangulações e decomposições poliédricas (Delaunay, Voronoi, diagramas de potência, Delaunay restrito) para domínios planares, superfícies e volumes ([README](https://github.com/dengwirda/jigsaw/blob/master/README.md)).
- **Tecnologia:** biblioteca C++17 header-only, com executável, API C e wrappers para MATLAB e Python.
- **Recursos por versão:** suporte a "multi-part domains with internal constraints" desde a v0.9.8; otimização ODT/CVT desde a v0.9.12 ([releases](https://github.com/dengwirda/jigsaw/releases)).
- **Uso:** é o gerador por trás das malhas do MPAS/E3SM ([MPAS-Tools](https://mpas-dev.github.io/MPAS-Tools/0.32.0/mesh_creation.html)).
- **Licença:** própria, **não OSI**. O uso privado, de pesquisa e institucional é livre; a distribuição como parte de sistema comercial só é permitida "by direct arrangement with the author" (LICENSE.md do repositório).
- **Atividade:** tag mais recente v0.9.14; último commit em 2025-08-15.

**[I]**
- O foco é a geração por refinamento de Delaunay com controle de tamanho, não a construção a partir de sítios dados pelo usuário.
- A saída é a malha (`.msh`), sem um modelo de dados de volumes finitos (owner/neighbour, vetores área, métricas).

### 1.2 VoroCrust (Sandia)

**[F]**
- Anuncia ser "the first provably correct algorithm for conforming Voronoi meshing of non-convex and non-manifold domains", com preservação de arestas vivas e sem clipping: as sementes são posicionadas em relação às superfícies ([site](https://vorocrust.sandia.gov/); [artigo](https://www.osti.gov/servlets/purl/1592222)).
- **Licença:** BSD-3 ([license](https://vorocrust.sandia.gov/about-vorocrust/license/)).
- **Disponibilidade:**
  - **binários** pré-compilados publicados para Windows 10, RHEL 7, Ubuntu 18.04–22.10 e macOS 12 ([downloads](https://vorocrust.sandia.gov/vorocrust-meshing-downloads/));
  - **código-fonte e repositório público** constam como "Coming Soon" ([getting source](https://vorocrust.sandia.gov/documentation/getting-started/getting-vorocrust-meshing-source/)).
- **Aplicação:** está sendo integrado a simulações de repositórios geológicos profundos.

**[I]** É o concorrente mais forte para o 3D multirregião conforme. O executável está disponível e pode servir de referência e benchmark. O que falta é código-fonte ou API pública para integração como biblioteca. Parece ser uma ferramenta de geração, não uma biblioteca com API de volumes finitos.

### 1.3 Geogram (Lévy, Inria)

**[F]**
- **Licença:** BSD-3.
- **Atividade:** v1.10.0 de 27/05/2026; último commit em 2026-09-24.
- **Delaunay:** Delaunay 3D paralelo e triangulações regulares, com células de Laguerre (`copy_Laguerre_cell_from_Delaunay`); `ConvexCell::clip_by_plane` ([wiki Delaunay3D](https://github.com/BrunoLevy/geogram/wiki/Delaunay3D)).
- **Voronoi restrito:** classe `GEO::RestrictedVoronoiDiagram` ([doc](http://alice.loria.fr/software/geogram/doc/html/classGEO_1_1RestrictedVoronoiDiagram.html)).
- **Voronoi recortado 3D:** calculado sobre um volume definido por malha triangulada fechada (`compute_RVD volumetric=true`), em domínios convexos e não convexos; `vorpalite profile=poly` gera malha poliédrica via CVT ([discussão #172](https://github.com/BrunoLevy/geogram/discussions/172)).
- **Exigência:** a superfície de entrada precisa ser fechada e manifold.

**[I]** É o melhor bloco de construção 3D com licença permissiva. É uma biblioteca de processamento geométrico geral: não oferece multirregião conforme nem modelo de dados de volumes finitos prontos. Não encontrei documentação de interfaces entre regiões.

### 1.4 Voro++ (Rycroft, LBNL)

**[F]**
- **Licença:** BSD-3-Clause-LBNL.
- **Atividade:** último commit em 2026-03-04.
- **Abordagem:** cálculo célula a célula em 3D (com versão 2D), incluindo recipiente periódico (`container_prd`).
- **Paredes:** plano, esfera, cilindro, cone e outras. Uma parede curva corta a célula com **um único plano por célula**. Exemplo: `wall_sphere::cut_cell_base` chama `nplane` uma vez (src/wall.cc).

**[I]**
- Fronteiras curvas ou não convexas são aproximadas.
- Não há domínio STL nem multirregião.
- A topologia global (faces compartilhadas) fica a cargo do usuário.

### 1.5 CGAL

**[F]**
- **Triangulações:** Delaunay, regulares e periódicas 2D/3D.
- **Voronoi 2D:** o adaptador `Voronoi_diagram_2` ([manual](https://doc.cgal.org/latest/Voronoi_diagram_2/index.html)) e o exemplo `print_cropped_voronoi`, que recorta por um **retângulo** ([exemplo](https://doc.cgal.org/latest/Triangulation_2/Triangulation_2_2print_cropped_voronoi_8cpp-example.html)).

**[I]** Não encontrei um componente pronto de Voronoi recortado por domínio arbitrário (2D ou 3D). O CGAL fornece as peças; a implementação atual do VMM (pasta `VMMLib/`) já monta o 2D sobre elas. O CGAL tem licenciamento por pacote e é duplo (open source ou comercial); a verificação pacote a pacote fica para o P02.

### 1.6 OpenFOAM: foamyHexMesh e polyDualMesh

**[F] foamyHexMesh:**
- Gera o dual de Voronoi de uma Delaunay 3D (CGAL), alinhado à superfície, hex-dominante, com baixa não-ortogonalidade.
- Exige superfície perfeitamente fechada ([release 2.3.0](https://openfoam.org/release/2-3-0/foamyhexmesh/)).
- Há relato de falha de compilação com o CGAL no bug tracker da Foundation ([#3496](https://bugs.openfoam.org/view.php?id=3496)).

**[F] polyDualMesh:** usa os **centros das células** do tetraedro como vértices do dual, não os circuncentros (`mesh.cellCentres()` em [polyDualMesh.C](https://github.com/OpenFOAM/OpenFOAM-5.x/blob/master/src/conversion/polyDualMesh/polyDualMesh.C)).

**[I]**
- O polyDualMesh **não é Voronoi**: não há garantia de ortogonalidade face–segmento. Mesmo assim, é o caminho mais usado para obter malhas poliédricas no ecossistema OpenFOAM.
- A manutenção do foamyHexMesh parece incerta.

### 1.7 LaGriT + LANL `voronoi` (subsuperfície)

**[F] LaGriT:**
- Licença BSD (LA-CC-15-069); Fortran/C/C++ com PyLaGriT; último commit em 2026-03-27.
- Geometrias 2D/3D "with multiple materials or regions"; controle de volume de Voronoi para FEHM, Amanzi/ATS, PFLOTRAN e TOUGH2 ([site](https://lanl.github.io/LaGriT/)).

**[F] `lanl/voronoi`:**
- Licença BSD-3; baseado em PETSc; último commit em 2022-10-17.
- Converte malhas de Delaunay (tri/tet) em tesselações de volume de controle, com saída `.stor` (FEHM), `.uge` (PFLOTRAN), `MESH` (TOUGH) e HDF5 ([README](https://github.com/lanl/voronoi/blob/master/README.md)).

**[I]** É o ecossistema mais próximo da aplicação ambiental de subsuperfície. Os volumes de Voronoi derivam de uma malha tetraédrica que precisa ser Delaunay: o usuário não controla os sítios nem recebe um Voronoi recortado explícito.

### 1.8 MRST, módulo `upr` (SINTEF)

**[F]**
- MRST é GPL-3.0 em MATLAB; último commit em 2026-09-11; o módulo `upr` está no repositório.
- Grades PEBI (Voronoi) em 2D/3D conformes a falhas, camadas e poços. Cada grade é "generated as a clipped Voronoi diagram" com pontos geradores escolhidos para garantir a conformidade ([Berge, Klemetsdal e Lie, 2019](https://link.springer.com/article/10.1007/s10596-018-9790-0)).

**[I]** É a mesma ideia de sítios posicionados junto às interfaces que propusemos para o R6. Validado na prática, mas preso ao MATLAB.

### 1.9 mf6Voronoi (Hatari Labs)

**[F]**
- Licença MIT; Python; último commit em 2026-09-17.
- Voronoi 2D para MODFLOW 6 (DISV), com refinamento em torno de rios e poços e entrada Shapefile/GeoJSON ([repo](https://github.com/hatarilabs/mf6Voronoi)).
- Usa `scipy.spatial.Voronoi` (Qhull) e shapely (geoUtils.py).

**[I]** Mostra que existe demanda na hidrologia por Voronoi 2D com refinamento por rios. É só 2D, sem multirregião conforme, e sem métricas além do que o MODFLOW exige.

### 1.10 VORO2MESH (Univ. Bolonha)

**[F]** Gerador de grades de Voronoi 3D para formações sedimentares em camadas, com TOUGH2-TMGAS. Versão beta gratuita mediante formulário; manual v1.7 ([página](https://site.unibo.it/softwaredicam/en/software/voro2mesh)).

**[I]** Nicho de reservatório/TOUGH. Não é uma biblioteca aberta.

### 1.11 Neper

**[F]** GPL-3; último commit em 2026-09-21; tesselações de Voronoi e Laguerre de policristais em 2D/3D, com células convexas e periodicidade; a malha final é de elementos finitos ([intro](https://neper.info/doc/introduction.html)).

**[I]** Tangencial: é do domínio de materiais e não gera malha de volumes finitos poliédrica.

### 1.12 TetGen, Qhull/SciPy, PolyMesher, MPAS-Tools

- **[F] TetGen:** AGPLv3 ou licença comercial ([FAQ](https://wias-berlin.de/software/tetgen/1.5/FAQ-license.html)); pode exportar o Voronoi dos vértices ([manual](https://wias-berlin.de/software/tetgen/1.5/doc/manual/manual.html)).
- **[I] TetGen:** as células exportadas não são recortadas.
- **[I] Qhull/SciPy:** Voronoi não limitado, sem clipping; serve de base para ferramentas como o mf6Voronoi.
- **[NV] PolyMesher** (Talischi et al., 2012): Voronoi 2D em MATLAB com função distância e reflexão de sementes na fronteira. A página do artigo não pôde ser lida (limite de requisições); licença não verificada.
- **[F] MPAS-Tools:** licença BSD-3; malhas planares ou esféricas via JIGSAW, com conversão para o formato MPAS ([docs](https://mpas-dev.github.io/MPAS-Tools/0.32.0/mesh_creation.html)).

---

## 2. Matriz ferramenta × critério

Legenda: ✔ sim · ◐ parcial · ✘ não · ? não verificado. **Crit.** = criticidade para o VMM e para o P02: A alta · M média · B baixa.

| Critério | Crit. | JIGSAW | VoroCrust | Geogram | Voro++ | CGAL | OF polyDual | OF foamyHex | LaGriT+voronoi | MRST upr | mf6Voronoi |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 2D | A | ✔ | ? | ◐ | ✔ | ✔ | ✘ | ✘ (quad: ◐) | ✔ | ✔ | ✔ |
| 3D | A | ✔ | ✔ | ✔ | ✔ | ✔ | ✔ | ✔ | ✔ | ✔ | ✘ |
| Sítios dados pelo usuário | A | ◐ | ✘ | ✔ | ✔ | ✔ | ✘ | ✘ | ✘ | ◐ | ◐ |
| Recorte por domínio não convexo | A | ✔ | ✔ (sem clipping) | ✔ | ◐ (aprox.) | ✘ (só peças) | n/a | ✔ | ◐ | ✔ | ✔ |
| Multirregião com interface conforme | A | ◐ (multi-part) | ✔ | ? | ✘ | ✘ | ✘ | ? | ✔ (materiais) | ✔ | ✘ |
| Arestas vivas | M | ✔ | ✔ | ? | ✘ | — | ✔ | ✔ | ? | ◐ | — |
| Domínio STL | M | ✔ | ✔ | ✔ | ✘ | ✔ (PMP) | ✔ (via tet) | ✔ | ◐ | ✘ | ✘ |
| Pesos / diagrama de potência | B | ✔ | ? | ✔ | ✔ (radical) | ✔ | ✘ | ✘ | ? | ? | ✘ |
| CVT | M | ✔ | ? | ✔ | ✘ | ◐ (Mesh_2) | ✘ | ◐ | ? | ◐ | ? |
| Métricas de VF prontas (vetor área, d_ij, não-ortog.) | A | ✘ | ? | ✘ | ◐ (vol., faces) | ✘ | ✔ (OpenFOAM) | ✔ (OpenFOAM) | ✔ (coef. `.stor`) | ✔ (no MRST) | ◐ (MODFLOW) |
| Ortogonalidade garantida (Voronoi) | A | ✔ | ✔ | ✔ | ✔ | ✔ | ✘ | ✔ | ✔ | ✔ | ✔ |
| Rótulos de patch/região | A | ◐ | ? | ✘ | ✘ | ✘ | ✔ | ✔ | ✔ | ✔ | ◐ |
| Saídas | B | `.msh` | ? | vários | texto/POV | — | OpenFOAM | OpenFOAM | FEHM/PFLOTRAN/TOUGH/AVS | MRST | Shapefile/MF6 |
| API de biblioteca | A | C++ header-only, C | ? | C++ | C++ | C++ | ✘ (utilitário) | ✘ (utilitário) | ✘ (comandos) | MATLAB | Python |
| Licença | A | própria, não OSI | BSD-3 | BSD-3 | BSD-LBNL | por pacote; GPL ou LGPL, ou comercial (P02) | GPL-3 | GPL-3 | BSD | GPL-3 | MIT |
| Atividade (último commit) | M | 2025-08 | ? | 2026-09 | 2026-03 | ativa | ativa | incerta | 2026-03 / 2022-10 | 2026-09 | 2026-09 |

**[I]** Várias células marcadas "?" ou "◐" vêm de documentação incompleta e precisam de teste prático se a decisão depender delas. As mais críticas são VoroCrust (disponibilidade, API, 2D) e Geogram (multirregião).

---

## 3. Lacunas diante dos requisitos R1–R7

Nenhuma das ferramentas analisadas reúne, **como biblioteca C++**, os seis pontos abaixo. Isso vale para o conjunto levantado, não é uma prova de ausência no estado da arte inteiro.

1. malha de Voronoi **recortada** com **sítios dados pelo usuário** (ou por geradores plugáveis);
2. **multirregião com interfaces conformes** (R5/R6), meio × região com número variável;
3. **modelo de dados de volumes finitos pronto**: owner/neighbour, vetor área, distância entre geradores, ponto de interseção, não-ortogonalidade, views por região, interface e patch;
4. **determinismo** e **API tipada** sem herança (R3), com testes por classe (R1/R2);
5. **o mesmo modelo em 2D e 3D**;
6. **licença permissiva ou compatível com uso acadêmico e industrial.**

Onde cada uma fica aquém **[I]**:

- **JIGSAW:** o mais completo em geração, mas com licença restritiva para uso comercial, sem modelo de volumes finitos e voltado ao refinamento automático, não a sítios dados.
- **VoroCrust:** resolve o 3D multirregião com garantias e tem binários públicos, mas não tem código-fonte nem API pública; não é uma biblioteca de volumes finitos.
- **Geogram:** resolve o recorte 3D com licença permissiva, mas não entrega multirregião nem modelo de volumes finitos.
- **LaGriT e MRST:** entregam multirregião e volumes finitos, mas em Fortran/comandos ou MATLAB, atrelados aos seus ecossistemas.
- **mf6Voronoi:** atende à hidrologia 2D, mas é Python, só 2D e sem conformidade.

**Determinismo [I]:** nenhuma das ferramentas documenta determinismo bit a bit (mesma entrada e semente → mesma malha e numeração) como garantia. Geradores com refinamento ou amostragem (JIGSAW, VoroCrust) e o Delaunay 3D paralelo (Geogram) tendem a depender da ordem de inserção ou do escalonamento de threads. Para quem valida solvers e reproduz resultados publicados, isso é um diferencial concreto do VMM.

**Lacuna identificada entre as ferramentas analisadas [I]:** uma **biblioteca C++ moderna, focada em volumes finitos, de Voronoi recortado multirregião conforme, com sítios controlados pelo usuário, determinística, 2D e 3D, com licença aberta**. Essa lacuna existe, mas é estreita: o valor está no modelo de dados de volumes finitos e na API, não no algoritmo geométrico em si, onde há concorrentes fortes.

---

## 4. Nichos possíveis

### N1 — Biblioteca C++ de malhas de Voronoi para volumes finitos (recomendado)

**Produto:** `domínio + sítios → modelo de malha de VF`, não `domínio + pontos → polígonos/poliedros`. O objeto central é `Mesh2D`/`Mesh3D`: células, faces, owner/neighbour, geradores, centroides de célula e de face, vetores área orientados, d_ij, pontos de interseção, não-ortogonalidade, patches, IDs de região e de interface, e as views/iteradores correspondentes. O backend geométrico é um detalhe interno; o solver só vê esse modelo.

- **Proposta:** a biblioteca que um desenvolvedor de solver de volumes finitos inclui para obter, a partir de domínio e sítios, uma malha pronta: topologia, métricas, regiões e patches.
- **A favor:**
  - lacuna clara (item 3);
  - casa com a sua experiência (volumes finitos, livro, tese);
  - a entrega 2D é realista, e o 3D reusa o modelo;
  - boa narrativa para o JOSS.
- **Contra:**
  - no 3D multirregião, disputa com VoroCrust e depende de um backend geométrico forte (CGAL ou Geogram);
  - público de desenvolvedores, não de usuários finais.

### N2 — Gerador de malhas Voronoi para hidrologia ambiental

- **Proposta:** uma ferramenta de ponta a ponta: GIS (Shapefile/GeoTIFF) → malha → formatos de solver (MODFLOW 6, PFLOTRAN, OpenFOAM, TOUGH).
- **A favor:**
  - demanda comprovada (mf6Voronoi, LaGriT, VORO2MESH);
  - usuário final claro;
  - impacto aplicado.
- **Contra:**
  - compete diretamente com ferramentas estabelecidas;
  - exige uma pilha GIS e muitos formatos, e o esforço vai para integração, não para o núcleo;
  - afasta-se da ideia de biblioteca.

### N3 — Camada de volumes finitos sobre motores existentes

- **Proposta:** não implementar o recorte 3D. Consumir a saída de Geogram, JIGSAW ou VoroCrust e entregar o modelo de volumes finitos, a conformidade de regiões, as métricas e a exportação.
- **A favor:** menor esforço, e o foco fica no que é único.
- **Contra:**
  - dependência de terceiros, com licenças heterogêneas (a do JIGSAW é restritiva);
  - menos controle sobre robustez e determinismo;
  - é mais difícil defender como contribuição de software.

### Recomendação [R]

**N1**, com duas salvaguardas:

1. **Arquitetura aberta a N3 no 3D.** O backend geométrico é uma policy (R3). Se, no P02 e depois, o Geogram (BSD) ou o VoroCrust se mostrarem melhores para o recorte e a conformidade 3D, o VMM os usa sem mudar a API.
2. **Casos ambientais como vitrine e adaptadores de saída**, sem virar N2. Os problemas-âncora do P04 viram exemplos da galeria; os formatos de solver entram como escritores (plugins), não como núcleo.

---

## 5. Rascunho da *statement of need* (N1)

Em inglês, formato JOSS; revisar depois do P02 (licença) e do P04 (casos-âncora).

> **Statement of need.** Finite volume (FV) methods on Voronoi (PEBI) meshes are attractive for environmental and subsurface flow because the segment joining neighbouring generators is orthogonal to their shared Voronoi face, a geometry particularly well suited to two-point flux approximations for isotropic diffusion and related FV discretisations (anisotropic problems require additional conditions, such as K-orthogonality). In practice, however, obtaining an FV-ready Voronoi mesh remains cumbersome. General geometry libraries (CGAL, Geogram, Voro++) provide Delaunay/Voronoi building blocks but not a finite-volume data model. Mesh generators (JIGSAW, VoroCrust, foamyHexMesh) produce meshes but not a programmable library interface, and in some cases carry restrictive licenses. Domain-specific pipelines (LaGriT, MRST, mf6Voronoi) are tied to Fortran/command-driven, MATLAB or Python ecosystems and to particular simulators. Moreover, OpenFOAM's widely used `polyDualMesh` builds the dual from cell centres rather than circumcentres, so the resulting cells are not Voronoi cells and orthogonality is not guaranteed.
>
> *VoronoiMeshMaker* is a C++ library that turns a user-defined domain and a set of generators (or pluggable generator strategies) into a clipped Voronoi mesh ready for FV solvers. Domains are composed of multiple regions (e.g. water, soil, air) with conforming interfaces. The library delivers cells, faces, region and patch labels, together with the geometric quantities FV schemes need: face area vectors, generator distances, face–segment intersection points and non-orthogonality measures. Mesh generation is deterministic, so published results can be reproduced exactly, and the same data model serves 2D and 3D. It targets researchers and developers who build FV solvers and need full control over generator placement and mesh semantics. It complements the mesh generators listed above rather than replacing them.
>
> *(Para as seções "Functionality"/"Design" do artigo, não para a statement of need: modelo de dados orientado a dados, extensão por templates e concepts sem herança, testes unitários por classe.)*

---

## 6. Riscos e mitigação

| # | Risco | Impacto | Mitigação |
|---|---|---|---|
| 1 | O backend 3D (CGAL ou Geogram) não resolve bem o recorte não convexo ou as interfaces | alto | backend como policy (DEC-002); comparação no P02; prova de conceito P15a antes da arquitetura 3D |
| 2 | Licença do CGAL incompatível com o público-alvo (pacotes GPL) | alto | verificação pacote a pacote no P02; cenário de núcleo permissivo com backend opcional |
| 3 | VoroCrust sem código-fonte ou API pública (só binários) | médio | usá-lo como referência técnica e benchmark; só considerar como backend se houver integração sustentável (P02) |
| 4 | Geogram sem multirregião conforme | médio | implementar a conformidade no próprio VMM (sítios espelhados + corte) sobre o recorte do backend |
| 5 | Robustez do recorte não convexo (casos quase degenerados, faces minúsculas, junções triplas) | alto | predicados exatos; testes de propriedade e casos patológicos desde a entrega 2D; tolerâncias explícitas |
| 6 | O nicho N1 é estreito e atrai poucos usuários | médio | casos ambientais como vitrine; escritores para os formatos dos solvers mais usados (P04) |

---

## 7. Entradas propostas para `planning/DECISIONS.md`

```
## DEC-001 — Nicho do VMM
- Data: 2026-09-28
- Origem: P01
- Status: PROPOSTA
- Decisão: O VMM é uma biblioteca C++ de malhas de Voronoi recortadas, multirregião conformes, prontas para
  volumes finitos, com sítios controlados pelo usuário (nicho N1). Não é um gerador GIS de ponta a ponta.
- Justificativa: nenhuma das ferramentas analisadas reúne essas características (P01 §3); alinhada à competência do autor; entrega 2D viável.
- Consequências: P04 foca em casos-âncora como exemplos e em escritores de formato como plugins; P02 avalia o
  backend como policy substituível.

## DEC-002 — Backend 3D como policy substituível
- Data: 2026-09-28
- Origem: P01
- Status: PROPOSTA
- Decisão: O recorte e a conformidade 3D ficam atrás de um concept de backend. O P02 avalia CGAL e Geogram
  e investiga o VoroCrust (inicialmente como referência técnica e benchmark) antes de qualquer implementação 3D.
- Justificativa: há concorrentes fortes no algoritmo 3D; o valor do VMM está no modelo de VF e na API.
- Consequências: P02 ganha a tarefa de comparar tecnicamente CGAL × Geogram para Voronoi recortado 3D.
```

---

## 8. Proposta de mudança na sequência (v3)

- **P02:** acrescentar uma comparação técnica CGAL × Geogram para Voronoi recortado 3D (funções disponíveis, multirregião, robustez, desempenho, licença) e verificar se o código do VoroCrust está obtenível.
- **Antes do P15 (Fase 3):** incluir uma **prova de conceito** curta (uma pequena implementação descartável) de recorte 3D com o backend escolhido, antes da arquitetura 3D definitiva.

---

## Resumo

- Levantadas 15 ferramentas; licença e atividade conferidas nos repositórios.
- Nenhuma oferece, como biblioteca C++, Voronoi recortado multirregião conforme, com sítios do usuário e modelo de volumes finitos pronto.
- Os concorrentes mais fortes por área:
  - 3D conforme: VoroCrust;
  - bloco 3D permissivo: Geogram;
  - geração geofísica: JIGSAW, com licença restritiva;
  - pipelines de subsuperfície: LaGriT e MRST.
- O `polyDualMesh` do OpenFOAM não é Voronoi (usa centros das células).
- **Recomendação:** nicho N1 (biblioteca de volumes finitos), com backend 3D substituível e casos ambientais como vitrine. O VMM **complementa** os geradores existentes, não os substitui.
- **Status (28/09):** N1, DEC-002 e sequência v3 **aprovados** pelo João (DEC-001 a DEC-003 em `planning/DECISIONS.md`).
- Pontos não verificados: PolyMesher; API e suporte 2D do VoroCrust; multirregião no Geogram.

## Decisões pendentes para o João

1. **Nicho:** N1 (recomendado), N2 ou N3?
2. **DEC-002:** aprovar o backend 3D como policy, com avaliação CGAL × Geogram no P02?
3. **Sequência v3:** aprovar as duas mudanças do §8?
