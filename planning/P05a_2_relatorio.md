# P05a.2 — Relatório: prova 2D multirregião

- **Data:** 2026-09-29
- **Sequência:** v5.2, prompt P05a, iteração P05a.2
- **Ambiente:** o mesmo do `planning/P05a_1_relatorio.md` §1 (WSL `Ubuntu-26.04-Test`, g++ 15.2.0, CMake 4.2.3, CGAL 6.2.1, Boost 1.92).
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Caso

- Quadrado 1 × 1 com furo quadrado central `[0,4; 0,6]²`.
- Região A = `[0; 0,7]²` menos o furo (tem buraco). Região B = o restante, em forma de "L" (não convexa).
- Interface em "L": `(0,7; 0) → (0,7; 0,7) → (0; 0,7)`.
- 145 sítios gerados por *dart throwing* com semente fixa: 70 em A e 75 em B, com espaçamento mínimo 0,042 e margem de 0,0175 até qualquer segmento.
- O gerador usa `std::mt19937_64` com conversão manual para `[0, 1)`, porque `uniform_real_distribution` não é portável.

## 2. Pipeline (sem CGAL no núcleo)

1. **Vizinhos:** Delaunay de cada região, só com os sítios dela (backend).
2. **Célula convexa:** caixa de trabalho recortada pelos semiplanos dos vizinhos (Sutherland–Hodgman próprio). Cada aresta leva um **rótulo**: o sítio vizinho, o segmento do layout ou o lado da caixa.
3. **Recorte pela região:** `CGAL::intersection` (Boolean_set_operations_2, Epeck) do polígono convexo com o polígono da região, que pode ter buracos (backend). Cada aresta de saída recebe o rótulo da aresta de entrada que a contém, por teste **exato** de colinearidade. A região vence em caso de empate.
4. **Vértices canônicos:** cada vértice é recalculado a partir dos dois rótulos que o cercam:
   - circuncentro dos três sítios, em ordem crescente de id;
   - interseção do bissetor canônico com o segmento;
   - ou o canto comum de dois segmentos.

   As duas células que compartilham um vértice obtêm **os mesmos doubles, bit a bit**.
5. **Faces:** as faces internas saem dos rótulos de sítio. As peças vistas pelas duas células precisam coincidir exatamente. As faces de fronteira saem dos rótulos de segmento, com o patch do segmento.
6. **Interface (refinamento comum):** para cada segmento de interface:
   - os pontos de quebra dos dois lados são ordenados pelo parâmetro ao longo do segmento;
   - pontos a menos de 10⁻¹² L são fundidos (DEC-020);
   - cada subintervalo precisa ser coberto por exatamente uma peça de cada região.

A topologia vem **só de rótulos**; a proximidade geométrica não decide nada. A única tolerância é a fusão de pontos de quebra na interface.

**Estratégias**

- **E1:** sítios independentes em cada região.
- **E2:** mesmos sítios de A que em E1. Os sítios de A a menos de `h` = 0,07 de um segmento de interface (com projeção dentro dele) são refletidos para B, e os demais sítios de B ficam fora dessa camada. O pipeline é o mesmo; E2 muda só os sítios. **[I]** Por isso, na forma implementada, E2 é "E1 + política de sítios espelhados", e o refinamento comum continua sendo a garantia de conformidade.

## 3. Resultados

**[F]** Todas as 14 execuções passaram (2 estratégias × {escala 1, 10⁻³, 10⁶ na ordem 0; ordens 1 a 4 na escala 1}). O executável sai com 0.

Em cada execução, o verificador (`invariants.hpp`, genérico em D) conferiu:
- soma das medidas (total, por região e por célula contra o *shoelace* do polígono recortado) com erro relativo ≤ 10⁻¹²;
- fechamento `|Σ ±S_f| ≤ 10⁻¹² L` por célula;
- owner < neighbour, patch só na fronteira, toda célula com medida positiva e adjacência CSR simétrica;
- comprimento da fronteira e da interface iguais aos do layout;
- não ortogonalidade das faces internas de uma mesma região ≤ 10⁻⁸ rad;
- construção: todos os rótulos resolvidos, toda peça de bissetor vista pelos dois lados, cobertura da interface correta, células conexas;
- topologia canônica igual à da execução de referência.

Execução de referência (escala 1, ordem 0):

| | E1 | E2 |
|---|---|---|
| células / faces internas / interface / fronteira | 145 / 359 / 28 / 57 | 145 / 361 / 19 / 57 |
| erro relativo da área total | 2,3·10⁻¹⁶ | 4,6·10⁻¹⁶ |
| pior célula (face × polígono), relativo à área total | 7,2·10⁻¹⁸ | 9,0·10⁻¹⁸ |
| fechamento máximo | 6,9·10⁻¹⁸ (tol 1,4·10⁻¹²) | 6,9·10⁻¹⁸ |
| não ortogonalidade interna máx. | 1,3·10⁻¹³ rad | 7,8·10⁻¹⁴ rad |
| não ortogonalidade na interface: máx. / média | 67,1° / 31,4° | 90,0° / 15,5° |
| faces espelhadas (par no seu segmento de reflexão) | 0 | 10 de 19, **81,1 % do comprimento**, máx. 2,7·10⁻¹⁵ rad |
| pontos de quebra fundidos (escala 1 / 10⁻³ / 10⁶) | 0 / 0 / 0 | 0 / 5 / 5 (distância máx. 1,6·10⁻¹⁹ e 8,7·10⁻¹¹, ou seja, ≈ 6·10⁻¹⁷ L) |
| topologia nas 3 escalas e 5 ordens | estável | estável |

## 4. Achados

1. **[F] Canto da interface (E2):** a célula do espelho de um sítio perto da quina `(0,7; 0,7)`, refletido através de I1, contorna a quina e também toca I0. Isso gera uma face com o **mesmo par** no segmento errado, a 90°: é o máximo de E2. A estatística de "face espelhada" passou a exigir o segmento de reflexão. **[I]** Espelhar quinas convexas exige tratamento próprio (reflexão pontual pela quina ou uma camada de sítios de canto).
2. **[F] A fusão por tolerância é necessária em E2:** nas escalas 10⁻³ e 10⁶, os pontos de quebra dos dois lados de um par espelhado diferem por arredondamento (até 6·10⁻¹⁷ L). Sem a fusão, surgiriam faces-lasca. Na escala 1, a coincidência foi exata, porque os segmentos são paralelos aos eixos. **[I]** Uma interface inclinada deve exercitar a fusão em qualquer escala; este caso não a cobre.
3. **[F] Dois bugs encontrados e corrigidos durante a iteração (dentro do limite de 3 tentativas):**
   - o semiplano usava a normal do par ordenado, e não de i para j;
   - `collinear_are_ordered_along_line` do CGAL **supõe** colinearidade, que precisa ser testada antes com `CGAL::collinear`.

   A precondição está no código-fonte do CGAL 6.2.1: `CGAL/Cartesian/function_objects.h:293`, `CGAL_kernel_exactness_precondition( collinear(p, q, r) )`. Documentação: <https://doc.cgal.org/latest/Kernel_23/group__kernel__global__function.html>.
4. **[I] Fragmentação:** o Voronoi euclidiano por região, recortado por uma região com buraco, pode partir uma célula em duas componentes. Aqui ocorreram 0 casos em 14 execuções, mas o pipeline só detecta o problema, não o resolve. O P06 precisa de uma política para isso.
5. **[F] Sanitizers:** com ASan e UBSan completos, o UBSan (checagem `vptr`) acusa um *downcast* em `/usr/local/include/CGAL/Arrangement_2/Arrangement_2_iterators.h:458`, disparado por `CGAL::intersection`. Todo o resto passa: com `-fno-sanitize=vptr`, os 7 testes passam. O objeto é um `Arr_face` da DCEL, convertido para o tipo `Face` do arranjo. Não verifiquei se é um problema conhecido do CGAL.

## 5. Iteração sobre volumes e faces (pedido do João durante a execução)

Acrescentei em `mesh.hpp`:
- `internal_faces(mesh)` e `boundary_faces(mesh)`: views de `FaceId`;
- `CellFaceIndex` (`index_cell_faces(mesh)`), com `internal_cells()` (volumes sem face de fronteira), `boundary_cells()` (volumes com pelo menos uma) e `faces_of(cell)`.

Ficam fora de `mesh.hpp`: os volumes que só tocam a interface contam como internos, e as faces de interface estão em `internal_faces`.

**[F]** Em E1: 387 faces internas + 57 de fronteira; 96 volumes internos + 49 de fronteira. As contagens batem com o verificador, as duas views de volumes particionam as células, e as faces alcançadas por volume somam `2·internas + fronteira`.
