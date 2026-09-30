# P18a — Relatório: prova de conceito da interface 3D conforme

- **Data:** 2026-09-30
- **Origem:** `planning/P15_arquitetura_3d.md` §12 (maior risco da Fase 3); pedido do João em 30/09
- **Ambiente:** WSL `Ubuntu-26.04-Test`; g++ 15.2.0; CGAL 6.2.1; Boost 1.92; Release (LTO, `-march=native`,
  `-ffp-contract=off`); 1 thread.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Hipótese e método

**Hipótese:** a estratégia do 2D (DEC-028, E1) funciona em 3D:
- cada região recebe as suas células de Voronoi, recortadas pela própria superfície fechada;
- em cada triângulo de interface, os pedaços das células de cada lado são convexos (célula convexa ∩ triângulo) e
  cobrem o triângulo;
- a face de interface entre duas células é a interseção dos seus pedaços (refinamento comum).

**Protótipo** (`prototypes/P18a/p18a.cpp`, descartável, sobre a biblioteca):
1. Caixa unitária em camadas, separadas por superfícies z = h(x, y) (plana, ondulada), todas na mesma grade n × n.
   As regiões compartilham os triângulos de interface (mesmos pontos).
2. Cada região é malhada com `vmm::build_mesh_3d` (P16/P17). Cada triângulo de interface é um patch próprio
   (`if:<t>`), de modo que as faces de contorno de cada célula dizem de qual triângulo vieram.
3. Em cada triângulo t, cada pedaço de um lado é recortado por cada pedaço do outro (recorte de polígonos convexos
   no plano de t, em double). Cada interseção vira uma face entre as duas células, orientada a partir da de menor
   índice.
4. As regiões são fundidas num único `Mesh<3>` e verificadas com `vmm::check_invariants`, incluindo a área de
   cada interface por par de regiões.

**Critérios:**
- invariantes da DEC-011 em 10⁻¹² (volumes total e por região, volume de cada célula, contorno, área de cada
  interface, fechamento, adjacência simétrica, nenhuma face de interface entre células da mesma região);
- cobertura de cada triângulo de interface pelo refinamento em 10⁻¹² L², nenhum pedaço órfão;
- mesma malha para outra ordem dos sítios e mesma topologia em outras escalas.

## 2. Resultados

**[F] 22 verificações, todas aprovadas** (log em `~/.cache/vmm-agent-build/logs/p18a_all.log`).

| Caso | Células | Faces de interface | Erro de área das interfaces | Fechamento | Cobertura (L²) | Não ortogonalidade na interface |
|---|---|---|---|---|---|---|
| C1 interface plana | 5 059 | 2 126 | 5,5·10⁻¹⁴ | 1,9·10⁻¹⁴ | 6,9·10⁻¹⁵ | 1,16 rad |
| C2 interface ondulada, espaçamentos diferentes | 12 096 | 4 548 | 8,8·10⁻¹⁵ | 6,0·10⁻¹⁶ | 1,7·10⁻¹⁶ | 1,27 rad |
| C3 três regiões (interfaces até o contorno) | 6 658 | 7 248 | 5,5·10⁻¹⁴ | 1,7·10⁻¹⁴ | 1,1·10⁻¹⁴ | 1,31 rad |
| C4 grades cartesianas espelhadas | 216 | 72 | 2,2·10⁻¹⁶ | 5,8·10⁻¹⁷ | 1,6·10⁻¹⁷ | 0,00 rad |
| C4 sítios a 10⁻⁷ da interface | 576 | 2 356 | 3,7·10⁻¹³ | 1,5·10⁻¹³ | 4,9·10⁻¹⁴ | 1,88 rad |
| C5 ordem invertida, escalas 10⁻³ e 10⁵ | 4 933 | 2 525 | ≤ 4,0·10⁻¹³ | relativo ≤ 10⁻¹³ | ≤ 1,3·10⁻¹³ | 1,26 rad |
| C6 estresse, três camadas | 118 423 | 58 635 | 1,6·10⁻¹³ | 5,9·10⁻¹⁴ | 2,5·10⁻¹⁴ | 1,23 rad |

- **Ordem e escala (C5):** a ordem invertida dos sítios deu a malha fundida idêntica bit a bit; as escalas 10⁻³ e
  10⁵ deram a mesma topologia.
- **Robustez:** em todos os casos, nenhum pedaço ficou sem par e nenhuma lasca foi descartada.
- **Custo (C6, 118 mil células):** 57 s para malhar as três regiões e 0,6 s para o refinamento comum e a fusão.

## 3. Achados

1. **[F] A conformidade da interface 3D funciona pelo mesmo caminho do 2D.** Os pedaços de cada lado cobrem cada
   triângulo com erro de arredondamento; o refinamento em double, com área mínima de (10⁻¹² L)², foi suficiente
   em todos os casos, inclusive nos degenerados. O custo é desprezível perto da construção das regiões.
2. **[F] A não ortogonalidade nas faces de interface é alta: 1,2 a 1,9 rad.** Em 1,88 rad (108°) o vetor entre os
   sítios forma mais de 90° com a normal da face, e um fluxo de dois pontos teria o sinal errado. É o efeito
   conhecido do E1 sem pares espelhados (no 2D chegou a 75°, P12 §2), mais forte em 3D. Com sítios espelhados
   (grades espelhadas, C4) a não ortogonalidade cai a zero. **[R]** No P18, oferecer em 3D os pares espelhados
   através da interface (DEC-028, E2; `InterfacePairs` já existe no 2D) e medir a não ortogonalidade da interface
   com eles. Os invariantes não falham com a não ortogonalidade (DEC-032), então isto é uma questão de qualidade
   para o solver, não de consistência da malha.
3. **[F] O que o P18a não testou:** a partição por precedência (DEC-018) a partir de formas que se sobrepõem. Aqui
   as superfícies das regiões foram construídas já compatíveis (mesmos triângulos na interface). **[R]** No P18,
   construir a partição 3D com o autorrefinamento exato do CGAL (`autorefine_triangle_soup`): as superfícies de
   todas as camadas viram uma sopa sem auto-interseções. Depois, cada triângulo é classificado pelos dois lados
   segundo a ordem de precedência, o análogo 3D do arranjo exato do 2D. Esse é o risco que sobra na Fase 3.
4. **[F] Área mínima de descarte.** Com o critério relativo à área de cada triângulo pequeno (10⁻¹²), a cobertura
   pareceu falhar (2,7·10⁻¹² no C1). Em termos da tolerância de áreas da DEC-020 (10⁻¹² L²), ficou em 10⁻¹⁴. O
   critério certo é o da DEC-020.

## 4. Veredito

| Critério | Resultado |
|---|---|
| Invariantes, inclusive a área de cada interface | **Atingido** em todos os casos |
| Cobertura de cada triângulo de interface | **Atingido** (≤ 1,3·10⁻¹³ L²) |
| Mesma malha para outra ordem, mesma topologia em outra escala | **Atingido** |
| Qualidade da face de interface para volumes finitos | **Não atingido sem pares espelhados** (achado 2) |

**[R] Desenho do P18:**
1. Partição por precedência com autorrefinamento exato e classificação dos triângulos (achado 3).
2. As regiões são construídas uma a uma com o `build_mesh_3d` atual, com um patch lógico por triângulo de interface.
3. O refinamento comum da interface entra na biblioteca, no mesmo formato deste protótipo.
4. Pares espelhados opcionais através das interfaces (achado 2).

## 5. Como reproduzir

```bash
cmake -S prototypes/P18a -B ~/build-p18a -G Ninja
cmake --build ~/build-p18a
~/build-p18a/p18a all
```

`p18a quick` roda tudo menos o estresse; `p18a <caso>` roda um caso (`flat`, `wavy`, `three`, `degenerate`,
`order`, `stress`).
