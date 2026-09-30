# P19 — Documentação e release 3D (entregas b, c e d)

- **Data:** 2026-09-30
- **Ambiente:** WSL `Ubuntu-26.04-Test`; g++ 15.2.0; CMake 4.2.3; CGAL 6.2.1; Boost 1.92.
- **Estado:** **aprovada como 0.2.0** (DEC-038, 30/09). Falta o push, feito pelo João (AGENTS.md); a CI e o Pages
  rodam pela primeira vez com ele.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. O que a Fase 3 entregou

| Etapa | Relatório | Resultado |
|---|---|---|
| P15a | `P15a_relatorio.md` | prova do recorte 3D: células convexas por semiespaços, recorte exato só no contorno |
| P15 | `P15_arquitetura_3d.md` | arquitetura; DEC-035 (célula partida inteira), DEC-036 (domínio por superfície triangulada) |
| P16 | `P16_relatorio.md` | 3D com uma região, domínio analítico (entrega b); DEC-037 (sem FMA) |
| P17 | `P17_relatorio.md` | domínio STL, reparo, recorte local (entrega c) |
| P18a | `P18a_relatorio.md` | prova da interface 3D conforme |
| P18 | `P18_relatorio.md` | várias regiões, buracos, pares espelhados, âncora A3 (entrega d) |
| P19 | este documento | documentação, CI, fonte adaptativa 3D, revisão para release |

**O que o P19 acrescentou:**
- **Fonte adaptativa 3D.** `AdaptiveOctreeSource3D`, o equivalente 3D da fonte por quadtree do 2D: a revisão
  mostrou que o 3D não tinha espaçamento variável, e o A3 do P04 pede 0,5 m junto ao leito e 10 m no ar.
- **Documentação.**
  - O guia de uso da malha passa a valer para `Mesh<D>` em 2D e 3D.
  - A página de arquivos inclui STL e os poliedros do VTU.
  - O guia 3D cobre domínio, sítios, construção, STL, várias regiões e pares espelhados.
  - A galeria tem três exemplos 3D (bloco em L, STL, A3), com corte vertical e sem o ar quando há vários meios.
  - Tudo em português e inglês.
- **CI.** Os testes de integração pesados levam o rótulo `slow` (23 testes; o de 1000 configurações levou 764 s
  no build de cobertura local). O job com sanitizadores pula esses testes, os demais rodam tudo, e o limite por
  teste subiu para 3600 s.

## 2. Revisão contra a linha de base, no 3D

| Req. | Estado no 3D | Evidência |
|---|---|---|
| R1/R2 teste por classe, árvore espelhada | **OK** | `check_requirements.py` sem problemas; triviais listadas |
| R3 sem virtual, herança ou enum de despacho | **OK** | *visitors* do CGAL (corefinement, autorrefinamento) por composição |
| R4 grafo de módulos | **OK** | nenhuma dependência nova entre módulos |
| R5/R6 regiões, meios, interfaces conformes | **OK** | precedência 3D, refinamento comum, área de cada interface nos invariantes (A3: 20 000 m² exatos) |
| R7 malha imutável | **OK** | o mesmo `Mesh<D>` |
| R9 adjacência | **OK** | `cell_adjacency`, `sparse_pattern`, `CellFaceIndex` genéricos |
| R14 volumes finitos e qualidade | **OK, com ressalva** | métricas genéricas; faces de interface não ortogonais sem pares (até 1,8 rad); com `InterfacePairs3D` a média cai para 0,01–0,05 rad |
| R16 invariantes | **OK** | em todo teste 3D e nos benchmarks B4–B7 |
| R17 tolerâncias relativas | **OK** | escalas 10⁻³ a 10⁶ com a mesma topologia (P16, P18a) |
| R18 determinismo | **OK** | malha idêntica bit a bit para outra ordem dos sítios, com uma ou várias regiões; saída do CGAL canonizada |
| R19 firewall | **OK** | CGAL só em `vmm_backend_cgal`; `PreparedDomain3` opaco |
| R21 dependências | **OK** | nenhuma nova (CGAL: PMP, AABB, autorrefinamento) |
| R23 SPDX | **OK** | verificador |
| R24 build | **OK** | `-Werror`; Release com LTO e `-march=native` (DEC-034), sem FMA (DEC-037) |
| R25 cobertura | **OK** | ver §6 |
| R26 casos patológicos | **OK** | grades cosféricas, sítios a 10⁻⁹ L do contorno, cunhas de 3°, faces coplanares entre camadas, pares espelhados em esfera |
| R27 benchmark | **OK** | B4–B7 (§3) |
| R28 saída | **OK** | nativo 3D, VTU com `VTK_POLYHEDRON`, STL de entrada e saída |
| R29/R30 documentação e portão | **OK** | site pt/en com o portão de testes; seções PETSc nas funções 3D principais |
| R12 compiladores | **Parcial** | GCC 15.2 e 14.3 localmente; Clang 18 só na CI, que não rodou |

## 3. Metas do P04 para o 3D

| Meta | Valor | Medido | Caso |
|---|---|---|---|
| Tempo, 10⁶ células, 1 thread | ≤ 10 min | 3,2 min (191 s) | B4, cubo |
| Memória no pico | ≤ 4 KB/célula | 1,37 KB/célula | B4 |
| Domínio STL grande | — | 80 mil células em 52 s num terreno de 316 mil triângulos; 3,5 ms por célula recortada | B6 |
| A3 | — | 104 mil células em 107 s, 2,9 KB/célula | B7 |

## 4. API pública 3D proposta para congelar

**Estável:**
- fachada `generate_mesh_3d`, `MeshRequest3D`, `MeshResult3D`;
- `TriangleSurface`, `Box3`, `repair_surface`, `TriangleSoup`;
- as formas 3D e `ShapeRegistry3D`, `Declaration3D` (regiões e buracos) e `Partition3D` (leitura);
- `SiteSet3D` e as fontes 3D, `InterfacePairs3D`, `generate_sites_3d`, `validate_sites_3d`;
- `build_mesh_3d`, `invariant_reference(Partition3D)`;
- `read_stl`, `read_stl_surface`, `write_stl` e `write_vtu(Mesh3D)`.

**Detalhe interno, sem garantia na 0.x** (como o backend 2D no P14):
- `backend/backend3d.hpp` (`Backend3D`, `LabelledPolyhedron3`, `CellClip3`, `PreparedDomain3`, `FaceLabel`);
- os campos de `BuildStats3D`.

## 5. Limites conhecidos

1. **Degenerescência maciça:** centenas de sítios cosféricos podem produzir células inválidas na construção em
   double. Os casos testados passam, inclusive grades cartesianas; a proteção geral seria um recorte convexo exato
   por célula (P18 §5).
2. **Qualidade das faces de interface sem pares:** use `InterfacePairs3D` quando o solver for sensível à não
   ortogonalidade (P18 §5).
3. **Uma thread:** o recorte exato domina o tempo nos domínios com muitas superfícies. A paralelização foi adiada
   pelo João.
4. **CAD** (STEP, IGES) não é lido: é preciso exportar para STL. `Extrusion` não aceita buracos no contorno; use
   `add_hole`.
5. **Memória do domínio:** com poucas células por triângulo (STL fino, malha grossa), as estruturas exatas do
   domínio dominam o pico (B6: 5,9 a 7,9 KB por célula).

## 6. Verificação (P19)

**[F] Depois da última mudança do P19:**
- 251/251 testes em Debug, em Release e no build de cobertura;
- 228 testes rápidos, sem o rótulo `slow`, que é o que o job com sanitizadores roda;
- cobertura de 97,4% das linhas e 91,7% dos ramos, nenhum arquivo abaixo do mínimo;
- verificação estática sem problemas;
- documentação gerada em português e inglês, com o portão de testes (251/251).

## 7. Para publicar (depende do João)

1. **Commit e push.** A CI (GCC 14 Debug e Release, Clang 18, sanitizadores, cobertura) e o Pages rodam pela
   primeira vez; qualquer falha lá precisa de uma volta.
2. **Numeração.** A DEC-025 previa 0.2 para os escritores MODFLOW, PFLOTRAN e TOUGH (P14a, não feito), e 0.3,
   0.4 e 0.5 para as entregas 3D, que estão prontas. Opções:
   - **(a)** publicar o 3D já (por exemplo como 0.2, com o roteiro renumerado numa nova DEC) e deixar os escritores
     para depois;
   - **(b)** fazer o P14a antes e publicar na ordem da DEC-025.

   **[R]** (a): o 3D está verificado e os escritores não dependem dele.
3. **JOSS:** o artigo pode citar 2D e 3D; o checklist do P14 §5 continua valendo.

## 7a. Preparação da 0.2.0 (feita)

- **Versão:** 0.2.0 no CMake da raiz e no pacote (`VoronoiMeshMakerConfigVersion`, compatível na mesma versão
  menor), na documentação e no início rápido (`find_package(VoronoiMeshMaker 0.2 REQUIRED)`).
- **Roteiro:** DEC-038 (3D na 0.2; escritores na 0.3).
- **`CHANGELOG`:** uma seção 0.2.0 com as entregas b, c e d.
- **[F] Teste do pacote instalado:** build com `-DVMM_ENABLE_LTO=OFF -DVMM_ENABLE_NATIVE_ARCH=OFF` (a configuração
  para distribuir, DEC-034) e `cmake --install` num prefixo temporário. Um projeto externo com
  `find_package(VoronoiMeshMaker 0.2 REQUIRED)` compilou, ligou `VoronoiMeshMaker::vmm` e `::vmm_io` e gerou uma
  malha 3D de duas regiões (1597 células, 4499 faces de interface).

## 8. O que sobra depois do P19

- **P14a:** escritores MODFLOW 6, PFLOTRAN e TOUGH (0.3 pela DEC-038).
- **Facilidade de uso:** ajudante que troca `Result` por exceção, executável com arquivo de configuração ou
  bindings em Python (este precisa de uma DEC).
- **Paralelização:** recorte e montagem das células.
- **1.0:** congelamento da API.
