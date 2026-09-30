# P05a — Provas de conceito antes da arquitetura

- **Data:** 2026-09-29
- **Sequência:** v5.2, prompt P05a, iteração P05a.4 (relatório final)
- **Entregáveis:** este arquivo, `prototypes/P05a/` e os relatórios `planning/P05a_1_relatorio.md`, `P05a_2_relatorio.md` e `P05a_3_relatorio.md`.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Ambiente e versões (DEC-012)

**[F]** Rodou na distribuição oficial `Ubuntu-26.04-Test` (Ubuntu 26.04.1 LTS), e não na nuvem como previa o plano: o código foi escrito e verificado no próprio WSL.

| Item | Versão |
|---|---|
| Compilador | g++ 15.2.0, C++23 |
| Build | CMake 4.2.3, Ninja 1.13.2 |
| CGAL | 6.2.1 (`/usr/local`; versão dos headers checada por `static_assert`) |
| Boost | 1.92 |
| Aritmética exata do Epeck | GMP |
| Commit | `d3edfe7` + árvore de trabalho; `prototypes/P05a/` novo, não versionado |

**[F]** Também há um CGAL 6.1.1 e um Boost 1.90 em `/usr/include`. Não foram usados.

Para reproduzir:
```bash
prototypes/P05a/run_evidence.sh /tmp/p05a_build
```

**[F]** Resultado da última execução:

| Modo | Resultado |
|---|---|
| Release | 7/7 testes CTest; 173 checagens PASS e 0 FAIL nos executáveis |
| Debug + ASan/UBSan sem `vptr` (`P05A_SANITIZE=novptr`) | 7/7 |
| Debug + ASan/UBSan completo (`P05A_SANITIZE=1`) | 6/7: o UBSan para dentro do CGAL (ver §4, item 7) |

## 2. O que funcionou

| Tarefa | Resultado |
|---|---|
| 1. Prova 2D multirregião (buraco, região em "L", interface em "L") | **[F]** Invariantes da DEC-011 cumpridos por E1 e E2 em 3 escalas (1, 10⁻³, 10⁶) e 5 ordens de inserção: medidas ≤ 5·10⁻¹⁶ relativo; fechamento ≤ 10⁻¹⁷ L; owner/neighbour consistentes; interface conforme; topologia canônica estável. |
| 2. Prova 3D mínima (caixa, uma região) | **[F]** Volume = 1 (erro ≤ 1,2·10⁻¹⁵); fechamento; V − E + F = 2 em toda célula; mesma topologia em 3 escalas e 3 ordens; **mesmo `PolyMesh<D>` e mesmo verificador do 2D, sem nenhuma mudança**. |
| 3. Firewall (DEC-007) | **[F]** Os 7 headers públicos compilam sem ligar ao CGAL, e a lista de headers abertos (`-M`) não contém CGAL, Boost, GMP nem MPFR. Os dois controles negativos detectam o header contaminado. `p05a_core` liga sozinho. |
| 4. Adjacência compacta (DEC-015) | **[F]** CSR simétrica e padrão esparso idêntico ao calculado diretamente dos pares (2D: 2000 células; 3D: 1000), sem geometria. Também é derivada de owner/neighbour nas provas 2D e 3D. |
| Pedido extra: iteradores | **[F]** Views de faces internas/de fronteira e de volumes internos/de fronteira, com as faces de cada volume, verificadas em 2D e 3D (ver DEC-029). |

## 3. E1 × E2 (escala 1, ordem 0)

| | E1 — Voronoi por região + refinamento comum | E2 — sítios espelhados (mesmo pipeline) |
|---|---|---|
| Faces de interface | 28 | 19 |
| Não ortogonalidade na interface: máx. / média | 67,1° / 31,4° | 90,0° / 15,5° |
| Comprimento com faces espelhadas exatas | 0 % | **81,1 %** (máx. 2,7·10⁻¹⁵ rad) |
| Pontos de quebra fundidos por tolerância | 0 | 5 nas escalas 10⁻³ e 10⁶ (≤ 6·10⁻¹⁷ L) |
| Conformidade | por construção | por construção: o refinamento comum continua sendo a garantia |
| Onde falha | ortogonalidade em toda a interface | quina convexa: o espelho contorna a quina e gera uma face a 90° |

**[R]** Estratégia da 0.1 (DEC-028, proposta):
- **E1 como mecanismo** de conformidade;
- **E2 como política opcional** de sítios, para recuperar a ortogonalidade;
- tratamento específico de quinas convexas antes de tornar E2 o padrão.

## 4. O que falhou ou ficou em aberto

1. **[F] Voronoi numa região com buraco pode fragmentar células.** Não ocorreu no caso testado (0 em 14 execuções), mas o pipeline só detecta o problema. Falta uma política: reatribuir o fragmento, inserir um sítio ou usar distância geodésica.
2. **[F] A tolerância de fusão é indispensável em E2.** Ela só foi exercitada fora da escala 1, porque a interface do caso é paralela aos eixos. **[I]** Falta testar uma interface inclinada.
3. **[F] Faces 3D minúsculas:** 2,5·10⁻¹⁰ L², com diferença de 3,6·10⁻¹¹ entre as duas cópias da face. **[I]** O limite de 10⁻⁸ rad da DEC-020 fica mal posto para faces degeneradas.
4. **[F] A tolerância de fechamento tem dimensão L^(D−1),** e não L como diz o plano.
5. **[F] Vértices guardados por face,** sem tabela global: 1013 bytes/célula em 3D (a meta é ≤ 4 KB). Não é benchmark.
6. **[F] "Headers sem o CGAL no caminho de includes"** não é literalmente possível aqui: `/usr/include` e `/usr/local/include` contêm `CGAL/`. A prova válida é a lista de headers efetivamente abertos.
7. **[F] UBSan (`vptr`) acusa um *downcast* em `CGAL/Arrangement_2/Arrangement_2_iterators.h:458`**, disparado por `CGAL::intersection` (Boolean_set_operations_2). O resto passa limpo com `-fno-sanitize=vptr`. Não verifiquei se é um problema conhecido do CGAL.

## 5. O que muda no P06

1. **Topologia por rótulos:** a fronteira do backend devolve geometria **com rótulos** (de que entrada veio cada aresta ou face). O núcleo reconstrói os vértices de forma canônica a partir dos rótulos, e nenhuma decisão topológica usa distância. Essa forma substitui a "topologia por proximidade" citada na DEC-011.
2. **Backend como composição de callables** (`struct` de funções), montado fora do núcleo. O núcleo compila e liga sem o CGAL, sem `virtual` (R3, DEC-007).
3. **Modelo de dados:** malha baseada em faces (owner/neighbour/patch + pontos da face em CSR) e genérica em D. Só `face_geometry` depende da dimensão. O P06 decide sobre a tabela de vértices com deduplicação.
4. **Invariantes:** o verificador genérico (`check_invariants<D>`) serve de modelo para o P07, com tolerância de fechamento 10⁻¹² · L^(D−1) e uma medida por célula calculada de forma independente.
5. **Políticas que faltam definir:**
   - células fragmentadas;
   - faces degeneradas (limiar relativo ou colapso);
   - quinas convexas em E2;
   - fusão de pontos de quebra (já implementada a 10⁻¹² L).
6. **Iteração:** faces internas primeiro e as de fronteira agrupadas por patch, formando spans contíguos; volumes internos e de fronteira como listas precomputadas (DEC-029).
7. **CI (P07):**
   - firewall por lista de dependências (`-M`) e controle negativo;
   - decisão sobre a checagem `vptr` do UBSan no alvo do backend;
   - `prototypes/` fora da cobertura (DEC-027).

## 6. Prazo e tentativas

**[F]** Quatro iterações, todas concluídas. Houve duas correções na P05a.2 (orientação do semiplano e teste de colinearidade), dentro do limite de três tentativas. As iterações rodaram em sequência, a pedido do João, sem aprovação entre elas.
