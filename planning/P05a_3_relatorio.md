# P05a.3 — Relatório: prova 3D mínima

- **Data:** 2026-09-29
- **Sequência:** v5.2, prompt P05a, iteração P05a.3
- **Ambiente:** o mesmo do `planning/P05a_1_relatorio.md` §1.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Caso e método

- Caixa unitária, uma região, 500 sítios por *dart throwing* (espaçamento mínimo 0,5·h e margem 0,25·h, com h = 500^(−1/3)), semente fixa.
- Vizinhos pela Delaunay 3D do CGAL (backend). Cada célula é a caixa recortada pelos semiespaços dos vizinhos (`src/core/voronoi3d.cpp`, sem CGAL).
- Os pontos de corte de uma aresta são calculados a partir do par de extremos em ordem lexicográfica. Assim, as faces vizinhas da mesma célula recebem vértices idênticos, e a característica de Euler pode ser checada por identidade exata.
- A face compartilhada é gravada com a cópia do owner, e a diferença para a cópia do neighbour é medida.

**Arquivos:** `include/p05a/voronoi3d.hpp`, `src/core/voronoi3d.cpp`, `apps/p05a_3_box3d.cpp`. **Não houve mudança em `mesh.hpp`, `invariants.hpp` ou `csr.hpp` para o 3D** **[F]**: o mesmo `PolyMesh<D>` e o mesmo `check_invariants<D>` servem às duas dimensões. A única função que muda com D é `face_geometry` (segmento × polígono).

## 2. Resultados

**[F]** 5 execuções (escala 1, 10⁻³ e 10⁶; ordens 0, 1 e 2), todas PASS. Execução de referência:

```
3D scale 1, order 0: cells 500 | faces internal 3204 interface 0 boundary 336
  measure rel. error: total 0.000e+00 | worst region 0.000e+00 | worst cell (vs polygon) 5.204e-18
  boundary rel. error 4.441e-16
  closure max |sum S_f| 1.156e-16 (tol 3.000e-12) | non-orthogonality max internal 6.958e-13 rad
  iteration: internal faces 3204 | boundary faces 336 | internal cells 247 | boundary cells 253
  build: Euler failures 0 | unmatched faces 0 | dropped tiny faces 0 | max |S_ij + S_ji|/|S_ij| 3.625e-11 | smallest internal face 2.458e-10 L^2
  faces per cell 13.49 | memory of the mesh arrays 1013 bytes/cell
```

- Volume total = 1 (erro relativo ≤ 1,2·10⁻¹⁵ nas três escalas).
- Fechamento de todas as células.
- V − E + F = 2 em todas as células.
- Owner/neighbour consistentes.
- A mesma topologia canônica (3540 faces) em todas as variações.

## 3. Achados

1. **[F] A tolerância de fechamento precisa da dimensão certa:** `|Σ S_f|` tem unidade L^(D−1). O verificador usa 10⁻¹² · L^(D−1); o plano dizia "10⁻¹² L". **[R]** Corrigir a redação ao implantar a DEC-020 no P07.
2. **[F] Faces minúsculas:** a menor face interna tem 2,5·10⁻¹⁰ L². As duas cópias de uma face diferem até 3,6·10⁻¹¹ em termos relativos (o pior caso é justamente a face minúscula). Ainda assim, a não ortogonalidade ficou em 7·10⁻¹³ rad. **[I]** Com mais sítios ou casos quase cosféricos, faces ainda menores podem violar o limite de 10⁻⁸ rad da DEC-020, que é mal posto para faces de diâmetro ≈ ε/10⁻⁸. O P06 precisa de uma política para faces degeneradas (colapso ou limiar relativo), e os vértices canônicos (como no 2D) eliminam a diferença entre as cópias.
3. **[F] Memória:** 1013 bytes/célula nos arrays da malha, com vértices guardados por face, sem tabela de vértices (meta da DEC-020: até 4 KB/célula em 3D). Não é benchmark.
4. **Fora do escopo** (P15a): domínio não convexo, multirregião e recorte por superfície em 3D.
