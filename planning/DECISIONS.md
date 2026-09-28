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
