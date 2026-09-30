# P11 — Topologia, classificação, métricas de volumes finitos, reordenação

- **Data:** 2026-09-29
- **Ambiente:** o do `planning/P07_infra.md`.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Entregas

**Topologia e classificação** (`mesh/mesh.hpp`):
- `Mesh<D>` genérica em D e imutável; `from_data` valida tamanhos, ids, ordem triangular superior e faixas de patch;
- tabela de vértices e CSR face → vértices;
- intervalos `faces()`, `internal_faces()`, `boundary_faces()`, `patch_faces(p)`, `cells()` (`IdRange`, sem armazenamento);
- `CellFaceIndex`: `faces_of(c)`, `internal_cells()`, `boundary_cells()` (pedido do João, DEC-029);
- `cell_adjacency` e `sparse_pattern`, sem geometria (DEC-015);
- classificação: face interna ou de interface pelo par de regiões, face de contorno pelo patch; região e meio por célula. Não existe par (i, j) único suposto: várias faces entre as mesmas duas células são aceitas.

**Métricas** (`mesh/metrics.hpp`):
- medida e centroide da célula (por pirâmides a partir do gerador);
- vetor de área e centroide da face;
- distância entre geradores (ou ao plano, no contorno);
- ponto de interseção do segmento x_o → x_n com a face (ou projeção, no contorno);
- não ortogonalidade, skewness e razão de aspecto;
- `quality_report` por região: máximos, percentil 99 e violações dos limites configuráveis.

**Invariantes** (`mesh/invariants.hpp`): DEC-011 genérico em D. A não ortogonalidade é relatada, não verificada: é garantida por construção (DEC-032).

**Reordenação** (`reorder/reorder.hpp`):
- `Permutation` com as permutações direta e inversa explícitas;
- políticas abertas por concept: `IdentityOrdering`, `LexicographicOrdering`, `HilbertOrdering` (Morton em 3D), `RcmOrdering` (ponto de partida pseudo-periférico de George–Liu, por componente);
- `bandwidth`, `profile`;
- `renumber`, que reordena as faces e inverte as que trocam owner e neighbour.

## 2. Validação contra casos analíticos (critério de conclusão)

| Caso | Verificação **[F]** |
|---|---|
| Malha cartesiana (h = 0,1) | área h² (10⁻¹⁵), centroide = gerador, face = h, d = h, não ortogonalidade < 10⁻¹², skewness < 10⁻¹² |
| Malha hexagonal (a = 0,1), células longe da borda | área √3/2·a², 6 faces de a/√3, d = a, skewness < 10⁻¹² |
| Dois quadrados construídos à mão | medida, centroide, interseção, distância ao contorno, razão de aspecto √0,5/0,5 |

## 3. Reordenação

**[F]** Resultados em malhas aleatórias:
- **RCM**, partindo de uma numeração embaralhada (2000 células): a banda cai mais de 10 vezes, o perfil mais de 10 vezes, e o resultado fica no mesmo nível da ordem canônica. É determinístico.
- **Hilbert:** o salto médio entre células consecutivas fica abaixo de 0,1 (uma ordem aleatória dá ~0,52).
- **Renumeração:** preserva todos os invariantes; a ida e volta com a inversa recupera a malha.

**Achado:** a ordem canônica (lexicográfica por sítio) já é bem compacta em banda. O primeiro teste do RCM comparava contra ela e falhou por isso; a comparação correta é contra uma numeração embaralhada.

## 4. Testes

**[F]** 25 casos nos módulos `mesh` e `reorder`, todos passando, incluindo os erros de `Mesh::from_data` (13 mutações de uma malha válida).
