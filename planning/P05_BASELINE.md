# P05 — Linha de base: requisitos, diretrizes e roteiro

- **Data:** 2026-09-28
- **Sequência:** v5.1, prompt P05
- **Destino no repositório:** `planning/P05_BASELINE.md` e `planning/project_guidelines.tex`
- **Convenção:** **[F]** fato verificado (com fonte) · **[I]** inferência · **[R]** recomendação · **NÃO VERIFICADO** quando não foi possível obter.
- **Natureza:** documental. Nenhum código, CMake ou teste foi alterado.
- **Regra a partir daqui:** esta linha de base, depois de aprovada, só muda por revisão explícita (nova versão deste arquivo e entrada no `DECISIONS.md`).

---

## 1. Ambiente e entradas

| Item | Valor |
|---|---|
| Onde rodou | sessão do Claude na nuvem, fora do WSL oficial |
| Repositório lido | GitHub, commit `220cc6c` (`planning/`, `AGENTS.md`, `VoronoiGridMaker/docs/architecture/project_guidelines.tex`) |
| Build, testes, cobertura | não executados (prompt documental) |
| Entradas | `00_requisitos_iniciais.md`, P01 a P04, `DECISIONS.md` (DEC-001 a DEC-021) |

**Respostas do João ao P04 (28/09):**
- DEC-017 a DEC-021: aprovadas como propostas.
- Questionário da §9: valem todas as opções recomendadas.
- Pergunta 7: o consumidor principal da malha é o **solver próprio** do João. Consequência: o **formato nativo** e a **API de adjacência** (DEC-015) são os pontos de saída prioritários; os escritores de solver externo vêm depois.

**Onde fica o documento de diretrizes [F]:** o único `project_guidelines.tex` do repositório está em `VoronoiGridMaker/docs/architecture/`, na árvore histórica (DEC-011, DEC-014), e não é referenciado por nenhum outro arquivo. **[R]** A versão vigente passa a ser `planning/project_guidelines.tex`, reescrita por este prompt; a antiga fica como histórico até o P06 decidir o destino da árvore `VoronoiGridMaker/`. O P13 move a versão vigente para a documentação definitiva (DEC-026, PROPOSTA).

---

## 2. Requisitos da linha de base

Cada requisito tem uma justificativa e um **critério de verificação** executável ou revisável. "CI" é o workflow do P07; "revisão" é a revisão do João em cada iteração.

**Numeração:** R1 a R13 mantêm o número e o sentido de `00_requisitos_iniciais.md`, porque a sequência de prompts e as decisões já os citam. R14 em diante consolidam as DEC-001 a DEC-021.

### 2.1 Requisitos iniciais, agora verificáveis

| # | Requisito | Justificativa | Verificação |
|---|---|---|---|
| R1 | Toda classe não trivial tem o seu `ut_<Classe>.cpp`; classe modificada tem o teste atualizado. Classe trivial = agregado sem invariantes nem comportamento (tags, opções sem validação, registros de dados). | requisitos R1; DEC-023 | script de CI lista classes públicas sem teste próprio e compara com a lista de triviais declarada |
| R2 | `tests/` espelha a árvore da biblioteca, com descoberta automática. | requisitos R2 | script de CI compara as árvores |
| R3 | Sem funções virtuais, sem herança e sem enum usado para escolher implementação; extensão por templates, concepts, traits, policies, composição e registros abertos. | requisitos R3; AGENTS.md; DEC-024 | CI procura `virtual` e listas de base (`class X : ...`, `struct X : ...`) nos fontes da biblioteca; revisão para enums de despacho |
| R4 | DOD, DDD, SOLID por polimorfismo estático e SoC, verificados por regras concretas: grafo de dependências entre módulos sem ciclos; núcleo sem dependência de IO e de backend; dados base separados de dados derivados. | requisitos R4 | script de CI que extrai os includes entre módulos e falha em ciclo ou em dependência proibida |
| R5 | O domínio tem N ≥ 1 regiões, com fronteira fixa; cada região pertence a um meio de um registro aberto; um meio tem de zero a várias regiões. | requisitos R5 | testes A1 (duas regiões do mesmo meio em contato) e A2 (duas regiões do mesmo meio sem contato) |
| R6 | Interfaces entre regiões são conformes e rotuladas pelo par de regiões. | requisitos R6 | verificador de invariantes: cada face de interface tem exatamente uma célula de cada lado; comprimento/área de cada interface igual ao analítico |
| R7 | A malha é estática: depois de construída, não há operação que a modifique. | requisitos R7 | revisão da API: o objeto de malha só expõe acesso `const` depois da construção |
| R8 | O VMM não depende do PETSc nem de nenhum solver. | requisitos R8; DEC-015 | CI procura `petsc` em includes e no CMake |
| R9 | A conectividade e a adjacência são expostas com tipos próprios ou padrão, suficientes para montar matrizes esparsas sem reconstruir a geometria. | requisitos R9; DEC-015 | teste que monta o padrão CSR de uma matriz a partir da API, sem geometria |
| R10 | Subsistema de erros próprio, sem herança nem virtual, com código estável, contexto, `std::source_location`, entidade relacionada e mensagens pt/en por catálogo; a exceção não deriva de `std::exception`. | requisitos R10; DEC-016 | GTest do subsistema; teste de catálogo: todo código tem texto em pt e em en |
| R11 | Falhas previsíveis e recuperáveis são devolvidas por `std::expected`; exceção só para o que a arquitetura define como excepcional. | requisitos R11; DEC-016 | revisão da API pública; tabela de funções que lançam, mantida no P06 |
| R12 | Padrão C++23; compiladores mínimos GCC 14 e Clang 18. | requisitos R12; DEC-014, DEC-022 | `CMAKE_CXX_STANDARD 23` obrigatório; CI com GCC e Clang nas versões mínimas e nas mais recentes |
| R13 | Nome VoronoiMeshMaker; namespace público `vmm`. | requisitos R13; DEC-014, DEC-022 | CI: nenhum símbolo público fora de `vmm`; nenhuma ocorrência nova de "VoronoiGridMaker" fora da árvore histórica |

### 2.2 Produto e geometria

| # | Requisito | Justificativa | Verificação |
|---|---|---|---|
| R14 | O VMM recebe domínio, regiões e sítios e entrega um modelo de malha de volumes finitos: células, faces, owner/neighbour, centros, vetores de área, medidas, d_ij, não ortogonalidade, rótulos de região e de patch. | DEC-001 | teste de integração dos problemas A1 e A2 verifica a presença e a consistência de cada grandeza |
| R15 | As regiões são declaradas por precedência e convertidas numa partição explícita, verificada por um validador de vazios, regiões anuladas e lascas. | DEC-018 | GTest do validador com casos de sobreposição total, vazio e fronteira quase coincidente |
| R16 | Toda malha produzida satisfaz os invariantes da DEC-011: soma das medidas igual à do domínio e de cada região; fechamento de cada célula; owner/neighbour consistentes; interfaces conformes. | DEC-011 | verificador de invariantes rodado em todos os testes de integração e no benchmark |
| R17 | Tolerâncias geométricas relativas à diagonal L da caixa envolvente (padrão 10⁻¹² L); nenhuma tolerância absoluta no código geométrico. | DEC-020 | CI procura constantes absolutas nos diretórios de geometria; testes A1 escalados por 10⁻³ e 10⁶ dão a mesma topologia |
| R18 | Determinismo: mesma entrada, configuração, semente e build produzem malha idêntica bit a bit, incluindo a numeração; outra plataforma ou outra ordem de inserção produzem a mesma topologia e numeração canônica. | DEC-020 | teste que gera duas vezes e compara somas de verificação; teste com 5 permutações dos sítios |

### 2.3 Arquitetura, dependências, licença e build

| # | Requisito | Justificativa | Verificação |
|---|---|---|---|
| R19 | `vmm_core` não depende do CGAL; nenhum header público inclui o CGAL; o backend é policy interna. | DEC-006, DEC-007 | CI compila cada header público sem o CGAL no caminho de includes |
| R20 | Tipo escalar público `vmm::Real = double`; nenhum tipo do CGAL na API. | DEC-006, DEC-011 | revisão; teste de compilação dos headers públicos sem o CGAL (R19) |
| R21 | Dependências: obrigatórias CGAL e Boost (headers); opcionais GMP, MPFR, TBB; GoogleTest só nos testes. Nenhuma outra sem nova DEC. | DEC-004, DEC-009 | CI compara os `find_package` do CMake com a lista permitida |
| R22 | Versões: só estáveis; versão mínima declarada no CMake; versões usadas registradas em cada build, teste e relatório. | DEC-012 | configure imprime as versões; relatório de cada prompt as registra |
| R23 | Licença: `vmm_core` BSD-3-Clause; `vmm_backend_cgal` com as obrigações da GPL; SPDX em todo arquivo-fonte. | DEC-008 | script de CI verifica o cabeçalho SPDX |
| R24 | Build limpo: sem `-ffast-math`; `NATIVE_ARCH` e `LTO` desligados por padrão; saída de exemplos fora da árvore de fontes; nenhum binário versionado. | DEC-012 | CI com `-Werror` nos alvos do VMM; verificação de que `git status` fica limpo depois do build |

### 2.4 Testes, qualidade e desempenho

| # | Requisito | Justificativa | Verificação |
|---|---|---|---|
| R25 | Cobertura do código novo: pelo menos 90% das linhas e 80% dos ramos por arquivo, medidas na CI. | DEC-023 | relatório do gcovr por arquivo; a CI falha abaixo do limite |
| R26 | Testes de propriedade (invariantes), golden files e casos patológicos (sítios quase cocirculares, colados à interface, regiões finas, escalas extremas) desde a primeira entrega. | DEC-011, DEC-020 | revisão do plano de testes de cada prompt de código |
| R27 | Benchmark leve B1, B2 e B3 a cada entrega; metas de tempo e memória da DEC-020, recalibráveis uma vez no P07. | DEC-020, DEC-021 | relatório do benchmark anexado ao relatório de cada entrega |

### 2.5 Saída e documentação

| # | Requisito | Justificativa | Verificação |
|---|---|---|---|
| R28 | Saída da 0.1: formato nativo do VMM (prioritário), VTK XML e OpenFOAM polyMesh. Da 0.2: MODFLOW 6, PFLOTRAN e TOUGH. Escritores fora do núcleo. | DEC-019; resposta à pergunta 7 | teste de ida e volta do formato nativo; `checkMesh` do OpenFOAM quando disponível |
| R29 | Documentação no modelo PETSc (seções fixas) e galeria sphinx-gallery com a paleta "Estuário"; bilíngue pt/en; um exemplo que falha quebra o build da documentação. | requisitos §6 | build da documentação na CI |
| R30 | Um exemplo só entra na galeria depois que os testes de classe e de integração das classes que ele usa passam. | AGENTS.md | revisão; o build da galeria depende do alvo de testes |

---

## 3. Diretrizes

Reescritas como regras verificáveis em `planning/project_guidelines.tex`. Em relação à versão histórica (`VoronoiGridMaker/docs/architecture/project_guidelines.tex`):

| Tema | Versão histórica | Linha de base |
|---|---|---|
| SOLID | "SOLID clássico não será adotado" | SOLID por polimorfismo estático, com regras verificáveis (R4) |
| Herança e virtual | "não serão os mecanismos centrais" | proibidos (R3) |
| Enums | não tratado | enum de dado permitido; enum de despacho proibido (DEC-024) |
| Escopo da V1 | domínio conexo, **sem** múltiplos materiais nem interfaces | multirregião conforme já na 0.1 (R5, R6, R15) |
| Sítios no contorno | proibidos | mantido: sítios estritamente no interior das regiões |
| Tolerâncias | não tratado | relativas à escala (R17) |
| Dependências | CGAL, GMP+MPFR, Boost, GTest, spdlog, yaml-cpp | lista da DEC-009 (R21); logging por fachada própria, sem spdlog no núcleo |
| Exceções | "sistema próprio" | DEC-016 (R10, R11) |
| Padrão, nome, namespace | C++23; VoronoiGridMaker | C++23 com compiladores mínimos; VoronoiMeshMaker; `vmm` (R12, R13) |
| Testes | regressão e patológicos | + propriedade, cobertura mínima, classe trivial definida (R1, R2, R25, R26) |
| Portabilidade | não tratado | CI pública e AGENTS.md convivem (§3.1) |

### 3.1 CI pública × AGENTS.md

- O `AGENTS.md` vale para agentes que trabalham na árvore local do João: ambiente único (WSL `Ubuntu-26.04-Test`), sem commit nem push, e resultados de outro ambiente não valem como evidência para a árvore local.
- A CI valida **commits publicados**, não a árvore de trabalho. As duas evidências são complementares e não se substituem: um relatório de prompt cita a evidência do WSL; a CI protege o que já foi commitado.
- **[I]** Não há conflito: a CI não altera a árvore local, e o agente local não usa a CI como evidência.

---

## 4. Roteiro de entregas

| Versão | Entrega | Escopo | Critério de aceitação |
|---|---|---|---|
| **0.1** | (a) 2D multirregião | domínio por precedência; sítios por região; Voronoi recortado multirregião conforme; topologia, classificação, métricas, reordenação; formato nativo, VTK XML e OpenFOAM; documentação e galeria | A1 (com a variante de ar) e A2 com todos os invariantes; B1 igual à VMMLib dentro da tolerância; metas 2D atingidas ou recalibradas; R1–R30 verificados; site gerado |
| **0.2** | escritores de subsuperfície | MODFLOW 6 (DISV e DISU), PFLOTRAN, TOUGH | cada escritor com teste de ida e volta ou validação pelo leitor oficial, quando disponível |
| **0.3** | (b) 3D, uma região, domínio analítico | depois do P15a; mesmo modelo de dados | invariantes 3D; caso 3D do benchmark; metas 3D atingidas ou recalibradas |
| **0.4** | (c) 3D com domínio STL | leitura, reparo e validação da superfície; patches | variante STL do A3 com invariantes |
| **0.5** | (d) 3D multirregião | espelhamento e corte, junções triplas, arestas vivas | A3 completo com invariantes e conformidade |
| **1.0** | API estável | congelamento da API pública | nenhuma quebra de API entre 0.5 e 1.0 sem nova DEC |

**[R]** Submeter ao JOSS a partir da 0.1, que já entrega o nicho N1 (DEC-001) no 2D.

---

## 5. Riscos

| # | Risco | Mitigação |
|---|---|---|
| 1 | Multirregião conforme na 0.1 é mais difícil do que o escopo da V1 histórica | P05a prova o 2D com duas regiões e um buraco antes do P06 |
| 2 | Cobertura mínima de 90%/80% atrasa as iterações | vale só para o código novo; exceções declaradas por arquivo, com justificativa |
| 3 | Regras verificáveis por script (R3, R4, R19) geram falsos positivos | listas de exceção versionadas e revisadas pelo João |
| 4 | GCC 14 e Clang 18 como mínimos excluem distribuições antigas | só afeta usuários que compilam a biblioteca; documentado na instalação |
| 5 | O 3D (0.3 a 0.5) depende do P15a | roteiro revisado depois do P15a (DEC-013) |
| 6 | Mudanças de escopo depois da aprovação | só por revisão explícita desta linha de base |

---

## 6. Ajustes propostos à sequência (v5.2)

Aplicados só depois da aprovação desta linha de base:

1. **P06:** incluir o formato nativo (DEC-019), a representação interna da partição de regiões (DEC-018), a política de índices (largura dos IDs) e a lista inicial de classes triviais (DEC-023).
2. **P07:** implantar os verificadores de R1, R2, R3, R4, R8, R19, R21, R23 e R25; os casos B1–B3 (DEC-021); os compiladores mínimos (DEC-022).
3. **P09:** trocar "Composição por precedência" pelo texto da DEC-018, com o validador de vazios, regiões anuladas e lascas.
4. **P12:** escritores da 0.1 na ordem formato nativo → VTK XML → OpenFOAM; os da 0.2 saem do P12 e ganham um prompt próprio depois do P14.
5. **P13:** mover `planning/project_guidelines.tex` para a documentação definitiva (DEC-026).
6. **REV:** citar a DEC-024 junto do R3 na tarefa 1 (enum de despacho).

---

## 7. Entradas para `planning/DECISIONS.md`

- DEC-017 a DEC-021: **APROVADAS** pelo João em 28/09 (respostas ao P04).
- Propostas deste prompt (status PROPOSTA):
  - **DEC-022 — Compiladores mínimos e namespace público**;
  - **DEC-023 — Classe trivial e cobertura mínima**;
  - **DEC-024 — Enums de dado × enums de despacho**;
  - **DEC-025 — Roteiro de versões**;
  - **DEC-026 — Documento de diretrizes vigente em `planning/`**.

---

## 8. Resumo e decisões pendentes

**Resumo**
- 30 requisitos, cada um com justificativa e critério de verificação: R1–R13 mantêm o número e o sentido originais; R14–R30 consolidam as DEC-001 a DEC-021.
- Diretrizes reescritas como regras verificáveis em `planning/project_guidelines.tex`; a versão histórica fica em `VoronoiGridMaker/`.
- Mudanças de fundo em relação à V1 histórica: multirregião já na 0.1; SOLID por polimorfismo estático; enum de despacho proibido; tolerâncias relativas; dependências reduzidas.
- Roteiro: 0.1 (2D multirregião), 0.2 (escritores de subsuperfície), 0.3 a 0.5 (3D), 1.0 (API estável).
- O formato nativo e a API de adjacência são a saída prioritária, porque o consumidor é o solver próprio do João.

**Decisões pendentes para o João**
1. Aprovar a linha de base (R1–R30, §2)? Recomendação: aprovar.
2. Aprovar as diretrizes reescritas (`planning/project_guidelines.tex`)? Recomendação: aprovar.
3. DEC-022 — GCC 14 e Clang 18 como mínimos; namespace `vmm`? Recomendação: aprovar.
4. DEC-023 — definição de classe trivial e cobertura de 90% das linhas e 80% dos ramos no código novo? Recomendação: aprovar.
5. DEC-024 — enums de dado permitidos, enums de despacho proibidos? Recomendação: aprovar.
6. DEC-025 — roteiro 0.1 a 1.0? Recomendação: aprovar.
7. DEC-026 — diretrizes vigentes em `planning/` até o P13? Recomendação: aprovar.
8. Aplicar os ajustes da §6 na sequência v5.2? Recomendação: aprovar.