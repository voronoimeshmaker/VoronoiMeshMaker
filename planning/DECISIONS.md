# Registro de decisões — VMM

- **Formato e regras:** ver `planning/sequencia_prompts.md`, seção "Registro de decisões".
- **Regra de edição:** entradas APROVADAS não são reescritas. Uma mudança gera uma nova entrada que cita a anterior.

## Índice

| DEC | Título | Status | Origem |
|---|---|---|---|
| 001 | Nicho do VMM | APROVADA | P01 |
| 002 | Backend 3D como policy substituível | APROVADA | P01 |
| 003 | Sequência v3 | APROVADA | P01 |
| 004 | Minimizar dependências externas | APROVADA | P02 |
| 005 | Sem Geogram | APROVADA | P02 |
| 006 | Divisão de responsabilidades VMM × backend | APROVADA | P02 |
| 007 | Firewall de compilação; backend como policy interna | APROVADA | P02 |
| 008 | Licença: vmm_core × vmm_backend_cgal | APROVADA | P02 |
| 009 | Dependências | APROVADA | P02 |
| 010 | Sequência v4 | APROVADA | P02 |
| 011 | Estrutura nova + migração seletiva | APROVADA | P03 |
| 012 | Higiene do repositório e do build; política de versões | APROVADA | P03 |
| 013 | Sequência v5 | APROVADA | P03 e revisão do João |
| 014 | Padrão C++23; nome VoronoiMeshMaker | APROVADA | requisitos (R12, R13) |
| 015 | Independência do PETSc | APROVADA | requisitos (R8, R9) |
| 016 | Subsistema de erros próprio | APROVADA | requisitos (R10, R11) |
| 017 | Problemas-âncora A1, A2 e A3 | APROVADA | P04 |
| 018 | Declaração de regiões por precedência | APROVADA | P04 |
| 019 | Formatos de saída por versão | APROVADA | P04 |
| 020 | Metas, determinismo e tolerâncias relativas | APROVADA | P04 |
| 021 | Benchmark leve B1, B2 e B3 | APROVADA | P04 |
| 022 | Compiladores mínimos e namespace público | APROVADA | P05 |
| 023 | Classe trivial e cobertura mínima | APROVADA | P05 |
| 024 | Enums de dado × enums de despacho | APROVADA | P05 |
| 025 | Roteiro de versões | APROVADA | P05 |
| 026 | Documento de diretrizes vigente em planning/ | APROVADA | P05 |
| 027 | Regras para protótipos em prototypes/ | APROVADA | P05a |
| 028 | Interface conforme na 0.1: refinamento comum com topologia por rótulos | APROVADA | P05a |
| 029 | Faces internas antes das de fronteira; views de volumes internos e de fronteira | APROVADA | P05a |
| 030 | Sem escritor OpenFOAM | APROVADA | João |
| 031 | Não ortogonalidade verificada só em faces não degeneradas | REJEITADA | P10 |
| 032 | Ortogonalidade garantida por construção | APROVADA | João |
| 033 | Retirada da VMMLib e do VoronoiGridMaker | APROVADA | João |
| 034 | Build padrão rápido | APROVADA | João |
| 035 | Célula partida pelo domínio fica inteira | APROVADA | P15a |
| 036 | Domínio 3D por superfícies trianguladas fechadas | APROVADA | P15 |
| 037 | Sem contração FMA | APROVADA | P16 |
| 038 | Roteiro renumerado: 3D publicado como 0.2 | APROVADA | P19 |
| 039 | Sem escritores MODFLOW 6, PFLOTRAN e TOUGH | APROVADA | João |

---

## DEC-001 — Nicho do VMM
- Data: 2026-09-28
- Origem: P01
- Status: APROVADA
- Decisão: O VMM é uma biblioteca C++ moderna que produz malhas Voronoi/PEBI prontas para solvers de volumes
  finitos, com sítios controlados pelo usuário, topologia, métricas geométricas, regiões, interfaces e patches
  (nicho N1). Produto: domínio + sítios → modelo de malha de VF (Mesh2D/Mesh3D). Não é um pipeline GIS completo.
- Justificativa: nenhuma das ferramentas analisadas reúne essas características (P01 §3); alinhada à competência
  do autor; entrega 2D viável.
- Consequências: P04 foca em casos-âncora como exemplos e em escritores de formato como plugins; P02 avalia o
  backend como policy substituível.

## DEC-002 — Backend 3D como policy substituível
- Data: 2026-09-28
- Origem: P01
- Status: APROVADA
- Decisão: O backend geométrico 3D fica obrigatoriamente atrás de um concept/policy substituível. A API pública e
  o modelo de dados do VMM não se acoplam a CGAL, Geogram ou qualquer outro backend; o solver só vê o modelo de
  malha do VMM. O P02 compara CGAL × Geogram e investiga o VoroCrust (inicialmente como referência e benchmark).
- Justificativa: há concorrentes fortes no algoritmo 3D; o valor do VMM está no modelo de VF e na API.
- Consequências: P02 ganha a comparação técnica CGAL × Geogram para Voronoi recortado 3D.

## DEC-003 — Sequência v3
- Data: 2026-09-28
- Origem: P01 §7
- Status: APROVADA
- Decisão: P02 inclui a comparação CGAL × Geogram e a verificação do VoroCrust; uma prova de conceito do
  recorte 3D (P15a) precede a arquitetura 3D.
- Justificativa: reduzir o risco técnico do 3D antes de fixar a arquitetura.
- Consequências: sequência de prompts passa à versão 3.

## DEC-004 — Minimizar dependências externas
- Data: 2026-09-28
- Origem: pedido do João durante o P02
- Status: APROVADA
- Decisão: O VMM deve depender do menor número possível de bibliotecas externas. Cada dependência precisa de
  justificativa explícita; dependências de conveniência (logging, configuração, GIS, formatos) são opcionais ou
  substituídas por código próprio pequeno.
- Justificativa: facilidade de instalação e manutenção para usuários externos; menor superfície de licença.
- Consequências: o P02 recomenda no máximo um backend geométrico obrigatório; a lista de dependências das
  diretrizes (CGAL, GMP+MPFR, Boost, GTest, spdlog, yaml-cpp) é revista no P02/P05.

## DEC-005 — Sem Geogram
- Data: 2026-09-28
- Origem: P02 (decisão do João)
- Status: APROVADA
- Decisão: O Geogram não será usado, nem como backend nem como dependência opcional.
- Justificativa: DEC-004 — evitar uma segunda biblioteca geométrica quando o CGAL já foi selecionado como backend.
  (TetGen e Triangle embutidos no Geogram podem ser desligados por opções de build; não são o motivo.)
- Consequências: CGAL é o único backend geométrico; VoroCrust (executável) é só referência externa no P15a.

## DEC-006 — Divisão de responsabilidades VMM × backend
- Data: 2026-09-28
- Origem: P02 §5
- Status: APROVADA
- Decisão: O backend responde às operações geométricas especializadas (triangulação → pares vizinhos, predicados
  exatos, consultas ao domínio, recorte exato de células de contorno 3D, IO/reparo de superfícies). O VMM constrói,
  possui e expõe a malha de volumes finitos. Nenhum tipo do CGAL aparece no modelo de dados do VMM.
- Justificativa: concentra no VMM o seu diferencial; delega o que exige robustez numérica especializada.
- Consequências: contratos precisos do backend definidos no P06.

## DEC-007 — Firewall de compilação; backend como policy interna
- Data: 2026-09-28
- Origem: P02 §5.3–5.4
- Status: APROVADA
- Decisão: GeometryBackend é uma policy interna de implementação, não um parâmetro da API do usuário; a API pública
  expõe só tipos do VMM (o concept pode virar ponto de extensão no futuro, só com tipos do VMM). Alvos CMake
  vmm_core (sem CGAL) e vmm_backend_cgal; headers do CGAL só em src/Backend/CGAL/*.cpp; instanciação explícita
  para D = 2, 3; CI compila os headers públicos sem o CGAL e um exemplo mínimo com vmm_core + backend de teste.
- Justificativa: DEC-002; impedir contaminação da API e do modelo de dados.
- Consequências: P06 define os tipos de fronteira; P07 cria os alvos e os testes de CI.

## DEC-008 — Licença: vmm_core × vmm_backend_cgal
- Data: 2026-09-28
- Origem: P02 §6
- Status: APROVADA
- Decisão: vmm_core sob BSD-3-Clause, sem dependência do CGAL. vmm_backend_cgal usa pacotes GPL do CGAL e fica
  sujeito às obrigações dessas licenças quando distribuído com a licença open source do CGAL; aplicações ou
  distribuições combinadas devem cumprir as obrigações GPL aplicáveis. Documentação clara (README "Licensing",
  aviso no CMake, SPDX, DCO), sem apresentar isso como aconselhamento jurídico.
- Justificativa: preservar a possibilidade de um backend permissivo futuro sem relicenciar o core.
- Consequências: P05 e P07 aplicam cabeçalhos, avisos e a divisão de alvos.

## DEC-009 — Dependências
- Data: 2026-09-28
- Origem: P02 §7
- Status: APROVADA
- Decisão: Obrigatórias: CGAL (headers) e Boost (headers). Multiprecisão padrão: Boost.Multiprecision.
  Opcionais: GMP, MPFR, TBB. GoogleTest só para testes. spdlog e yaml-cpp fora do núcleo.
- Justificativa: DEC-004.
- Consequências: P05 atualiza project_guidelines.tex; P08 cria a fachada de logging própria.

## DEC-010 — Sequência v4
- Data: 2026-09-28
- Origem: P02 §9
- Status: APROVADA
- Decisão: P15a usa o CGAL; VoroCrust só como referência externa quando possível; o P15a testa células convexas por
  semiespaços + recorte só das células necessárias, nos casos: domínio convexo, não convexo, arestas vivas, células
  de fronteira complexas, múltiplos componentes, quase degenerados, determinismo quanto à ordem de inserção.
  A arquitetura 3D definitiva só é decidida depois do P15a.
- Justificativa: validar experimentalmente a hipótese central do backend 3D antes de fixar a arquitetura.
- Consequências: sequência de prompts passa à versão 4.

## DEC-011 — Estratégia de base de código: estrutura nova + migração seletiva
- Data: 2026-09-28
- Origem: P03 §10–11
- Status: APROVADA
- Decisão: A nova arquitetura nasce numa estrutura nova (vmm_core sem CGAL + vmm_backend_cgal, conforme o P06), para
  a qual os componentes da VMMLib migram seletivamente, módulo a módulo, com os seus testes. Salvaguardas:
  (1) verificação por invariantes geométricos como critério principal, em 2D e 3D: a soma das medidas das células é
  igual à do domínio; os vetores de área de cada célula fechada somam zero; owner e neighbour são consistentes; as
  interfaces entre regiões são conformes;
  (2) oráculo operacional como segunda checagem, só para o que a VMMLib cobre (2D, domínio convexo de um anel):
  a VMMLib continua compilando com os testes verdes, e as suas malhas em casos de referência são gravadas como
  golden files. A topologia deve bater exatamente nos casos sem degenerescência; as grandezas geométricas batem
  dentro de uma tolerância relativa à escala do domínio. Os golden files são gerados num build sem -ffast-math e
  sem VMM_ENABLE_NATIVE_ARCH, e cada um registra no cabeçalho o compilador, as flags e as versões das dependências.
  Se uma atualização de dependência estourar a tolerância, a causa é investigada; o oráculo só é regenerado com
  uma nota neste registro;
  (3) critério de retirada: a VMMLib é removida quando todos os casos do oráculo forem reproduzidos e todos os
  tipos migrados tiverem teste por classe;
  (4) a restrição de domínio convexo de um anel (VoronoiCellBuilder2D::validate_domain) não é herdada.
  A árvore VoronoiGridMaker/ é só referência histórica; a nova estrutura não é obrigada a reproduzi-la.
- Justificativa: o CGAL é incluído por 93 de 137 headers públicos e define o tipo Real; o objeto de saída
  (ClippedVoronoiDiagram2D) guarda a triangulação do CGAL; a DEC-007 exige alvos separados; o recorte só convexo e a
  topologia por proximidade geométrica precisam ser substituídos; 51 de 140 arquivos não são alcançados pelo build.
  Como as dependências acompanham as versões estáveis mais recentes (DEC-012), os golden files podem variar um pouco
  entre versões; por isso os invariantes são o critério principal.
- Consequências: P06 define a estrutura nova, o mapa de migração, os invariantes, os casos do oráculo e o critério
  de retirada; P07 gera os golden files; P08–P12 migram por iterações.

## DEC-012 — Higiene do repositório e do build; política de versões
- Data: 2026-09-28
- Origem: P03 §2, §3, §7, §8, §12; requisitos §4.4
- Status: APROVADA
- Decisão:
  (1) Repositório: remover do git os 55 binários e o .pyc versionados; gerar a saída de exemplos e paper fora da
  árvore de fontes; não migrar os 32 headers de encaminhamento nem os 25 placeholders; .gitattributes com fins de
  linha normalizados (LF).
  (2) Build: VMM_ENABLE_NATIVE_ARCH e VMM_ENABLE_LTO OFF por padrão; remover a opção -ffast-math; declarar o GMock
  como dependência dos testes; tornar portável a verificação de headers do CGAL; instrumentar a biblioteca na
  cobertura e medir linhas, ramos e funções; teste de CI "nenhum header público inclui o CGAL".
  (3) Versões: o projeto acompanha as versões estáveis mais recentes das dependências, sem fixar uma versão.
  Distinguem-se: a versão mínima suportada, declarada no CMake (por exemplo, find_package(CGAL 6.2 REQUIRED));
  as versões testadas, mantidas pela CI; e as versões efetivamente usadas, registradas em cada build, teste e
  execução relevante (relatório de cada prompt e cabeçalho dos golden files). Só versões estáveis: nunca beta,
  release candidate ou build interno.
- Justificativa: CI pública, reprodutibilidade, robustez geométrica, diffs revisáveis e medição honesta de cobertura.
  No P03, o ambiente oficial usava um CGAL 6.0-beta1 e depois um build interno 6.2.1-I-900; ambos foram trocados pela
  6.2.1 oficial.
- Consequências: P07 executa (1) e (2) e implanta (3); cada prompt passa a declarar o ambiente e as versões usadas.

## DEC-013 — Sequência v5
- Data: 2026-09-28
- Origem: revisão do P03 e decisões do João
- Status: APROVADA
- Decisão:
  (1) As revisões são feitas pelo João, não por outra IA; o prompt REV fica como opção, não como etapa obrigatória.
  (2) planning/ é a única fonte de verdade: prompts, relatórios, decisões e requisitos vivem lá, versionados no git.
  (3) Cada prompt declara o ambiente em que roda (WSL oficial ou outro) e registra as versões usadas (DEC-012).
  (4) Arquivos entregues um a um, completos; um commit por aprovação, feito pelo João; tags não obrigatórias.
  (5) P05a — provas de conceito curtas, 2D e 3D, antes do P06, para reduzir o risco da arquitetura.
  (6) P15a com critérios de sucesso explícitos, prazo e um plano de contingência caso a hipótese não se confirme.
  (7) Limite de tentativas por iteração; esgotado o limite, o prompt para e pede decisão ao João.
  (8) Iterações só de movimentação de arquivos são separadas das que mudam lógica.
  (9) Os testes por classe (R1/R2) são reorganizados junto com a migração de cada módulo.
  (10) A DEC-012 é executada dentro do P07.
  (11) A partir do P07, um benchmark leve (tempo e memória de 2 ou 3 casos fixos) acompanha cada entrega.
  (12) Índice de decisões no topo deste arquivo, em vez de um arquivo separado.
  (13) Só se pedem comandos ao João quando o resultado pode mudar uma decisão.
- Justificativa: reduzir retrabalho e cerimônia sem perder rastreabilidade; tirar o risco do 3D cedo.
- Consequências: sequência de prompts passa à versão 5 (planning/sequencia_prompts.md).

## DEC-014 — Padrão C++23; nome VoronoiMeshMaker
- Data: 2026-09-28
- Origem: requisitos R12, R13 e §8
- Status: APROVADA
- Decisão: O padrão oficial é C++23; seus recursos (em particular std::expected) são usados quando trazem benefício
  concreto e estão disponíveis nos compiladores suportados. O nome do projeto e da biblioteca é VoronoiMeshMaker;
  "VoronoiGridMaker" designa só a árvore histórica do repositório. O namespace público (candidato: vmm) continua em
  aberto.
- Justificativa: as diretrizes já previam C++23; std::expected sustenta a DEC-016; um nome único evita ambiguidade.
- Consequências: P05 formaliza o namespace e a lista de compiladores suportados; P07 ajusta o CMake para C++23.

## DEC-015 — Independência do PETSc
- Data: 2026-09-28
- Origem: requisitos R8, R9, §4.3 e §9
- Status: APROVADA
- Decisão: O VMM não depende do PETSc. Nenhum header, tipo, objeto ou chamada da API do PETSc aparece no núcleo, no
  backend ou na interface pública. O VMM expõe conectividade e adjacência com tipos próprios ou padrão do C++
  (estruturas compactas equivalentes a CSR ou views), suficientes para que a aplicação monte matrizes esparsas,
  pré-aloque, reordene ou particione sem reconstruir a geometria. A conversão para Mat, IS, AO ou equivalentes é
  responsabilidade da aplicação ou de uma camada externa.
- Justificativa: o VMM entrega dados de malha, não estruturas de um solver (DEC-001); DEC-004.
- Consequências: P06 define as estruturas de adjacência e as views; exemplos de integração com PETSc, se houver,
  ficam fora da biblioteca.

## DEC-016 — Subsistema de erros próprio
- Data: 2026-09-28
- Origem: requisitos R10, R11 e §5
- Status: APROVADA
- Decisão: O VMM tem um subsistema próprio de erros, sem herança e sem funções virtuais (R3), independente de CGAL,
  PETSc e aplicações. Falhas previsíveis e recuperáveis são devolvidas por tipo de resultado baseado em
  std::expected; condições excepcionais usam um tipo de exceção próprio, que não deriva de std::exception.
  O erro pode carregar código estável, categoria, mensagem, origem, contexto, std::source_location e a entidade
  relacionada (SiteId, CellId, FaceId, RegionId, PatchId). Subsistemas de outros projetos servem só de referência
  conceitual.
- Justificativa: R3; diagnósticos úteis em malhas grandes exigem apontar a entidade com problema.
- Consequências: P06 define a API do subsistema; P08 o implementa; IErrorLogger, ThreadLocalBufferLogger e
  VMMException da VMMLib não são migrados como estão.

## DEC-017 — Problemas-âncora A1, A2 e A3
- Data: 2026-09-28
- Origem: P04 §2
- Status: APROVADA (28/09, respostas do João ao P04)
- Decisão: Os casos de integração e da galeria são três problemas sintéticos, gerados por parâmetros:
  A1 — seção transversal de rio 2D (canal trapezoidal, duas camadas de solo, variante com ar);
  A2 — trecho de rio 2D em planta, com meandro e ilha; A3 — bloco 3D com canal de profundidade variável,
  duas camadas de solo e ar. Dados reais ficam como variantes opcionais da galeria.
- Justificativa: juntos cobrem não convexidade, buraco, múltiplos componentes, duas regiões do mesmo meio,
  arestas vivas, junções triplas e forte variação de densidade, com resposta analítica para os invariantes.
- Consequências: P06 usa A1 e A2 para validar a arquitetura da entrega (a); P09–P12 os transformam em testes de
  integração; A3 é revisado depois do P15a.

## DEC-018 — Declaração de regiões por precedência
- Data: 2026-09-28
- Origem: P04 §3
- Status: APROVADA (28/09, respostas do João ao P04)
- Decisão: Na API, as regiões são declaradas por precedência (CSG por diferença, na ordem declarada).
  Internamente, a declaração é convertida numa partição explícita, verificada por um validador que garante
  cobertura sem vazios (salvo região de fundo), aponta regiões vazias ou fragmentadas pela ordem e aponta lascas
  abaixo de uma fração da escala local.
- Justificativa: é a forma mais simples para o usuário e cobre os três problemas-âncora; o validador neutraliza a
  dependência de ordem e as sobreposições escondidas. Partição explícita pelo usuário é impraticável em 3D.
- Consequências: P06 define a representação interna da partição; a função rótulo fica como consulta interna e
  alternativa a avaliar no P15a para domínios STL.

## DEC-019 — Formatos de saída por versão
- Data: 2026-09-28
- Origem: P04 §4
- Status: APROVADA (28/09, respostas do João ao P04)
- Decisão: 0.1 — VTK XML, OpenFOAM polyMesh e um formato nativo do VMM (modelo de volumes finitos completo,
  versionado, usado também nos golden files). 0.2 — MODFLOW 6 (DISV e DISU), PFLOTRAN (UNSTRUCTURED_EXPLICIT) e
  TOUGH (MESH). CGNS opcional, sem prazo, atrás de opção de build. Gmsh fora. Todos são escritores fora do núcleo.
- Justificativa: DEC-001, DEC-004 e DEC-015; os formatos de subsuperfície são listas de conexões com distâncias e
  áreas, que o VMM já calcula; o Gmsh não representa células poliédricas gerais; o CGNS exige HDF5.
- Consequências: P06 define o formato nativo; P12 implementa os escritores da 0.1; a ordem dos escritores da 0.2
  depende do solver que o João usa.

## DEC-020 — Metas, determinismo e tolerâncias relativas
- Data: 2026-09-28
- Origem: P04 §5
- Status: APROVADA (28/09, respostas do João ao P04)
- Decisão: Metas iniciais — 2D: 10⁶ células em até 30 s e até 1 KB por célula; 3D: 10⁶ células em até 10 min e
  até 4 KB por célula (Release, 1 thread); faces internas de Voronoi com não ortogonalidade abaixo de 10⁻⁸ rad.
  Determinismo: bit a bit com mesma entrada, configuração, semente e build; mesma topologia e numeração entre
  plataformas e entre ordens de inserção. Tolerâncias relativas à diagonal L da caixa envolvente: 10⁻¹² L para
  pontos, 10⁻¹² de erro relativo para somas de medidas e golden files. As metas de tempo e memória podem ser
  recalibradas uma única vez com a linha de base do P07.
- Justificativa: dar critérios verificáveis às entregas; eliminar tolerâncias absolutas (P03 §9).
- Consequências: P05 incorpora as metas na linha de base; P07 mede a linha de base e implanta as tolerâncias nos
  verificadores de invariantes.

## DEC-021 — Benchmark leve B1, B2 e B3
- Data: 2026-09-28
- Origem: P04 §6
- Status: APROVADA (28/09, respostas do João ao P04)
- Decisão: B1 — quadrado unitário com 10⁶ sítios uniformes e semente fixa, comparável com a VMMLib; B2 — A1 sem ar,
  cerca de 5·10⁴ células; B3 — A2, cerca de 5·10⁵ células. Mede-se tempo por fase, pico de memória, invariantes e
  uma soma de verificação da topologia, em Release, a cada entrega.
- Justificativa: DEC-013; B1 é o único caso com oráculo da VMMLib; B2 e B3 exercitam multirregião.
- Consequências: P07 implanta o benchmark e registra a linha de base.

## DEC-022 — Compiladores mínimos e namespace público
- Data: 2026-09-28
- Origem: P05 §2 (R12, R13)
- Status: APROVADA (28/09, aprovação do P05)
- Decisão: Compiladores mínimos GCC 14 e Clang 18; a CI testa as versões mínimas e as mais recentes. Recursos de
  C++23 sem suporte completo nesses compiladores (por exemplo, std::mdspan) não são usados. Namespace público: vmm.
- Justificativa: std::expected existe desde a libstdc++ 12 e a libc++ 16; "deducing this" desde o GCC 14 e o
  Clang 18/19; std::mdspan só a partir da libstdc++ 16 (tabela de suporte a C++23 do cppreference). vmm já é o
  namespace da VMMLib e o candidato citado nos requisitos.
- Consequências: P07 configura a matriz de compiladores da CI; P06 organiza os sub-namespaces.

## DEC-023 — Classe trivial e cobertura mínima
- Data: 2026-09-28
- Origem: P05 §2 (R1, R25)
- Status: APROVADA (28/09, aprovação do P05)
- Decisão: Classe trivial é um agregado sem invariantes nem comportamento (tag, opção sem validação, registro de
  dados); só as classes não triviais exigem ut_<Classe>.cpp próprio, e a lista de triviais é declarada e revisada.
  O código novo tem pelo menos 90% das linhas e 80% dos ramos cobertos por arquivo, medidos na CI; exceções por
  arquivo exigem justificativa.
- Justificativa: torna o R1 verificável; os limites ficam próximos do que a VMMLib já atinge (94,9% das linhas e
  81,9% dos ramos, P03 v2.2) e evitam testes vazios só para cumprir número.
- Consequências: P06 declara a lista inicial de classes triviais; P07 implanta o limite na CI.

## DEC-024 — Enums de dado × enums de despacho
- Data: 2026-09-28
- Origem: P05 §3
- Status: APROVADA (28/09, aprovação do P05)
- Decisão: Enum de dado (descreve estado ou categoria, sem escolher implementação) é permitido. Enum de despacho
  (valor que, num switch ou cadeia de if, escolhe entre implementações alternativas) é proibido; a escolha é feita
  por policy em tempo de compilação ou por registro aberto de callables em tempo de execução.
- Justificativa: interpreta o AGENTS.md ("no enums or equivalent closed feature dispatch") de forma verificável e
  alinhada ao R3; o P03 separou os enums da VMMLib exatamente por esse critério.
- Consequências: P08 revê os enums de despacho da VMMLib na migração; o prompt REV verifica cada enum novo.

## DEC-025 — Roteiro de versões
- Data: 2026-09-28
- Origem: P05 §4
- Status: APROVADA (28/09, aprovação do P05)
- Decisão: 0.1 — entrega (a), 2D multirregião, com formato nativo, VTK XML e OpenFOAM; 0.2 — escritores MODFLOW 6,
  PFLOTRAN e TOUGH; 0.3 — entrega (b), 3D com uma região e domínio analítico; 0.4 — entrega (c), 3D com domínio
  STL; 0.5 — entrega (d), 3D multirregião; 1.0 — congelamento da API pública. Submissão ao JOSS a partir da 0.1.
- Justificativa: entregas verticais e publicáveis; o 3D só começa depois do P15a (DEC-010).
- Consequências: P14 prepara a 0.1; a sequência ganha um prompt próprio para os escritores da 0.2.

## DEC-026 — Documento de diretrizes vigente em planning/
- Data: 2026-09-28
- Origem: P05 §1
- Status: APROVADA (28/09, aprovação do P05)
- Decisão: A versão vigente das diretrizes é planning/project_guidelines.tex, reescrita no P05 como regras
  verificáveis. VoronoiGridMaker/docs/architecture/project_guidelines.tex fica como histórico até o P06 decidir o
  destino da árvore VoronoiGridMaker/. O P13 move a versão vigente para a documentação definitiva.
- Justificativa: o único project_guidelines.tex do repositório está na árvore histórica (DEC-011) e não é
  referenciado por nenhum outro arquivo; planning/ é a fonte única (DEC-013).
- Consequências: P06 e P13 atualizam os caminhos.

## DEC-027 — Regras para protótipos em prototypes/
- Data: 2026-09-28
- Origem: P05a, plano de iterações
- Status: APROVADA (29/09, respostas do João ao P05a)
- Decisão: Código de prova de conceito fica em prototypes/<prompt>/, com CMake próprio, fora do build principal e
  sem API estável. Está isento de teste por classe, árvore espelhada e cobertura mínima (R1, R2, R25), mas respeita
  o R3 (sem virtual, sem herança, sem enum de despacho) e usa tolerâncias relativas (R17). Cada protótipo termina
  num executável que verifica os invariantes e sai com código diferente de zero se algum falhar. Nada de
  prototypes/ entra no código de produção sem passar pela arquitetura (P06) e pelas regras completas.
- Justificativa: provas de conceito precisam ser rápidas, mas não podem validar formas que a arquitetura teria de
  proibir.
- Consequências: P05a e P15a seguem esta regra; P07 exclui prototypes/ da CI de cobertura e dos verificadores de
  R1 e R2.
## DEC-028 — Interface conforme na 0.1: refinamento comum com topologia por rótulos
- Data: 2026-09-29
- Origem: P05a §4 (planning/P05a_provas_conceito.md)
- Status: APROVADA (29/09, respostas do João ao P05a)
- Decisão: Na 0.1, cada região gera o seu Voronoi e o recorta pelo próprio polígono; a conformidade da interface é
  garantida pelo refinamento comum dos pontos de quebra dos dois lados (E1), com fusão de pontos a 10⁻¹² L
  (DEC-020). A topologia vem de rótulos de aresta propagados pelo backend e os vértices são recalculados de forma
  canônica a partir dos rótulos (circuncentro, bissetor × segmento, canto), nunca por proximidade. Sítios
  espelhados na interface (E2) entram como política opcional de geração de sítios sobre o mesmo pipeline, para
  recuperar a ortogonalidade onde funcionam.
- Justificativa: no P05a as duas estratégias cumpriram todos os invariantes em 3 escalas e 5 ordens; E1 garante a
  conformidade por construção, E2 recuperou ortogonalidade exata em 81 % do comprimento da interface, mas falha
  na quina convexa (face a 90°) e depende da fusão por tolerância. Alternativa descartada: E2 como mecanismo único
  (sem garantia de conformidade).
- Consequências: P06 define o formato dos rótulos na fronteira do backend, a política de células fragmentadas e a
  de faces degeneradas; P10/P11 implementam; P15a avalia a extensão a 3D.

## DEC-029 — Faces internas numeradas antes das de fronteira; views de volumes internos e de fronteira
- Data: 2026-09-29
- Origem: P05a, pedido do João durante a execução (iteradores de volumes internos e de fronteira)
- Status: APROVADA (29/09, respostas do João ao P05a)
- Decisão: A malha expõe dois intervalos de volumes (internos: nenhuma face de fronteira; de fronteira: ao menos
  uma), cada um com acesso às faces do volume, e dois intervalos de faces (internas, incluindo as de interface; de
  fronteira). As faces são renumeradas com as internas primeiro e as de fronteira agrupadas por patch, de modo que
  os intervalos de faces sejam spans contíguos; os de volumes são listas de índices precomputadas.
- Justificativa: no P05a as views filtradas (std::views::filter) funcionaram em 2D e 3D, mas não são iteráveis
  como objeto const e refazem o filtro a cada passada; intervalos contíguos são mais simples e baratos. A
  renumeração dos volumes fica livre para critérios de localidade (P06).
- Consequências: P06 fixa a API (tipos e nomes) e a ordem canônica das faces; P09/P10 implementam com testes por
  classe.

## DEC-030 — Sem escritor OpenFOAM
- Data: 2026-09-29
- Origem: instrução do João durante o P06 ("não vou usá-lo de forma alguma")
- Status: APROVADA (29/09, instrução do João)
- Decisão: SUBSTITUI a DEC-019 no ponto do OpenFOAM: o escritor OpenFOAM polyMesh sai da 0.1 e não entra em versão
  futura planejada. Os escritores da 0.1 passam a ser o formato nativo do VMM (com leitura) e o VTK XML (.vtu).
  O restante da DEC-019 (0.2: MODFLOW 6, PFLOTRAN, TOUGH; CGNS opcional; Gmsh fora) continua valendo.
- Justificativa: o consumidor da malha é o solver próprio do João (resposta à pergunta 7 do P04); o formato nativo
  e a API de adjacência (DEC-015) cobrem esse uso.
- Consequências: R28 e P12 perdem o OpenFOAM (extrusão 2D, patches empty, checkMesh); a ordem das faces da DEC-029
  continua valendo por ser útil ao solver próprio, não por exigência do OpenFOAM.

## DEC-031 — Não ortogonalidade verificada só em faces não degeneradas
- Data: 2026-09-29
- Origem: P10 (teste de 1000 configurações aleatórias, semente 599)
- Status: REJEITADA (29/09, João: a ortogonalidade é garantida pelo CGAL; substituída pela DEC-032)
- Decisão: O limite de 10⁻⁸ rad da DEC-020 para faces internas de uma região vale para faces de tamanho linear
  s ≥ max(10⁻⁶ · d, 64 · ε · L / 10⁻⁸), com d a distância entre os geradores, ε o épsilon de máquina e L a escala do
  domínio (para L = 1, cerca de 10⁻⁶; no A2, com L ≈ 2,2 km, cerca de 1,6 mm). Faces menores são contadas e têm a não
  ortogonalidade máxima relatada à parte (InvariantReport::tiny_faces, max_nonortho_tiny), sem reprovar a malha.
- Justificativa: a direção de uma face de tamanho s, calculada a partir de vértices com erro de arredondamento ε, só é
  conhecida até ε/s. Na semente 599, uma face quase degenerada (sítios quase cocirculares) teve 1,4·10⁻⁸ rad com todos
  os demais invariantes em 10⁻¹⁶; no benchmark (B1 com 10⁶ células, B3 = A2 em coordenadas de km) apareceram faces
  de 1,1–1,4·10⁻⁸ rad. Os vértices carregam erro de arredondamento ~ε·L, então a direção de uma face de tamanho s só é
  conhecida até ~ε·L/s. O achado 3 do P05a já apontava que a meta é mal posta para faces degeneradas.
- Consequências: P10 e P12 usam o critério; o relatório de qualidade continua listando as faces curtas (P04 §5).

## DEC-032 — Ortogonalidade garantida por construção
- Data: 2026-09-29
- Origem: instrução do João sobre a DEC-031
- Status: APROVADA (29/09, instrução do João)
- Decisão: SUBSTITUI a DEC-020 no ponto da não ortogonalidade. Uma face interna de uma região é, por construção, um
  pedaço do bissetor entre dois sítios vizinhos, escolhido pelo backend exato (CGAL: Delaunay e recorte rotulado); a
  ortogonalidade é, portanto, uma garantia estrutural e não um critério numérico. O ângulo medido sobre os vértices
  gravados em double só reflete o arredondamento e passa a ser relatado (InvariantReport, relatório de qualidade),
  sem reprovar a malha. A garantia estrutural continua verificada no construtor: toda peça de bissetor precisa ser
  vista pelas duas células vizinhas (senão, erro InterfaceNotConforming).
- Justificativa: confiança no CGAL para a topologia (DEC-006, DEC-007); a direção de uma face minúscula medida sobre
  coordenadas arredondadas é indeterminada (semente 599, benchmark), sem que a malha esteja errada.
- Consequências: a DEC-031 é rejeitada; InvariantReference perde nonorthogonality_limit; QualityLimits mantém um
  limite só para o relatório de qualidade.

## DEC-033 — Retirada da VMMLib e do VoronoiGridMaker
- Data: 2026-09-30
- Origem: instrução do João ("a VMMLib antiga deve sumir do mapa"; "ficar somente com a nova versão", depois de
  confirmar que a migração foi feita)
- Status: APROVADA (30/09, instrução do João); a remoção dos arquivos é feita por ele
- Decisão: o repositório fica só com a biblioteca nova (`vmm/`). Saem `VMMLib/`, `VoronoiGridMaker/`, os `examples/`,
  `tests/`, `paper/` e `docs_sphinx/` antigos, `.readthedocs.yaml`, `.cleanup.cmake`, os módulos CMake do legado
  e o gerador de golden files (`vmm/tools/golden/`). As diretrizes (`planning/project_guidelines.tex`) viram a página
  `vmm/docs/guidelines.rst` (DEC-026).
- Verificação do critério da DEC-011 (P06 §11): O1–O4 reproduzidos (vizinhos idênticos, área até 1,1·10⁻¹³); os 56
  arquivos "refatorar"/"mover" do mapa do P06 §15 têm destino implementado com teste por classe; nada da `vmm/` depende
  da VMMLib fora do gerador de golden files. Lacuna fechada antes da retirada: contornos com auto-interseção, buracos
  tocando o anel externo ou uns aos outros passam a ser rejeitados (`InvalidPolygon`) em `ShapeOutline::make`.
  Os textos de `VoronoiGridMaker/docs/theory/` eram só marcadores, sem conteúdo a aproveitar.
- Consequências: os golden files O1–O4 ficam congelados (não são mais regeneráveis; a VMMLib continua no histórico
  do git, commit d189461). O otimizador de Lloyd (CVT) não foi migrado e fica para a 0.2, com a VMMLib do histórico
  como referência. O build da raiz não tem mais as opções VMM_BUILD_VMMLIB, VMM_INSTALL_VMMLIB e VMM_BUILD_PAPER.

## DEC-034 — Build padrão rápido
- Data: 2026-09-30
- Origem: instrução do João ("ao compilar a biblioteca a opção default sempre deverá ser a release, com os opcionais
  que acelerem a velocidade do código")
- Status: APROVADA (30/09, instrução do João)
- Decisão: SUBSTITUI a DEC-012 (2) no ponto das otimizações. Sem tipo de build informado, o CMake usa Release
  (também como CMAKE_DEFAULT_BUILD_TYPE em geradores multiconfiguração). Em Release e RelWithDebInfo, os alvos do
  vmm/ usam LTO (VMM_ENABLE_LTO, ON) e -march=native (VMM_ENABLE_NATIVE_ARCH, ON). -ffast-math continua proibido, porque muda resultados
  geométricos. Builds de cobertura e de sanitizadores ficam sem essas otimizações.
- Consequências: binários e pacotes para outras máquinas precisam de -DVMM_ENABLE_NATIVE_ARCH=OFF (uma CPU sem as
  instruções da máquina de build encerra com "illegal instruction"). O determinismo da DEC-020 continua valendo para
  o mesmo build; entre máquinas, mesma topologia e geometria dentro da tolerância. Com LTO, as bibliotecas estáticas
  levam só o código intermediário do GCC: quem as instala e liga precisa do mesmo compilador com LTO; para distribuir,
  use -DVMM_ENABLE_LTO=OFF. (Objetos "fat" foram tentados e quebraram a ligação dos testes com o GCC 15.)

## DEC-035 — Célula partida pelo domínio fica inteira
- Data: 2026-09-30
- Origem: P15a §3 (achado 5); resposta do João ("mantenha a célula inteira")
- Status: APROVADA (30/09, instrução do João)
- Decisão: quando o domínio não convexo corta uma célula de Voronoi em vários pedaços, a célula fica inteira, com
  todos os pedaços, ligada ao seu sítio. Um aviso é registrado no log e a célula é contada nas estatísticas da
  construção (BuildStats::fragmented_cells). Vale para 2D (comportamento atual do builder2d) e 3D.
- Justificativa: os invariantes continuam valendo; mover pedaços para células vizinhas mudaria a célula de Voronoi e
  a garantia de ortogonalidade das faces (DEC-032).
- Consequências: o 3D usa o mesmo contador (fragmented_cells) e o mesmo aviso; o usuário pode refinar os sítios onde
  elas aparecem.

## DEC-036 — Domínio 3D por superfícies trianguladas fechadas
- Data: 2026-09-30
- Origem: P15a; P15 §3–§4
- Status: APROVADA (30/09, João)
- Decisão: o domínio 3D (e cada região, na multirregião) é uma superfície triangulada fechada, orientada para fora,
  sem auto-interseção, possivelmente com vários componentes, com um rótulo de patch por triângulo. Formas analíticas
  (caixa, esfera, cilindro, extrusão de forma 2D) são poligonizadas para essa representação, como as curvas no 2D;
  o STL (P17) entra na mesma representação depois de reparo e validação.
- Justificativa: é a entrada natural do recorte exato do PMP/CGAL (P02 §6), validada no P15a; uma única
  representação para formas analíticas e STL.
- Consequências: P16 implementa TriangleSurface, as formas 3D e o validador; as superfícies curvas ficam aproximadas
  por triângulos, com o volume exato da superfície poligonizada como referência dos invariantes.

## DEC-037 — Sem contração FMA
- Data: 2026-09-30
- Origem: P16 (falha só em Release: ear clipping do prisma em L na escala 10⁻³)
- Status: APROVADA (30/09, João)
- Decisão: os alvos do vmm/ compilam com -ffp-contract=off (GCC e Clang). Complementa a DEC-034: o
  -march=native continua ligado, mas o compilador não funde a*b + c em FMA.
- Justificativa: em C++ o GCC contrai por padrão; com as instruções FMA do -march=native, testes geométricos em
  double que valem exatamente zero (pontos colineares, vértice sobre uma aresta) mudaram de sinal entre o build
  Debug e o Release, e a triangulação do contorno de uma extrusão falhou. É o mesmo tipo de efeito que levou a
  DEC-012 a proibir -ffast-math. As decisões de topologia exatas ficam no CGAL; as poucas em double (formas,
  geração de sítios) precisam dar o mesmo resultado em qualquer build.
- Consequências: resultados iguais entre Debug e Release na mesma máquina; perda de velocidade medida no P16.

## DEC-038 — Roteiro renumerado: 3D publicado como 0.2
- Data: 2026-09-30
- Origem: P19 §7; instrução do João ("pode publicar como 0.2")
- Status: APROVADA (30/09, instrução do João)
- Decisão: SUBSTITUI a DEC-025 no roteiro. A 0.2 publica as entregas 3D (b), (c) e (d) junto com o 2D da 0.1: uma
  região com domínio analítico, domínio STL e várias regiões com interfaces conformes. Os escritores MODFLOW 6,
  PFLOTRAN e TOUGH (P14a) passam para a 0.3. A 1.0 continua sendo o congelamento da API pública.
- Justificativa: o 3D está implementado e verificado (P16–P19), e os escritores não dependem dele.
- Consequências: pacote CMake 0.2.0 (`find_package(VoronoiMeshMaker 0.2)`, compatível na mesma versão menor); a
  0.1.0 não foi publicada separadamente.

## DEC-039 — Sem escritores MODFLOW 6, PFLOTRAN e TOUGH
- Data: 2026-09-30
- Origem: resposta do João ao início do P14a ("para que isso?"; opção "descartar")
- Status: APROVADA (30/09, instrução do João)
- Decisão: o P14a é cancelado. O VMM não terá escritores MODFLOW 6 (DISV, DISU), PFLOTRAN nem TOUGH, como já não tem
  OpenFOAM (DEC-030). Substitui a DEC-019 e a DEC-038 nesse ponto: a 0.3 deixa de ser a versão dos escritores.
- Justificativa: o solver do João lê a malha pela API e pelo formato nativo `.vmesh` (DEC-015, DEC-019); os
  escritores só serviriam a usuários desses programas e não mudam a malha.
- Consequências: a saída do VMM fica no formato nativo (persistência) e no VTK XML (visualização). Um escritor
  externo continua possível fora da biblioteca, a partir da API de adjacência e das métricas.
