# P10 — Sítios, Voronoi e recorte 2D multirregião conforme

- **Data:** 2026-09-29
- **Ambiente:** o do `planning/P07_infra.md`.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Entregas

**Sítios** (`sites/`):
- `SiteSet` em SoA, com pesos reservados a custo zero, e `validate_sites`, que rejeita sítio não finito, fora da região ou a menos de 10⁻¹² L da fronteira, duplicado, e componente sem sítio;
- fontes, todas determinísticas pela semente com o gerador portável e uma semente própria por componente:
  - `UniformRandomSource`: distância mínima e margem;
  - `AdaptiveQuadtreeSource`: espaçamento variável h(x), O(N);
  - `RandomCountSource`;
  - `CartesianGridSource` e `HexagonalGridSource`;
  - `ExplicitSites`;
- `InterfacePairs`: pares espelhados ao longo das interfaces (E2 da DEC-028), com exclusão dos sítios próximos.

**Malha** (`voronoi/builder2d.hpp`):
1. sítios canonizados na entrada (região, depois posição lexicográfica), o que torna o resultado independente da ordem;
2. Delaunay por região, feito pelo backend;
3. célula convexa por semiplanos rotulados;
4. **caminho rápido:** sem segmento da região por perto, a célula dispensa o recorte exato;
5. recorte exato rotulado, feito pelo backend;
6. vértices canônicos recalculados a partir dos rótulos;
7. faces internas pela chave (i, j) e interface por refinamento comum;
8. tabela de vértices com fusão a 10⁻¹² L e faces na ordem da DEC-029.

## 2. Critério de conclusão: ≥ 1000 configurações aleatórias

**[F]** `tests/integration/RandomConfigurations`:
- **Geração:** sementes 1 em diante; domínio retangular de 1 a 4 × 1 a 4; de 0 a 3 camadas extras (círculos, retângulos ou buracos); espaçamento diferente por região; pares de interface em ¼ dos casos.
- **Resultado:** **1000 configurações construídas, todas com os invariantes da DEC-011**; 257 delas com pares espelhados. Outras 15 foram rejeitadas de forma válida (`RegionWithoutSites`: região fina demais para o espaçamento). Tempo: 6,6 min em Debug.

## 3. Outros testes

| Teste | Resultado **[F]** |
|---|---|
| Quadrado com buraco e interface em "L", E1 e E2 | invariantes OK |
| Caminho rápido × recorte exato | malhas idênticas bit a bit |
| Escalas 10⁻³ e 10⁶; 5 ordens de inserção | mesma topologia canônica; pontos idênticos bit a bit entre ordens (R18) |
| A1 com quadtree adaptativa | invariantes OK, > 1000 células |
| Grade cartesiana 40 × 30 (cocircular) | todas as células com 4 faces, invariantes OK |
| Casos patológicos de sítios | sítio no buraco, na região errada, exatamente na interface, colado a 10⁻⁹ da interface (aceito), duplicado, não finito, região sem sítios |
| Região fina (0,02 de espessura) | recebe > 50 sítios |

## 4. Oráculo da VMMLib (DEC-011)

**[F]** `tests/integration/Oracle`, com os golden files gerados pela VMMLib (P07). Mesmos sítios; comparação célula a célula pela identidade do sítio:

| Caso | Células | Pares de vizinhos (novo / VMMLib) | Faltando / sobrando | Maior diferença relativa de área |
|---|---|---|---|---|
| O1 retângulo 4 × 3, 2000 aleatórios | 2000 | 5839 / 5839 | 0 / 0 | 4,4·10⁻¹⁴ |
| O2 grade 40 × 30 | 1200 | 2330 / 2330 | 0 / 0 | 1,1·10⁻¹⁴ |
| O3 grade hexagonal | 1343 | 3883 / 3883 | 0 / 0 | 1,3·10⁻¹⁴ |
| O4 quadrado, 10⁴ aleatórios | 10 000 | 29 653 / 29 653 | 0 / 0 | 1,1·10⁻¹³ |

## 5. Achados e correções durante a execução

1. **[F] Laço infinito:** `UniformRandomSource` com `min_distance_fraction(0)` nunca terminava, porque todo candidato era aceito e o limite de falhas nunca era atingido. Agora devolve `InvalidSpacing`; para sorteio sem distância mínima existe `RandomCountSource`.
2. **[F] Polígono auto-sobreposto (semente 125):** pares espelhados geram quádruplas exatamente cocirculares. O recorte por semiplanos em `double` deixava dois vértices a 10⁻¹⁶ com uma aresta invertida, e o CGAL recusava o polígono (precondição de Boolean_set_operations_2). Duas correções:
   - vértices consecutivos a menos de 10⁻¹² L passam a ser um só (DEC-020) antes do backend;
   - o backend passou a converter exceções do CGAL em `BackendFailure`, com a célula envolvida.
3. **[F] Vértices de grau > 3:** não têm construção canônica única. O casamento das duas cópias de uma face passou a usar a tolerância de ponto, e a deduplicação unifica os vértices.
4. **[F] Não ortogonalidade de faces quase degeneradas:** na semente 599 (1,4·10⁻⁸ rad) e no benchmark (1,1–1,4·10⁻⁸) apareceram faces de 2·10⁻⁸ com ângulo acima de 10⁻⁸, com todos os outros invariantes em 10⁻¹⁶. **[I]** A direção de uma face de tamanho s só é conhecida até ε·L/s. **DEC-031 (PROPOSTA):** o limite vale para faces de tamanho ≥ max(10⁻⁶·d, 64·ε·L/10⁻⁸); as menores são contadas e relatadas à parte.
5. **[I] Colapso de faces curtas:** o P04 pede nenhuma face abaixo de 10⁻³ do espaçamento local depois do colapso de arestas curtas. Isso é incompatível com a meta de 10⁻⁸ rad, porque mover um vértice de 10⁻³ h gira as faces vizinhas em ~10⁻³ rad (já registrado no P06 §7). **[R]** Manter o colapso só no nível da tolerância de ponto e relatar as faces curtas no relatório de qualidade.
6. **Fragmentos:** uma célula recortada em vários pedaços fica com todos eles (conta em `BuildStats2D::fragmented_cells`, aviso no log). Todos os invariantes continuam válidos, inclusive a ortogonalidade. Nas 1000 configurações e nos âncoras, o contador ficou em 0 nos casos inspecionados; não houve verificação sistemática.
