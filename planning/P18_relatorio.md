# P18 — Relatório: 3D com várias regiões (entrega d)

- **Data:** 2026-09-30
- **Sequência:** `planning/sequencia_prompts.md`, Fase 3, P18 ("3D multirregião conforme: espelhamento e corte,
  junções triplas, arestas vivas"); desenho do `planning/P18a_relatorio.md` §4
- **Ambiente:** WSL `Ubuntu-26.04-Test`; g++ 15.2.0; CMake 4.2.3; CGAL 6.2.1; Boost 1.92; Release com LTO,
  `-march=native` e `-ffp-contract=off`; 1 thread.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. O que foi entregue

| Peça | Onde | Conteúdo |
|---|---|---|
| Declaração | `domain/declaration3d` | regiões e buracos (`add_hole`) por precedência (DEC-018) |
| Partição | `src/backend/cgal/backend3d.cpp` | autorrefinamento exato de todas as camadas; classificação de cada triângulo pelos dois lados; arredondamento para double sem interseções |
| Construção | `voronoi/builder3d` | um diagrama de Voronoi por região (DEC-028, E1); refinamento comum dos pedaços sobre cada triângulo de interface; `BuildStats3D::interface_faces`, `interface_slivers` |
| Pares espelhados | `sites/sources3d` | `InterfacePairs3D` em `SiteGenerationOptions3D::interface_pairs` (DEC-028, E2) |
| Âncora A3 | `tools/anchors` | `a3()` e `heightfield_block()`; teste de integração, exemplo `ex_anchor_a3`, benchmark B7 |
| Documentação | `guide/mesh3d.rst` | seção "Várias regiões", em português e inglês; a galeria corta as malhas de várias regiões na vertical |

**Partição por precedência (detalhe):**
1. Cada camada é validada isoladamente (fechada, sem auto-interseção).
2. Todas as camadas vão para uma sopa de triângulos autorrefinada em aritmética exata (CGAL
   `autorefine_triangle_soup`). O *visitor*, implementado por composição, guarda a origem de cada triângulo.
3. Os triângulos são agrupados por identidade exata dos pontos; um triângulo compartilhado por duas camadas
   (faces coplanares) aparece no grupo com as duas origens.
4. Cada lado é classificado pela camada de maior precedência que o contém. Para as camadas do grupo, a
   orientação decide; para as outras, um teste exato de pertinência do centroide. Ficam os triângulos com regiões
   diferentes dos dois lados; o patch de um triângulo de contorno vem da camada de maior precedência.
5. O CGAL arredonda o resultado para double com `apply_iterative_snap_rounding`, garantindo que as superfícies
   arredondadas não se cortem.

## 2. Testes e cobertura

- **[F] Testes:** 248/248 em Debug, no build de cobertura e no portão da documentação em Release, antes da
  correção de memória da §4; a rodada depois da correção está registrada na §6. São 11 testes novos:
  - `Cgal3D.PartitionByPrecedence`: núcleo pintado por cima, metade coplanar com patches da camada de maior
    precedência, buraco e esfera no cubo;
  - `InterfacePairs3D` (3), `Declaration3D.Holes`;
  - integração `MultiRegion3D` (5): núcleo, esfera com buraco, três camadas coplanares, ordem invertida idêntica
    bit a bit, pares espelhados;
  - integração `AnchorA3`.

  O teste de erros da partição foi ajustado: duas regiões deixaram de ser erro e passaram a ser verificadas a
  região anulada e a declaração só com buraco.
- **[F] Cobertura:** 97,4% das linhas e 91,7% dos ramos, nenhum arquivo abaixo do mínimo.
- **[F] Verificação estática:** nenhum problema.

## 3. Âncora A3 (P04 §2.3)

Bloco de 500 × 200 × 30 m, com quatro regiões por precedência: `solo_inf`, `solo_sup` (contato inclinado
z = 15 + 0,01x), `atmosfera` (acima de z = 25) e `canal` (seção trapezoidal com 40 m no topo e taludes 1V:2H,
profundidade de 3 a 6 m, eixo curvo em planta).

**[F] Resultados:**
- volume do canal = 69 000 m³ (∫(40 − 2d)·d dx, exato);
- interface água/ar = 20 000 m² (40 m × 500 m);
- volume total = 3·10⁶ m³;
- há interfaces canal/solo de cima, canal/solo de baixo (o leito corta o contato a jusante), solo/ar e entre os
  dois solos;
- existem células do canal vizinhas do ar e do solo ao mesmo tempo (junções triplas);
- os patches são os seis da caixa, e as pontas do canal herdam `montante` e `jusante` (faces coplanares com a caixa).

| Caso | Células | Faces de interface | Construção | Memória | Invariantes |
|---|---|---|---|---|---|
| A3, refinamento 1 | 12 545 | 39 645 | 20 s | 5,0 KB/célula | PASS |
| A3, refinamento 2 (B7) | 103 645 | 137 180 | 107 s | 2,9 KB/célula | PASS |

## 4. Desempenho e uma regressão corrigida

- **[F] B4 (cubo, 10⁶ células):** 191 s e 1,37 KB/célula, a mesma malha (checksum) do P17.
- **[F] 2D sem regressão:** B1 5,5 s, B2 0,35 s, B3 8,2 s, mesmos checksums.
- **[F] Regressão encontrada e corrigida.** A primeira versão multirregião guardava cada face interna num vetor
  próprio antes de ordenar, e a memória do B4 subiu para 2,0 KB/célula. Passei a um armazenamento contíguo, em que
  as faces das regiões já saem em ordem e só as de interface são ordenadas e intercaladas. A primeira tentativa
  tinha um `reserve` exato por face, que tornou a montagem quadrática (B7 de 108 para 549 s). Removido, os números
  voltaram: B4 191 s e 1,37 KB/célula; B7 107 s. A malha é a mesma em todas as versões (mesmos checksums).
- **[I] Onde está o tempo no A3:** no recorte exato (91 de 108 s no B7), porque as camadas finas fazem quase todas
  as células tocarem alguma superfície (70% recortadas).

## 5. Achados

1. **[F] Qualidade das faces de interface.** Sem pares, a não ortogonalidade nas interfaces chega a 1,8 rad (A3)
   e a média ponderada pela área fica em cerca de 0,55 rad. Com `InterfacePairs3D` a média cai para 0,011 rad
   (interface plana) e 0,048 rad (esfera); o máximo continua perto de 0,9 a 1,3 rad em faces pequenas entre
   células que não formam par.
2. **[F] Pares espelhados em interfaces curvas criam uma degenerescência maciça.** Com o mesmo afastamento para
   todos, os sítios internos de uma esfera ficam à mesma distância do centro (cosféricos), e o recorte em double
   gerou células inválidas. O afastamento agora varia de forma determinística (±20%) de par para par; cada par
   continua espelhado, com a face ortogonal. Isso revelou dois reforços na limpeza de faces, que passam a valer
   para tudo:
   - laços que se tocam num vértice são divididos;
   - a limpeza roda de novo depois da inserção de vértices em "T", que podia deixar um "espinho".
3. **[F] Faces coplanares entre camadas funcionam** (núcleo a meio cubo, topo do canal e base da atmosfera, pontas
   do canal e faces da caixa): o autorrefinamento as iguala, e o agrupamento por identidade exata junta as origens.
4. **[I] Limite conhecido:** a construção em double continua sensível a degenerescências maciças (centenas de
   sítios cosféricos). Os casos testados passam, inclusive grades cartesianas; um recorte convexo exato por célula
   seria a proteção geral, com custo alto. **[R]** Só se aparecer um caso real.

## 6. Verificação final

**[F] Depois da correção de memória:**
- 248/248 testes em Debug, no build de cobertura e no portão da documentação;
- cobertura de 97,4% das linhas e 91,7% dos ramos, nenhum arquivo abaixo do mínimo;
- documentação gerada com a figura do A3.

A figura mostra o corte vertical sem a camada de ar, que antes cobria o canal; a galeria passou a omitir o ar nas
figuras 3D com vários meios.

## 7. Pendências para o João

1. **P19:** documentação e release 3D.
2. **Numeração das versões:** 0.3, 0.4 e 0.5 foram implementadas, e a 0.2 (escritores MODFLOW, PFLOTRAN e TOUGH)
   não.
3. **Facilidade de uso:** a opção para quem sabe pouco C++ continua aberta.
4. **Paralelização:** fica para a etapa posterior, como você decidiu; o recorte exato é o ganho natural.
