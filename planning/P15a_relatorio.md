# P15a — Relatório: prova de conceito do recorte 3D

- **Data:** 2026-09-30
- **Sequência:** `planning/sequencia_prompts.md`, Fase 3, P15a (DEC-003, DEC-010)
- **Ambiente:** WSL `Ubuntu-26.04-Test`; g++ 15.2.0; CMake 4.2.3; CGAL 6.2.1; Boost 1.92; Release com LTO e
  `-march=native` (DEC-034); 1 thread.
- **Prazo:** o plano pede um prazo fixado pelo João; a prova foi pedida em 30/09 ("proceda para concluir essa
  etapa") e executada no mesmo dia.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Hipótese e método

**Hipótese (plano):** construir as células de Voronoi convexas por interseção de semiespaços e recortar apenas as
células necessárias contra o domínio 3D.

**Protótipo** (`prototypes/P15a`, descartável, fora do build principal):

1. **Domínio:** superfície triangulada fechada e orientada, com um patch por triângulo, possivelmente com vários
   componentes. Formas de teste: caixa, icosfera, prismas de polígonos (L, cunhas, estrela) e uniões.
2. **Sítios:** em ordem canônica (lexicográfica), o que torna o resultado independente da ordem de entrada (R18).
3. **Vizinhos:** Delaunay 3D do CGAL.
4. **Célula convexa:** a caixa envolvente recortada pelos bissetores, com um rótulo de plano em cada vértice (o
   recorte do P05a, em double).
5. **Vértices canônicos:** um vértice sobre três bissetores da célula i é o circuncentro dos 4 sítios, calculado
   uma única vez em aritmética exata (`Epeck`) e arredondado. Depois, os vértices a menos de 10⁻¹² L são fundidos
   (DEC-020, como no 2D), as faces com largura abaixo desse limite são descartadas e os vértices em "T" são
   inseridos na aresta vizinha.
6. **Classificação:** a célula só é recortada se a sua caixa toca algum triângulo do domínio (árvore AABB). Se não
   toca, a célula inteira está no interior, porque o sítio está dentro.
7. **Recorte exato:** `corefine_and_compute_intersection` do CGAL (`Epeck`) entre a célula (triangulada em leque a
   partir do centroide de cada face) e o domínio. Cada triângulo resultante recebe o rótulo do triângulo de origem
   (bissetor j ou triângulo t do domínio); os triângulos de mesmo rótulo e conexos formam uma face poligonal.
8. **Montagem:** no `vmm::Mesh<3>` genérico da biblioteca, verificado pelo `vmm::check_invariants<3>`. Nenhuma
   mudança na `vmm/` foi necessária **[F]**.

**Critérios de sucesso (plano):**
- todos os casos com os invariantes da DEC-011 dentro da tolerância (10⁻¹²);
- mesma topologia para ordens de inserção diferentes;
- tempo por célula compatível com as metas do P04 (3D: 10⁶ células em até 10 min, 1 thread; memória até 4 KB por
  célula).

Somam-se os critérios de construção: todo recorte feito, toda face interna vista pelas duas células, nenhuma face
com buraco e nenhum triângulo sem rótulo.

## 2. Resultados

**[F]** `p15a all`: 37 verificações, todas aprovadas (log em `~/.cache/vmm-agent-build/logs/p15a_final.log`).

| Caso | Células | Recortadas | Erro de volume | Fechamento | \|S_ij+S_ji\|/\|S_ij\| | µs/célula |
|---|---|---|---|---|---|---|
| C1 cubo, 2000 aleatórios | 2000 | 723 | 8,9·10⁻¹⁶ | 2,1·10⁻¹⁷ | 7,7·10⁻¹⁴ | 506 |
| C1 icosfera (320 triângulos) | 2000 | 679 | 6,6·10⁻¹⁶ | 3,2·10⁻¹⁷ | 7,2·10⁻¹⁴ | 925 |
| C2 prisma em L, 3000 | 3000 | 1057 | 5,9·10⁻¹⁶ | 5,9·10⁻¹⁷ | 4,6·10⁻¹⁴ | 518 |
| C3 cunha de 15° | 1500 | 807 | 2,6·10⁻¹⁵ | 2,7·10⁻¹⁷ | 2,8·10⁻¹⁴ | 788 |
| C3 cunha de 3° | 800 | 782 | 5,3·10⁻¹⁶ | 2,8·10⁻¹⁷ | 1,2·10⁻¹³ | 1376 |
| C4 estrela de 12 pontas, 150 | 150 | 142 | 1,7·10⁻¹⁶ | 2,7·10⁻¹⁷ | 7,7·10⁻¹⁵ | 2333 |
| C4 estrela, 3000 | 3000 | 1915 | 2,2·10⁻¹⁵ | 1,2·10⁻¹⁷ | 1,4·10⁻¹³ | 1296 |
| C5 dois cubos e uma esfera | 1500 | 756 | 8,9·10⁻¹⁶ | 1,2·10⁻¹⁶ | 4,1·10⁻¹⁴ | 888 |
| C6 cubo, grade 10×10×10 (cosférica) | 1000 | 488 | 8,9·10⁻¹⁶ | 5,6·10⁻¹⁸ | 4,6·10⁻¹⁶ | 410 |
| C6 cubo, 80 sítios a 10⁻⁹ L do contorno | 1080 | 485 | 6,7·10⁻¹⁶ | 3,0·10⁻¹⁷ | 1,9·10⁻¹⁴ | 581 |
| C6 prisma em L, grade | 3000 | 1208 | 4,8·10⁻¹⁴ | 2,0·10⁻¹⁷ | 1,9·10⁻¹⁵ | 331 |
| C7 prisma em L, 2000 (3 permutações, escalas 10⁻³ e 10⁶) | 2000 | 782 | ≤ 4,1·10⁻¹⁶ | relativo ≤ 10⁻¹⁷ | ≤ 3,2·10⁻¹⁴ | 550 |
| C8 prisma em L, 10⁵ | 100 000 | 12 028 | 3,3·10⁻¹⁵ | 1,7·10⁻¹⁷ | 3,2·10⁻¹³ | 350 |
| C8 estrela, 3·10⁴ | 30 000 | 10 815 | 3,4·10⁻¹⁶ | 5,5·10⁻¹⁸ | 3,6·10⁻¹³ | 800 |

Em todos os casos:
- **Invariantes:** erro de volume de cada célula contra o volume exato ≤ 1,9·10⁻¹⁷ (relativo ao total); área do
  contorno ≤ 1,1·10⁻¹⁴; nenhuma face mal orientada; adjacência simétrica.
- **Construção:** nenhum recorte falhou, nenhuma face ficou sem par, nenhuma face com buraco, nenhum triângulo sem
  rótulo e nenhuma face da caixa envolvente sobrou.
- **Ordem e escala (C7):** três permutações da entrada deram malhas idênticas bit a bit; as escalas 10⁻³ e 10⁶
  deram a mesma topologia.
- **Não ortogonalidade:** até 2,5·10⁻⁹ rad (C8), só relatada (DEC-032).

### Tempo e memória (C8, prisma em L com 10⁵ células)

| Fase | Tempo | Parcela |
|---|---|---|
| Delaunay 3D | 0,45 s | 1 % |
| Células convexas e vértices canônicos | 21,1 s | 60 % |
| Classificação | 0,09 s | 0 % |
| Recorte exato (12 028 células, 1,05 ms cada) | 12,7 s | 36 % |
| Montagem | 0,75 s | 2 % |
| **Total** | **35,0 s (350 µs/célula)** | |

- **Tempo [I]:** para 10⁶ células no mesmo domínio, a parcela recortada cai (as células de contorno crescem como
  N^(2/3): 12 % em 10⁵, ~6 % em 10⁶). A extrapolação dá 4–6 min, dentro da meta de 10 min. Não foi medido com 10⁶.
- **Memória [F]:** pico de 647 MB para 10⁵ células (≈ 6,6 KB/célula), acima da meta de 4 KB. O protótipo guarda
  todos os polígonos de todas as células antes da montagem.

## 3. Achados

1. **[F] A hipótese se confirma.** Células convexas por semiespaços, com recorte exato só das células que tocam o
   contorno, funcionam nos oito casos pedidos: domínio convexo, não convexo, arestas vivas (até 3°), células de
   contorno complexas, vários componentes, casos quase degenerados, ordem de inserção e estresse.
2. **[F] A degenerescência exige a mesma receita do 2D, e mais um passo.** Sem vértices canônicos, fusão a
   10⁻¹² L e descarte de lascas, a grade cartesiana (8 sítios cosféricos por vértice) gerou 241 malhas de célula
   inválidas e 1138 faces sem par. Com a receita, restaram faces em "T" (um vértice no meio da aresta de uma face e
   não da vizinha), resolvidas inserindo o vértice na aresta vizinha. Resultado: zero falhas.
3. **[F] A corefinement do CGAL, em Release, não verifica pré-condições.** Uma célula com triângulo degenerado
   derrubou o processo (falha de segmentação). É preciso validar a entrada antes (malha fechada e sem
   auto-interseção) e triangular as faces em leque a partir do centroide, nunca a partir de um vértice.
4. **[F] A saída da corefinement não tem ordem determinística.** O mesmo recorte devolveu os laços começando em
   vértices diferentes. Canonizar a saída (cada laço começa no menor ponto; faces ordenadas por rótulo) restaurou
   a malha idêntica bit a bit.
5. **[F] Células desconexas em domínios não convexos.** Na estrela, 9 de 150, 20 de 3000 e 46 de 3·10⁴ células
   ficaram em mais de um pedaço (a célula convexa atravessa duas pontas). Os invariantes continuam valendo; o
   protótipo mantém a célula inteira, como o 2D faz hoje (com aviso). **[R]** O P15 precisa decidir a política
   (manter com aviso, ou atribuir o pedaço sem o sítio à célula vizinha) para 2D e 3D juntos.
6. **[F] Rótulos por tolerância geométrica.** O protótipo descobre a origem de cada triângulo recortado por
   distância ao plano (10⁻¹⁰ L), com prioridade para o domínio. Nas grades houve 154 e 523 triângulos ambíguos,
   todos resolvidos corretamente por essa prioridade. **[R]** Na biblioteca, usar o *visitor* da corefinement para
   levar o rótulo exato, como o 2D faz com as arestas; isso exige implementar o conceito do CGAL por composição
   (AGENTS.md: sem herança).
7. **[F] Custo do recorte exato: 1 a 2,3 ms por célula recortada**, dominado pela cópia do domínio em cada chamada
   (a icosfera, com 320 triângulos, é a mais cara). Compartilhar o domínio com `do_not_modify(true)` foi medido no
   P15 e ficou de 22 a 142 vezes mais lento. **[I]** Com superfícies STL de 10⁵ triângulos, copiar o domínio
   por célula fica inviável. **[R]** Para o P16/P17: recortar contra um pedaço local fechado do domínio, ou usar o
   *clip* por planos da célula sobre os triângulos próximos.
8. **[F] O núcleo genérico serve ao 3D sem mudança.** `Mesh<3>::from_data` e `check_invariants<3>` foram usados
   como estão, com faces internas em ordem triangular superior e patches contíguos.
9. **[F] Memória acima da meta** (item 2 da seção 2). **[R]** Montar a malha por células à medida que ficam prontas,
   sem guardar todas as células; em 2D a mesma otimização levou a 673 B/célula.

## 4. Veredito

| Critério | Resultado |
|---|---|
| Invariantes da DEC-011 em todos os casos | **Atingido** (37/37 verificações) |
| Mesma topologia para ordens diferentes | **Atingido** (malha idêntica bit a bit) |
| Tempo por célula | **Atingido por extrapolação** (350 µs/célula em 10⁵; 10⁶ não medido) |
| Memória (meta do P04) | **Não atingido no protótipo** (6,6 KB/célula; causa conhecida, item 9) |

A hipótese central se confirma, e a contingência do plano não é necessária. **[R]** O P15 (arquitetura 3D) pode
partir deste desenho:
- um backend 3D no mesmo modelo de struct de funções do 2D (Delaunay 3D, vértice canônico exato, recorte rotulado
  de uma célula pelo domínio);
- a receita de degenerescência do item 2 no núcleo;
- montagem em fluxo.

Decisões que o P15 precisa tomar com o João: a política de células desconexas (item 5), a representação do domínio
3D (superfície triangulada com patches) e o rótulo exato pelo *visitor* (item 6).

## 5. Como reproduzir

```bash
cmake -S prototypes/P15a -B ~/build-p15a -G Ninja
cmake --build ~/build-p15a
~/build-p15a/p15a all
```

`p15a quick` roda tudo menos o estresse (cerca de 20 s); `p15a <caso>` roda um caso (`convex`, `nonconvex`,
`sharp`, `complex`, `components`, `degenerate`, `order`, `stress`).
