# P17 — Relatório: domínio STL (entrega c)

- **Data:** 2026-09-30
- **Sequência:** `planning/sequencia_prompts.md`, Fase 3, P17 ("leitura, reparo e validação da superfície,
  rótulos de patch, clipping contra superfície triangulada"); arquitetura em `planning/P15_arquitetura_3d.md` §6
- **Ambiente:** WSL `Ubuntu-26.04-Test`; g++ 15.2.0; CMake 4.2.3; CGAL 6.2.1; Boost 1.92; Release com LTO,
  `-march=native` e `-ffp-contract=off` (DEC-034, DEC-037); 1 thread.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. O que foi entregue

| Módulo | Arquivos | Conteúdo |
|---|---|---|
| geometry | `repair.hpp/.cpp` | `TriangleSoup`, `repair_surface`: solda de pontos (10⁻⁹ da diagonal), triângulos colapsados e repetidos (um par de cópias opostas sai inteiro), orientação consistente por componente com o menor número de inversões; buracos, arestas não manifold e superfícies não orientáveis são erros `InvalidSurface`, nunca preenchidos |
| geometry | `surface.hpp/.cpp` | hierarquia de caixas na `TriangleSurface`: `distance` e `contains` em O(log n); `contains` pelo sinal do vetor até o ponto mais próximo contra a pseudonormal ponderada por ângulo (Bærentzen e Aanæs) |
| io | `stl.hpp/.cpp` | `read_stl` (ASCII e binário, detectado pelo conteúdo), `read_stl_surface`, `write_stl`; no ASCII, cada `solid nome` vira um patch |
| backend | `backend3d.cpp` | recorte local: corefinement da célula com os triângulos do domínio que tocam a sua caixa e classificação exata por pedaço; recorte contra o domínio inteiro como reserva |
| voronoi | `builder3d` | `BuildStats3D::local_clips` |
| tools | `anchors.hpp`, `benchmark.cpp` | `anchors::terrain_block(n)` (4n² + 8n triângulos, patches `terrain` e `rock`), caso B6 |

Também entraram o exemplo `ex_stl_domain` na galeria, que grava, lê, repara e malha um terreno em STL, e a seção
de STL em `guide/mesh3d.rst`, em português e inglês.

**Fora do escopo [F]:** CAD (STEP, IGES) exigiria uma biblioteca nova (R21). Recomenda-se exportar do CAD para STL.

## 2. O recorte local

**[F] Antes do P17**, o custo por célula recortada crescia com o tamanho do domínio (B6, 5000 sítios):

| Triângulos | Recorte por célula |
|---|---|
| 480 | 1,5 ms |
| 5 180 | 2,5 ms |
| 20 160 | 5,2 ms |

Extrapolando, um STL de 10⁶ triângulos custaria cerca de 190 ms por célula.

**[F] Medição por etapa (cronômetros temporários, já removidos).** A cópia do domínio não era o gargalo. O custo
estava na classificação dos triângulos, feita com um teste exato de pertinência por triângulo contra o domínio
inteiro. A corefinement marca as arestas de interseção, e cada pedaço delimitado por elas fica inteiro dentro ou
inteiro fora; basta uma consulta por pedaço. Só isso reduziu a classificação de 4,7 s para 0,29 s no B6 de
20 mil triângulos.

**Método:**
1. Os triângulos do domínio que tocam a caixa da célula formam uma superfície local aberta.
2. A célula e essa superfície são corefinadas, com os rótulos levados pelo *visitor*.
3. Ficam os pedaços da célula dentro do domínio (teste exato contra o domínio inteiro, com a árvore montada uma
   vez) e os pedaços do domínio dentro da célula.
4. Se algum pedaço cair exatamente sobre a outra superfície, ou se o resultado não fechar, a célula é recortada
   contra o domínio inteiro, pelo método do P16. O teste `CoplanarFacesFallBackToTheWholeRegion` exercita esse
   caminho.

**[F] Mesma malha:** o recorte local deu os mesmos checksums do método anterior em B4, B5 e B6.

## 3. Testes e cobertura

- **[F] Testes:** 237/237 em Debug, no build de cobertura e no portão da documentação em Release. São 13 testes
  novos:
  - `RepairSurface` (5);
  - `Stl` (4);
  - um de `TriangleSurface` (pertinência junto a vértices e arestas, cavidade);
  - `CoplanarFacesFallBackToTheWholeRegion`;
  - integração `StlDomain` (terreno em STL binário e ASCII até a malha, 2).
- **[F] Cobertura:** 97,6% das linhas e 92,0% dos ramos, nenhum arquivo abaixo do mínimo (R25).
- **[F] Verificação estática:** nenhum problema. **[F] Documentação:** gerada, com a figura do exemplo STL.

## 4. Desempenho (Release, 1 thread)

| Caso | Triângulos do domínio | Células | Construção | Recorte por célula recortada | Invariantes |
|---|---|---|---|---|---|
| B6 | 20 160 | 5 000 | 4,9 s | 2,5 ms | PASS |
| B6 | 79 520 | 20 000 | 15,2 s | 2,8 ms | PASS |
| B6 | 315 840 | 80 000 | 51,5 s | 3,5 ms | PASS |
| B4 (cubo) | 12 | 10⁶ | 192 s (P16: 212 s) | 1,0 ms | PASS |
| B5 (esfera) | 1 280 | 10⁵ | 38 s (P16: 44 s) | 2,0 ms | PASS |

- **Escala:** o custo por célula recortada fica praticamente constante (2,5 a 3,5 ms) enquanto o domínio cresce
  16 vezes. A geração de 80 mil sítios no terreno de 316 mil triângulos levou 2,1 s; antes da hierarquia de
  caixas, só a de 5000 sítios no terreno de 20 mil triângulos levava 3,7 s.
- **2D sem regressão:** B1 5,3 s, B2 0,34 s, B3 8,2 s, com os mesmos checksums.
- **[F] Memória do domínio:** no B6 o pico por célula é alto (5,9 a 7,9 KB), porque as estruturas exatas do
  domínio (malha `Epeck`, duas árvores de caixas) dominam quando há poucas células por triângulo. Com 316 mil
  triângulos e 80 mil células o pico foi 451 MB. **[I]** Para malhas grandes a memória volta a ser dominada pelas
  células (cerca de 1,4 KB cada, B4).

## 5. Achados

1. **[F] O gargalo do recorte era a classificação, não a cópia.** Medir por etapa evitou otimizar a coisa errada
   (P15 §6 supunha a cópia).
2. **[F] O `callgrind` não serve para o CGAL exato:** o programa morre antes do recorte, provavelmente porque o
   valgrind não suporta a troca de modo de arredondamento da aritmética de intervalos. O `perf` não está instalado
   no ambiente.
3. **[F] STL binário arredonda para float.** O teste aceita diferença de volume de até 10⁻⁶ contra o original; não
   medi o valor exato.
   Os invariantes usam a superfície lida como referência, então passam; a malha representa o arquivo, não a
   geometria que o gerou.
4. **[R] Paleta da galeria:** o meio `soil` não estava na paleta e a figura saiu na cor da água; corrigido. A página
   precisa ser regerada para a figura nova.

## 6. Pendências para o João

1. **Próxima etapa:** P18 (várias regiões em 3D, entrega d) ou antes a prova curta P18a da interface 3D conforme,
   recomendada no P15 §12.
2. **Numeração e facilidade de uso:** continuam abertas (P16 §5).
