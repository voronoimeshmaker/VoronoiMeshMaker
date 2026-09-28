# Revisão do P03 — Claude (AUTORREVISÃO, não independente)

> **Conflito de interesse:** eu sou o autor do P03. Esta revisão segue o prompt, mas **não substitui** a revisão de uma IA independente. Usei métodos diferentes dos do relatório sempre que possível, para não repetir os mesmos erros.

## 1. Ambiente e execução

- **Onde:** container na nuvem (Ubuntu 24.04, g++ 13.3, CGAL 5.6.1), cópia do commit `7402178`. **O WSL `Ubuntu-26.04-Test` continua inacessível**, então o roteiro da §12 **não** foi executado.
- **O que foi executado:**
  - contagem de tipos com uma expressão regular diferente da do relatório;
  - leitura direta dos trechos citados;
  - teste de inclusão, header a header (`g++ -fsyntax-only -H`), para ver quais headers puxam o CGAL;
  - reexecução do experimento de ordem de inserção;
  - checagem cruzada entre as colunas do CSV.

## 2. Verificação por amostragem

| Item | Resultado | Evidência |
|---|---|---|
| a) 115 tipos; 6 com `ut_` próprio; 44 sem citação | **CONFIRMADO** (115 e 6 por um método diferente; os 44 pelo mesmo método do P03) | regex alternativa sobre `VMMLib`: 115. Os 6 são Rectangle, Ring2D, Config (`ErrorConfig.h:55`), Status, Site2D e SiteSet |
| b) 32 encaminhamentos; 25 vazios; 51 não alcançados; 4 mortos | **CONFIRMADO** (mesmo método; sem segunda fonte) | arquivos `.d` do build Release; linhas de código fora de comentários |
| c) 55 ELF versionados; exemplos e paper na árvore de fontes; testes no build | **CONFIRMADO** | `git ls-files \| file`: 55; `examples/CMakeLists.txt:125`, `paper/CMakeLists.txt:76` (`"${dir_abs}"`); `tests/CMakeLists.txt:191` |
| d) vazamento do CGAL | **CONFIRMADO, e subestimado no P03** | `Core/type.h:66,80,115,116`; `ClippedVoronoiDiagram2D.hpp:158` (`DelaunayTriangulation2D delaunay{}`). Ver o problema 1 |
| e) só domínio convexo de um anel | **CONFIRMADO** | `VoronoiCellBuilder2D.hpp:118` (`NotImplemented`), `:131` (`is_simple_ccw_convex`) |
| f) faces casadas por proximidade geométrica | **CONFIRMADO** | `VoronoiFaceConnectivity2D.hpp:107` (tolerância 64·eps·escala), `:133–147` (buckets 3 × 3) |
| g) `std::uniform_real_distribution` | **CONFIRMADO** | `SiteFactory.hpp:932–934` |
| h) `VerifyCGALHeaders.cmake:48`; GMock implícito | **CONFIRMADO** | `check_cxx_source_compiles` na linha 48; `tests/CMakeLists.txt:181` (`if(TARGET GTest::gmock)`) |
| i) experimento de ordem de inserção | **CONFIRMADO** (reexecutado) | mesma saída: 0 polígonos diferentes, 0 vizinhos diferentes, max\|dA\| = 1,09e-16 |

## 3. Problemas encontrados

**IMPORTANTE 1: o vazamento do CGAL é estrutural, não pontual (§8 do P03).**
- **[F]** **93 de 137 headers públicos incluem o CGAL transitivamente.** O caminho é `Boundary2DTypes.hpp:28` → `Core/constants.h` → `Core/type.h` → `<CGAL/…>`. Além disso, `Real = Kernel::FT` (`type.h:115`) é o `Real` de todo o código (`Boundary2DTypes.hpp:41`, `Site2D.hpp:50`).
- **Conclusão:** a frase do P03 de que "`Point2` POD próprio … não usa o CGAL" é **enganosa**. O layout é próprio, mas o header e o tipo escalar dependem do CGAL.
- **Correção:** reescrever o trecho na §8. Isso também reforça a DEC-011.

**IMPORTANTE 2: a tarefa 4 foi cumprida só em parte.** O prompt pede cobertura **por arquivo**. O CSV traz a cobertura por tipo, e o relatório cita um `cov_files.csv` que **não foi entregue**. **Correção:** anexar a tabela por arquivo (62 linhas) ao entregável.

**IMPORTANTE 3: há inconsistências no CSV.**
- `VolumeNumberingState2D` está como **APROVEITAR**, mas usa o enum de despacho `VolumeNumberingMethod2D`, segundo a própria coluna `enum_despacho_usado`. Deveria ser REFATORAR.
- 11 linhas dizem "funciona e é testado", mas a coluna `testes_que_citam` está vazia: `SiteValidationReport`, `SiteSpacingCell2D`, `SiteSpacingCellHash2D`, `SiteSpacingIndex2D`, `LloydIterationStats2D`, `LloydResult2D`, `ShortVoronoiEdge2D`, `VoronoiGenerationTimings2D`, `TimedVoronoiGeneration2D`, `VolumeRenumbering2D` e `CentroidBox`.
- `BoundaryShortEdgeCollapseResult2D` herdou a justificativa da struct de opções ("enum de política"), que não se aplica a ela.
- **Correção:** refazer a regra de classificação por tipo, não por arquivo. Com isso, a contagem 51/60/4 muda.

**IMPORTANTE 4: tolerâncias absolutas, sem escala, quase não aparecem no P03.**
- **[F]** `kEpsilon = 1e-6` (absoluto) é usado em `Halfplane2D.hpp:45,51`, `BisectorHalfplane.hpp:39`, `BoundaryConditionPoint2D.hpp:84` e `VoronoiCell2D.hpp:251`.
- **[I]** Um domínio em milímetros, ou em UTM (~10⁶ m), muda o comportamento desses testes. O P03 cita as "tolerâncias fixas" numa célula de tabela, mas não aponta as linhas nem o risco para os casos ambientais.
- **Correção:** listar esses pontos e encaminhar ao P05 (política de tolerâncias).

**MENOR 5.** O experimento cobre só um retângulo e três padrões de sítios. Não testa sítios quase coincidentes, densidade muito variável nem domínios estreitos. A conclusão "Delaunay independente da ordem" deve se limitar a esses casos.

**MENOR 6.** `VMM_ENABLE_LTO=ON` por padrão não foi avaliado, e há um aviso no próprio código sobre o GCC internalizando o CGAL com LTO (`ConfigCompiler.cmake:56`). Pesa na portabilidade.

**MENOR 7.** A falta de memória com `-j8` depende do container e não deve ser apresentada como característica do projeto.

## 4. Omissões relevantes

- **Headers que puxam o CGAL (problema 1):** é a métrica mais direta para a DEC-007, e o P03 não a mediu.
- **Nomes com "2D" fixo em tipos e namespaces:** `vd2d`, `s2d`, `…2D`. O custo de chegar ao modelo genérico em dimensão (P06) não foi dimensionado.
- **Contrato de erros:** não foi avaliado se o que os construtores lançam hoje (`VMM_THROW` × `std::runtime_error`) é testado.

## 5. Parecer sobre as propostas

- **DEC-011: APROVAR COM MUDANÇAS.** A evidência do problema 1 reforça a estratégia C. A decisão deve acrescentar duas coisas:
  1. uma definição operacional do "oráculo": malhas da VMMLib gravadas como golden files, com uma tolerância declarada;
  2. o critério de retirada da VMMLib: todos os casos do oráculo reproduzidos na estrutura nova e com testes por classe.
- **DEC-012: APROVAR COM MUDANÇAS.** Acrescentar:
  1. rever o `VMM_ENABLE_LTO` por padrão;
  2. a métrica "nenhum header público inclui o CGAL" como teste de CI.

## 6. Veredito

**APROVAR COM RESSALVAS.** Corrigir os problemas 1 a 4 numa v2.1 do P03 (sem novas conclusões, só precisão). A evidência oficial no WSL continua pendente. **Recomenda-se ainda uma revisão por uma IA independente**, dado o conflito de interesse.
