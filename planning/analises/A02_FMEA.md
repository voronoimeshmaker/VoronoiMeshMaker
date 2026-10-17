# A02 — FMEA do VoronoiMeshMaker

- **Data:** 2026-10-16
- **Objeto:** `main` do GitHub, commit `fb02bb7` (08/10/2026), versão 1.1.0 preparada e não publicada.
- **Ambiente:** sessão do Claude na nuvem, fora do WSL oficial (AGENTS.md); por isso, estes resultados não substituem a evidência do WSL. Ubuntu 24.04.5; GCC 14.2.0; CMake 3.28.3 (sistema), 3.29.6, 3.30.5, 3.31.6 e 4.4.4 (pip); CGAL 6.2.1 (a versão estável mais recente); Boost 1.83; GoogleTest 1.14; 2 núcleos.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.
- **Entradas:** etapas e achados do SIPOC (A01), código, testes, `planning/`, CI e os experimentos da §2.

## 1. Escalas

| Nota | Severidade (S) | Ocorrência (O) | Detecção (D) |
|---|---|---|---|
| 1–2 | sem efeito perceptível | improvável | detectado sempre, antes do uso |
| 3–4 | incômodo; contornável | rara, em casos especiais | detectado por teste ou relatório existente |
| 5–6 | resultado degradado ou retrabalho relevante | ocasional, em uso real previsível | detectado só por inspeção ou por quem conhece o problema |
| 7–8 | malha inutilizável para o objetivo, ou uso bloqueado | frequente no uso pretendido (rios, estuários, MohidNG) | detectado só tarde (no solver ou na integração) |
| 9–10 | resultado errado sem aviso | quase certa | não detectável com os controles atuais |

**NPR = S × O × D.** Faixas: **≥ 200 crítico**, 100–199 alto, < 100 moderado.

---

## 2. Execução e experimentos

### 2.1 Build e testes

| Verificação | Resultado | Evidência |
|---|---|---|
| Configuração com CMake 3.28.3 (mínimo declarado em `CMakeLists.txt:1`; versão do Ubuntu 24.04) | **falha** no `try_compile` de `VerifyCGALHeaders.cmake:46-50`: `Target ... links to: CGAL::CGAL but the target was not found` | [F] `cfg.log`; repetido com o 3.28.3 do pip, com CGAL instalado e com CGAL da árvore-fonte |
| Configuração com CMake 3.29.6, 3.30.5, 3.31.6 e 4.4.4 | passa | [F] |
| Build Release (padrão: LTO, `-march=native`, `-Werror`), GCC 14.2, 2 núcleos | 286/286 alvos, **0 avisos**, 2298 s | [F] `build.log` |
| `ctest -L vmm` (Release) | **324/324 aprovados**; 214 s de parede com `-j2`; 23 testes `slow` somam 368 s·proc | [F] `ctest.log` |
| `check_requirements.py` | 0 problemas | [F] |
| CI no GitHub para `fb02bb7` | `vmm-ci` e `vmm-docs` com sucesso | [F] página do Actions em 16/10 |
| Cobertura, sanitizadores, Clang 19 e documentação | **não executados aqui** (custo de build com 2 núcleos); a evidência é a da CI e dos relatórios | — |

### 2.2 Experimentos com o `vmm-mesh` (Release, uma amostra, máquina compartilhada com outro processo)

**a) Pares espelhados e CVT (2D, `interface_pairs = 0.05`, sítios uniformes de 0,05).** Não ortogonalidade das faces internas medida a partir do `.vmesh` (ângulo entre a normal da face e a reta entre os geradores; script `tools/nonortho.py`):

| Caso | Faces de interface | Máx. (rad) | Média (rad) | Faces > 0,01 rad |
|---|---:|---:|---:|---:|
| Interface reta, sem pares | 50 | 1,06 | 0,46 | 50 |
| Interface reta, com pares | 22 | 1,06 | 0,11 | 4 |
| Interface reta, com pares + CVT (10 iterações) | 40 | 1,02 | 0,48 | 38 |
| Interface circular (r = 0,3), sem pares | 160 | 1,10 | 0,42 | 157 |
| Interface circular, com pares | **arquivo idêntico ao sem pares** (`cmp`) | — | — | — |

**[F]** O CVT desfaz a ortogonalidade que os pares dão. **[F]** Na interface curva, os pares não são gerados, e nada avisa: o círculo vira 64 segmentos de cerca de 0,029, mais curtos que a folga de 1 × `spacing` exigida em cada ponta do segmento (`sites/sources.hpp:142-144`). As faces internas de uma mesma região ficam em ~10⁻⁸ rad em todos os casos (DEC-032).

**b) CVT, escala e convergência.**

| Caso | Células | Geração (s) | Geração + CVT (s) | CVT |
|---|---:|---:|---:|---|
| 2D, quadrado, 5 iterações | 7 164 | 0,21 | 0,81 | não convergiu |
| 2D, quadrado, 5 iterações | 28 333 | 0,88 | 2,99 | não convergiu |
| 2D, 200 iterações, tolerância padrão 10⁻⁶ | 7 164 | — | 21,0 | **não convergiu** |
| 2D, tolerância 10⁻³ | 1 797 | — | 0,30 | convergiu em 6 |
| 3D, cubo, 3 iterações | 2 592 | 2,98 | 15,3 | não convergiu |
| 3D, cubo, 3 iterações | 7 565 | 7,45 | 34,7 | não convergiu |

[F] Em 3D, cada iteração custa cerca de 1,2 ms por célula, porque a malha inteira é reconstruída e revalidada a cada tentativa (`src/facade/cvt.cpp:96-117` e `156-170`). [I] 10⁶ células × 50 iterações ≈ 17 h numa thread. [F] A verificação do suporte das interfaces percorre todos os segmentos/triângulos da partição para cada ponto testado (`src/facade/cvt.cpp:47-69`). [I] Com uma costa ou um STL de 10⁵ elementos, isso domina o custo.

**c) Colunas e horizontes (`generate_layered_mesh` via `vmm-mesh`, base em grade, 2 camadas).**

| Colunas | Células | Tempo (s) | Pico de memória (KiB) |
|---:|---:|---:|---:|
| 3 969 | 7 938 | 16,6 | 34 004 |
| 16 129 | 32 258 | 264,2 | 113 196 |

[F] 4,1× mais colunas custaram 15,9× mais tempo (expoente ≈ 1,97), o que confirma o crescimento quadrático descrito no `P27_desempenho.md`. [I] Extrapolação: 10⁵ colunas ≈ 2,8 h; 10⁶ colunas ≈ 12 dias.


---

## 3. Tabela FMEA

| ID | Etapa | Modo de falha | Efeito | Causa | Controle atual | S | O | D | NPR | Ação recomendada |
|---|---|---|---|---|---|---:|---:|---:|---:|---|
| F01 | Build | Configuração falha com o CMake mínimo declarado (3.28) | usuário do Ubuntu 24.04 LTS (CMake 3.28.3) não consegue compilar | `try_compile` de `VerifyCGALHeaders.cmake:46-50` liga ao alvo `CGAL::CGAL`, que o 3.28 não propaga | nenhum: a CI usa o CMake mais novo do runner | 7 | 7 | 8 | **392** | subir `cmake_minimum_required` para 3.29 **ou** trocar o `try_compile` por `CMAKE_REQUIRED_INCLUDES`; job de CI com o CMake mínimo |
| F02 | Build | Compiladores e plataformas fora da matriz falham sem aviso | quebra para quem usa GCC 15/16, Clang 20+, macOS ou Windows | CI só com gcc-14 e clang-19 no Ubuntu 24.04; R12 pede mínimo **e** mais recente | CI parcial | 5 | 5 | 6 | **150** | acrescentar à CI as versões mais recentes de GCC e Clang; declarar as plataformas suportadas |
| F03 | Instalação | Pacote instalado com o build padrão não roda em outra máquina ou não liga com outro compilador | `illegal instruction` ou erro de LTO no consumidor | padrão Release com `-march=native` e LTO (DEC-034) | documentação (DEC-034, README) | 6 | 5 | 5 | **150** | aviso no `install` quando `NATIVE_ARCH` ou `LTO` estiverem ligados; preset `package` |
| F04 | Publicação | Versões sem tag nem release | `find_package(VoronoiMeshMaker 1.0)` sem referência; citação e reprodução impossíveis; o site mostra 1.1 antes de ela sair | tags previstas no P20 §7 não foram criadas | nenhum | 6 | 9 | 3 | **162** | criar `v1.0.0` no commit da 1.0, publicar a 1.1 com tag e release; publicar o site só a partir de tags (ou com aviso de versão em desenvolvimento) |
| F05 | Processo | Commits entram no `main` com a CI falhando | `main` quebrado para quem clona; regressões descobertas depois | push direto no `main`, sem PR obrigatório | CI posterior ao push | 5 | 7 | 4 | **140** | proteger o `main`: PR com `vmm-ci` obrigatório |
| F06 | Processo | Afirmações de teste, cobertura e desempenho sem evidência verificável por terceiros | revisão (inclusive a do JOSS) não consegue conferir; erros de relatório passam | logs ficam em `build/`, fora do git; ambiente único (AGENTS.md); várias revalidações só com testes dirigidos (P27) | CI cobre o que é publicado | 5 | 9 | 7 | **315** | anexar os logs-chave aos relatórios (ou artefatos da CI); exigir suíte completa, não só testes dirigidos, antes de declarar uma entrega |
| F07 | Governança | Decisões tomadas fora do `DECISIONS.md` | perda de rastreabilidade; decisões conflitantes futuras | Q01–Q10 (horizontes) e as decisões do CVT ficaram nos pacotes `planning/horizontes` e `planning/cvt` | nenhum | 5 | 8 | 6 | **240** | registrar como DEC-043 em diante (horizontes, CVT, execução sem aprovação intermediária) |
| F08 | Verificação | R1 não é verificado para headers do topo (`cvt.hpp`, `layered.hpp`, `vmm.hpp`) | classe nova sem teste próprio passa na CI | `check_requirements.py:77-80` pula headers fora de módulos | revisão manual | 4 | 6 | 8 | **192** | cobrir os headers do topo (módulo `facade`) no verificador |
| F09 | Verificação | A medida de cobertura depende da versão do `gcovr` | limite R25 aprovado ou reprovado por mudança de ferramenta | `gcovr==7.2` fixado; o 8 conta diferente (P20 §6a) | fixação da versão | 4 | 5 | 5 | **100** | documentar a dependência; avaliar migração para o gcovr atual e recalibrar |
| F10 | Entrada | Sem espaçamento variável no `vmm-mesh` | usuário sem C++ não consegue malhas graduadas (rio, costa) | `AdaptiveQuadtreeSource`/`AdaptiveOctreeSource3D` não estão no registro de configuração (`src/app/registries.cpp`) | nenhum | 6 | 8 | 3 | **144** | fonte `adaptive` no `.cfg` com campo de espaçamento por arquivo ou por distância a um contorno |
| F11 | Entrada | Polígono 2D só com coordenadas em linha no `.cfg` | contornos reais (milhares de vértices) inviáveis sem C++ | forma `polygon` lê só `xy`; `file` existe só para `stl` | nenhum | 5 | 8 | 2 | **80** | `polygon` com `file` (CSV/WKT simples) |
| F12 | Entrada | Sem entradas geoespaciais (raster, XYZ, sistema de coordenadas) | pré-processamento manual para todo caso real | fora do nicho (DEC-001: não é pipeline GIS) | decisão de escopo | 5 | 9 | 2 | **90** | manter fora do núcleo; documentar o caminho recomendado (ferramenta externa) |
| F13 | Sítios | Pares espelhados não são gerados em interfaces curvas ou finamente segmentadas, sem aviso | usuário pede ortogonalidade na interface e recebe faces de até 1,1 rad | folga de 1 × `spacing` em cada ponta de **cada segmento** (`sources.hpp:142-144`); segmentos mais curtos que 2 × `spacing` ficam sem pares | nenhum (arquivo idêntico, §2.2a) | 7 | 8 | 8 | **448** | gerar os pares ao longo da polilinha da interface (não por segmento); devolver contagem e aviso quando uma interface ficar sem pares |
| F14 | Malha | Faces de interface não ortogonais mesmo com pares | fluxo de dois pontos com erro na interface (o caso do MohidNG) | sem pares: até 1,8 rad em 3D (P19); com pares: falha em quinas convexas e nas pontas (DEC-028; §2.2a: 4 de 22) | `quality_report` | 7 | 7 | 4 | **196** | tratar quinas e pontas (pares em leque, sítio no vértice); relatar a não ortogonalidade por interface no `vmm-mesh` |
| F15 | Malha 3D | Degenerescência maciça (centenas de sítios cosféricos) gera célula inválida | a geração falha | construção em double nas células (P19 §5.1) | invariantes acusam | 6 | 3 | 3 | **54** | recorte convexo exato por célula, como o P18 §5 propõe |
| F16 | Malha | Célula partida pelo domínio fica inteira, com pedaços desconexos | célula com centroide fora de si; problemas em solver e no CVT | DEC-035 | `BuildStats::fragmented_cells` e aviso no log | 6 | 4 | 4 | **96** | expor o contador no `vmm-mesh`; opção de falhar quando houver fragmentação |
| F17 | CVT | O CVT desfaz a ortogonalidade dos pares espelhados | interface volta a ter faces de até 1 rad (38 de 40, §2.2a) | os sítios dos pares se movem de forma independente; o `SiteSet` não sabe quais são pares nem quais são fixos | nenhum | 7 | 8 | 7 | **392** | pares como entidade do CVT (movidos juntos ao longo da interface, ou fixos); opção de sítios fixos |
| F18 | CVT | O CVT de densidade uniforme desfaz a graduação de tamanho [I] | malhas refinadas na costa ou no leito ficam uniformes dentro de cada região | Lloyd sem densidade (`cvt.hpp:19`) | documentado em `cvt/01_entrega.md` | 7 | 8 | 6 | **336** | CVT com densidade derivada do `SpacingField` (ρ ∝ h⁻ᴰ); teste que preserve a graduação |
| F19 | CVT | Custo do CVT inviável para malhas grandes | 10⁶ células em 3D ≈ 17 h para 50 iterações [I] | reconstrução e validação completas a cada tentativa (`cvt.cpp:96-117`, `156-170`); suporte verificado por varredura linear da partição (`cvt.cpp:47-69`) | nenhum | 6 | 8 | 4 | **192** | índice espacial na verificação de suporte; reconstrução incremental ou só das células afetadas; benchmark até 10⁵ |
| F20 | CVT | Tolerância padrão praticamente inalcançável | `converged = false` mesmo depois de 200 iterações (§2.2b) | Lloyd puro (convergência lenta) com `relative_tolerance = 1e-6` | relatório de convergência | 4 | 9 | 3 | **108** | tolerância padrão realista ou aceleração (Anderson, L-BFGS) |
| F21 | CVT | Um sítio inválido rejeita o passo de todos [I] | estagnação em domínios não convexos (costas, meandros) | passo global com busca de passo única (`cvt.cpp:156-181`) | `stalled` no relatório | 6 | 6 | 4 | **144** | passo por sítio, com projeção do sítio para dentro da região |
| F22 | CVT × colunas | O resultado do CVT perde a organização em colunas | não serve ao consumidor em colunas (MohidNG, H04) | `cvt_domain` devolve `Partition3D`; o CVT devolve `Mesh3D` (`cvt.hpp:52-63`) | documentado | 6 | 7 | 3 | **126** | CVT na base 2D antes da extrusão, como fluxo recomendado para colunas |
| F23 | Colunas | Custo quadrático no número de colunas | 10⁵ colunas ≈ 2,8 h; 10⁶ ≈ 12 dias [I] (§2.2c) | classificação por enrolamento percorre todas as arestas para cada triângulo (P29 §1) | documentado (P27, P29) | 8 | 9 | 4 | **288** | índice espacial ou propagação topológica; benchmark até 10⁵ colunas como critério de aceitação |
| F24 | Colunas | Células finas e muito inclinadas em pinch-outs e horizontes inclinados | passo de tempo minúsculo ou instabilidade no solver | sem espessura mínima, remoção de células finas nem ajuste do fundo (o MOHID tem `MININITIALLAYERTHICKNESS`) | nenhum relatório de qualidade específico para colunas | 6 | 7 | 5 | **210** | opções declaradas de espessura mínima e de tratamento do fundo; relatório de espessura e inclinação por coluna |
| F25 | Colunas | Horizontes só como gráficos z(x,y) numa grade fixa | dobras e superfícies multivaloradas fora | escopo (Q01) | documentado | 4 | 3 | 3 | **36** | manter; documentar |
| F26 | Saída | Pontos quase coincidentes colapsam ao converter o exato para double | falha da geração perto do limite de representação | saída em double (P29 §2) | rejeição explícita | 5 | 3 | 5 | **75** | manter a rejeição; teste de escala extrema documentado |
| F27 | Integração | Sem contrato de leitura para o MohidNG | integração tardia e cara; convenções (profundidade, k, índices) divergentes | H01/H02 tratam só do que o VMM entrega; não há especificação do lado do consumidor, nem leitor fora do C++ | nenhum | 7 | 8 | 7 | **392** | documento de contrato `.vmesh`/`.vlayers` para consumidores + leitor de referência mínimo e teste de ida e volta |
| F28 | Saída | Arquivos `.vlayers` antigos convivem com `.vmesh` novos | consumidor lê colunas que não correspondem à malha | CVT grava só `.vmesh` (`cvt/01_entrega.md`) | documentado | 3 | 5 | 6 | **90** | gravar um identificador comum (hash) nos dois arquivos e verificar na leitura |
| F29 | Regressão | Oráculo de regressão só para 2D de uma região (O1–O4, congelados) | mudança de comportamento em 3D e colunas passa despercebida entre versões | DEC-033 congelou o gerador; DEC-041 não garante malha idêntica entre versões menores | invariantes | 5 | 4 | 6 | **120** | golden files próprios (2D multirregião, 3D, colunas) gerados pelo VMM a cada versão, comparados por tolerância |
| F30 | Desempenho | Domínios STL grandes e malhas finas lentos e com memória dominada pelo domínio | 3,5 ms por célula recortada; até 7,9 KB/célula (B6) | uma thread; estruturas exatas do domínio (P19 §5) | benchmark B6 | 5 | 6 | 3 | **90** | paralelizar o recorte (adiado pelo João) |
| F31 | Código | Estilo inconsistente nos arquivos novos do CVT | revisão mais difícil | `cvt.hpp`/`cvt.cpp` sem espaçamento padrão; não há `.clang-format` | revisão | 2 | 8 | 3 | **48** | `.clang-format` do projeto e verificação na CI |


---

## 4. Resumo

**Distribuição:** 9 críticos (NPR ≥ 200), 13 altos e 9 moderados, num total de 31 modos de falha.

**Críticos, por NPR:**

| # | ID | Modo de falha | NPR |
|---|---|---|---:|
| 1 | F13 | Pares espelhados não são gerados em interfaces curvas ou finamente segmentadas, sem aviso | 448 |
| 2 | F01 | Configuração falha com o CMake mínimo declarado (3.28) | 392 |
| 3 | F17 | O CVT desfaz a ortogonalidade dos pares espelhados | 392 |
| 4 | F27 | Sem contrato de leitura para o MohidNG | 392 |
| 5 | F18 | O CVT de densidade uniforme desfaz a graduação de tamanho [I] | 336 |
| 6 | F06 | Afirmações de teste, cobertura e desempenho sem evidência verificável por terceiros | 315 |
| 7 | F23 | Custo quadrático no número de colunas | 288 |
| 8 | F07 | Decisões tomadas fora do `DECISIONS.md` | 240 |
| 9 | F24 | Células finas e muito inclinadas em pinch-outs e horizontes inclinados | 210 |

**Altos:** F14 (196), F08 (192), F19 (192), F04 (162), F02 (150), F03 (150), F10 (144), F21 (144), F05 (140), F22 (126), F29 (120), F20 (108), F09 (100).

**Moderados:** F16 (96), F12 (90), F28 (90), F30 (90), F11 (80), F26 (75), F15 (54), F31 (48), F25 (36).

**Padrões:**
1. **A geometria básica está sólida.** 324/324 testes, invariantes em 10⁻¹⁵, ortogonalidade das faces internas em ~10⁻⁸ rad, nenhum aviso de build. Nenhum modo de falha crítico está no núcleo 2D/3D de uma região.
2. **Os riscos se concentram nas interfaces e nas extensões da 1.1:**
   - pares espelhados (F13, F14, F17);
   - CVT (F17–F22);
   - colunas (F23, F24).
   Justamente as partes que o MohidNG vai usar.
3. **Escala:** colunas e CVT foram validados só em malhas pequenas. As medições mostram crescimento quadrático (colunas) ou custo por iteração alto (CVT 3D).
4. **Processo:** instalação (F01–F03), publicação (F04), evidência (F06) e decisões (F07). Nada disso exige mudar a geometria, mas impede que a biblioteca seja usada e citada por terceiros como prevê a DEC-041.

