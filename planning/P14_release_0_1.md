# P14 — Release 0.1 (entrega (a))

- **Data:** 2026-09-29
- **Ambiente:** o do `planning/P07_infra.md`.
- **Estado:** **candidata**. A publicação depende da sua aprovação (critério de conclusão do P14). Nada foi commitado nem publicado.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Revisão contra a linha de base (P05 §2)

| Req. | Estado | Evidência / observação |
|---|---|---|
| R1 teste por classe | **OK** | `check_requirements.py`: toda classe pública não trivial tem `ut_<Classe>.cpp`; triviais listadas |
| R2 árvore espelhada | **OK** | verificador; `tests/integration` para integração |
| R3 sem virtual/herança/despacho | **OK** | verificador sem ocorrências; enums só de dado |
| R4 grafo de módulos | **OK** | tabela de dependências permitidas, sem ciclos |
| R5 domínio multirregião, meios | **OK** | P09; A1 (2 regiões do mesmo meio em contato), A2 (sem contato) |
| R6 interfaces conformes e rotuladas | **OK** | refinamento comum; invariante de conformidade; comprimentos analíticos |
| R7 malha imutável | **OK** | `Mesh<D>` só com acesso `const`, construída por `from_data` |
| R8 sem PETSc | **OK** | verificador |
| R9 adjacência sem geometria | **OK** | `cell_adjacency`, `sparse_pattern`, `CellFaceIndex` |
| R10 subsistema de erros | **OK** | códigos estáveis, entidade, `source_location`, catálogo pt/en testado |
| R11 `std::expected` | **OK** | toda a API pública devolve `Result`; `Exception` só para invariante interno |
| R12 C++23, GCC 14 / Clang 18 | **Parcial** | C++23 com GCC 15.2 e GCC 14.3 verificados localmente (154/154 testes com g++-14, Release, `-Werror`); Clang 18 só na CI, que ainda não rodou |
| R13 nome e namespace | **OK** | `vmm`; nenhum "VoronoiGridMaker" em `vmm/` |
| R14 modelo de volumes finitos | **OK** | P11 |
| R15 precedência e validador | **OK** | P09 |
| R16 invariantes DEC-011 | **OK** | todos os testes de integração e o benchmark; 1000 configurações aleatórias; ortogonalidade estrutural (DEC-032) |
| R17 tolerâncias relativas | **OK** | `Tolerance`; escalas 10⁻³ e 10⁶ com a mesma topologia |
| R18 determinismo | **OK** | bit a bit entre 5 ordens de inserção; checksum de topologia no benchmark |
| R19 firewall | **OK** | 26 headers sem CGAL/Boost/GMP (`-M`), com controle negativo |
| R20 `Real = double` | **OK** | |
| R21 dependências | **OK** | `find_package` só da lista permitida |
| R22 versões | **OK** | configure imprime; golden files registram |
| R23 SPDX | **OK** | verificador (GPL só no backend CGAL e no gerador de golden) |
| R24 build limpo | **OK** | `-Werror`, sem `fast-math`, `NATIVE_ARCH`/`LTO` OFF, saídas fora da árvore; binários retirados do índice (em *staging*) e `.gitattributes` com LF |
| R25 cobertura | **OK** | 97,4 % / 91,9 %; 1 exceção declarada |
| R26 propriedade, golden, patológicos | **OK** | 1000 configurações; O1–O4; sítios colados, cocirculares, regiões finas, escalas extremas |
| R27 benchmark | **OK** | B1–B3 (P07 §4) |
| R28 saída da 0.1 | **OK** | nativo e VTU; OpenFOAM removido (DEC-030) |
| R29 documentação | **OK** | site pt/en e galeria gerados localmente; seções PETSc nas funções principais; workflow de publicação no Pages (roda após o push) |
| R30 gate da galeria | **OK** | `build_docs.sh` roda os testes antes |

## 2. Problemas por severidade

**Alta**
- Nenhum encontrado.

**Média**
1. **CI nunca executada** (R12). GCC 14.3 já passou localmente (154/154); Clang 18 continua sem verificação até a CI rodar.
2. ~~Memória~~: resolvida (673 B/célula depois da otimização da montagem).
3. ~~DEC-031~~: rejeitada pelo João e substituída pela DEC-032 (ortogonalidade garantida por construção, ângulo só relatado).

**Baixa**
4. **Documentação:** avisos do Breathe com concepts; figuras com matplotlib em vez de PyVista.
5. ~~Higiene do repositório~~: feita (P07 §2); falta o seu commit.
6. **Fragmentos de célula:** ficam como célula de várias peças. Todos os invariantes continuam válidos, mas não há teste dedicado que force esse caso.
7. **Crescimento N log N:** não medido em série.

## 3. API pública congelada da 0.1

Headers em `vmm/include/vmm/`, namespace `vmm`:
- **Fachada:** `vmm.hpp` (`generate_mesh_2d`, `MeshRequest2D`, `MeshResult2D`);
- **Módulos:** `core/`, `error/`, `geometry/`, `domain/`, `sites/`, `backend/`, `voronoi/`, `mesh/`, `reorder/`, `io/`.

**[R]** Congelar como estão, com três ressalvas marcadas como detalhe interno (sem garantia de estabilidade na 0.x):
- `backend/backend2d.hpp`: o struct de callables pode mudar com o 3D;
- `EdgeLabel`;
- `BuildStats2D`.

## 4. Arquivos de release

- **Criados:** `vmm/README.md`, `vmm/CHANGELOG.md`, `vmm/CONTRIBUTING.md` e `vmm/LICENSE` (BSD-3-Clause, com a nota da GPL no backend).
- **Não alterados:** o `README.md` da raiz, que tem alterações suas.
- **Não criados:** arquivos de licença na raiz, porque a escolha final é sua (DEC-008).

## 5. Checklist do JOSS

| Item | Estado |
|---|---|
| Statement of need | a escrever (base: P01 e DEC-001, nicho N1) |
| Comparação com o estado da arte | P01 |
| Testes automatizados | sim (153 na biblioteca nova, CI definida) |
| Documentação e exemplos | sim (site local, galeria com 3 exemplos) |
| Instalação | `cmake --install`, `find_package(VoronoiMeshMaker)` |
| Licença OSI | BSD-3-Clause (núcleo) e GPL-3.0 (backend) |
| Repositório público com issues | sim (GitHub) |
| `paper.md` | a escrever |

## 6. Para a 0.2

- escritores MODFLOW 6 (DISV/DISU), PFLOTRAN e TOUGH (DEC-019, P14a);
- documentação em inglês, seções PETSc por símbolo, publicação no GitHub Pages;
- figuras com PyVista na CI;
- teste dedicado de fragmentos de célula e política opcional de fusão;
- redução de memória na montagem (meta de 1 KB/célula);
- ~~retirada da VMMLib e de `VoronoiGridMaker/`~~: decidida na DEC-033 (30/09); diretrizes em `vmm/docs/guidelines.rst`.
- otimizador de Lloyd (CVT), não migrado da VMMLib (referência no histórico do git, commit d189461).
- paralelização da construção (recorte das células e montagem), adiada pelo João em 30/09 para uma etapa posterior; hoje o build roda em 1 thread e o gargalo é o recorte exato do CGAL (P07 §4).

## 7. Revisão da sequência para a Fase 3

**[R]** Manter o P15a antes da arquitetura 3D, reaproveitando o que já existe:
- `Mesh<D>`, `check_invariants<D>`, `compute_metrics<D>` e `renumber<D>` já são genéricos em D, e `face_geometry` já tem a versão 3D;
- o protótipo 3D do P05a (`prototypes/P05a`) mostrou células por semiespaços com Euler e fechamento corretos.

**Atualização 30/09:** o P15a foi executado; ver `planning/P15a_relatorio.md` (hipótese confirmada, 37/37 verificações).

O P15a deve focar no que falta: recorte por domínio não convexo em 3D, vértices canônicos em 3D (as duas cópias de uma face diferiram em até 3,6·10⁻¹¹ no P05a) e a garantia estrutural da DEC-032 em 3D (faces como pedaços de bissetor escolhidos pelo backend). Nenhuma nova DEC é necessária para isso.

## 8. Decisões pendentes para o João

1. **Aprovar a publicação da 0.1?** Recomendação: aprovar depois de a CI rodar verde no GitHub.
2. ~~DEC-031~~: resolvida (rejeitada; DEC-032 aprovada).
3. ~~Meta de memória~~: não é mais necessária (673 B/célula, dentro de 1 KB).
4. ~~Retirar a VMMLib e `VoronoiGridMaker/`~~: decidido, retirar agora (DEC-033).
5. ~~Licenças na raiz~~: criados `LICENSE` (BSD-3-Clause), `COPYING` (GPL-3.0) e `LICENSES.md` (qual parte usa qual).
