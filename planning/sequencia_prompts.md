# Sequência de prompts — conclusão do VMM

Versão 5 — 28/09/2026. A sequência pode ser alterada a qualquer momento (veja "Versionamento").

## Como usar

- Execute **um prompt por vez**. O próximo só começa depois que o anterior terminou **e** você aprovou o entregável e as decisões dele.
- Cada prompt é autocontido, porque pode rodar numa sessão nova.
- Todos os entregáveis ficam em `planning/`, na raiz do repositório, para que as sessões seguintes os leiam.
- **`planning/` é a única fonte de verdade** (DEC-013): requisitos, decisões, prompts e relatórios vivem lá, versionados no git.
- **Entrega arquivo a arquivo:** cada arquivo é entregue completo, um por vez. O João revisa, copia e faz o commit quando julgar oportuno (um commit por aprovação; tags não obrigatórias).
- **Ambiente declarado:** cada prompt diz em que ambiente roda (WSL oficial ou outro) e registra as versões usadas (DEC-012).
- **Comandos para o João:** só se pedem quando o resultado pode mudar uma decisão.

## Registro de decisões (`planning/DECISIONS.md`)

Cada decisão ocupa uma entrada, com numeração que nunca é reutilizada:

```
## DEC-001 — <título curto>
- Data: AAAA-MM-DD
- Origem: P0x (entregável que a propôs)
- Status: PROPOSTA | APROVADA | REJEITADA | SUBSTITUÍDA por DEC-nnn
- Decisão: <uma ou duas frases>
- Justificativa: <por quê; alternativas descartadas>
- Consequências: <o que muda nos prompts e requisitos>
```

- **Proposta:** o agente acrescenta entradas com status PROPOSTA.
- **Aprovação:** só você muda uma entrada para APROVADA ou REJEITADA.
- **Obrigatoriedade:** uma decisão APROVADA só é reaberta por uma nova entrada que a SUBSTITUI, nunca por edição.

## Protocolo dos prompts de código (P05a, P07 a P13 e toda a Fase 3)

1. **Iterações pequenas.** Cada prompt de código roda em iterações `Pxx.1`, `Pxx.2`…
   - Cada iteração trata de um módulo, ou de um grupo pequeno de classes relacionadas.
   - O diff deve ser pequeno o bastante para você revisar inteiro; como referência, cerca de 500 linhas de código de produção.
   - A primeira iteração de cada prompt começa com um plano de iterações. Esse plano não tem código e precisa da sua aprovação.
2. **Relatório por iteração:** `planning/Pxx_k_relatorio.md`, com arquivos alterados, comandos executados, evidência dos testes e cobertura.
3. **Revisão pelo João.** Você revisa cada iteração. O prompt **REV** fica disponível como apoio opcional, numa sessão nova.
4. **Sua aprovação fecha a iteração.** Só então a próxima começa.
5. **Limite de tentativas.** Se uma iteração falhar três vezes (build, testes ou revisão), o agente para, descreve o impasse e pede decisão.
6. **Movimentação separada de lógica.** Iterações que só movem ou renomeiam arquivos não mudam lógica, e vice-versa.
7. **Testes por classe na migração.** Cada módulo migrado leva os seus testes reorganizados na árvore espelhada (R1/R2).
8. **Benchmark leve.** A partir do P07, cada entrega mede tempo e memória de 2 ou 3 casos fixos e compara com a anterior.

## Versionamento desta sequência

- **Nova versão:** toda mudança de plano gera uma nova versão deste arquivo.
- **Registro:** cada versão tem uma linha no "Histórico de versões" (no fim do arquivo), com o motivo e a decisão DEC que a originou.
- **Proposta de mudança:** os prompts que podem alterar o plano (P01, P03, P05, P14) terminam propondo, se for o caso, uma nova versão da sequência.

## Regras comuns (vão implícitas em todos os prompts)

Cole este bloco no início de cada prompt:

```
REGRAS COMUNS
- Repositório: /home/jflavio/Programas/VMM, na distribuição WSL Ubuntu-26.04-Test (siga o AGENTS.md).
  Remoto: https://github.com/voronoimeshmaker/VoronoiMeshMaker
- Declare no início do entregável o ambiente em que rodou e as versões usadas (compilador, CMake, CGAL,
  Boost, GoogleTest e as demais que o prompt usar). Só versões estáveis (DEC-012). Padrão C++23 (DEC-014).
- Não faça commit nem push. Preserve as alterações não commitadas.
- Leia antes de começar: AGENTS.md, planning/00_requisitos_iniciais.md, planning/DECISIONS.md e
  todos os planning/P*.md já concluídos. As decisões APROVADAS são obrigatórias; não as reabra.
- Novas decisões: acrescente-as ao planning/DECISIONS.md com status PROPOSTA (numeração
  sequencial). Nunca altere o status de uma entrada existente.
- Nada de afirmar que um teste passou, que a cobertura foi atingida ou que uma dependência funciona
  sem evidência do ambiente ativo. Afirmações sobre software de terceiros (licenças, APIs,
  limitações) precisam de fonte citada (documentação oficial ou código-fonte), com link.
- Distinga sempre: FATO VERIFICADO / INFERÊNCIA / RECOMENDAÇÃO.
- Entregáveis em português; código, identificadores, comentários técnicos e Doxygen em inglês.
- Ao terminar: (1) resumo de até 15 linhas; (2) lista numerada de DECISÕES PENDENTES para o João,
  cada uma com opções e a sua recomendação; (3) PARE. Não inicie a etapa seguinte.
```

## REV — Revisão independente (opcional; apoio à revisão do João)

```
[REGRAS COMUNS]

OBJETIVO
Revisar criticamente a iteração <Pxx.k>, que você NÃO escreveu, como apoio à revisão do João.

ENTRADAS
planning/Pxx_k_relatorio.md, o diff da iteração (git diff contra o último estado aprovado),
planning/P05_BASELINE.md, planning/DECISIONS.md.

TAREFAS
1. Conformidade: a iteração faz o que o plano de iterações previa, e nada além disso?
   Viola algum requisito, alguma decisão APROVADA, o AGENTS.md ou o R3 (virtual, herança,
   enum de despacho fechado)?
2. Correção: erros lógicos e geométricos, casos de borda sem tratamento, robustez numérica,
   determinismo.
3. Testes: cada classe nova ou modificada tem GTest atualizado (R1/R2)? Os testes verificam
   comportamento ou só executam código? Faltam testes de propriedade ou casos patológicos?
4. Evidência: rode você mesmo o build e os testes no WSL; compare com o relatório.
5. Legibilidade da API e da documentação Doxygen.

ENTREGÁVEL
planning/Pxx_k_revisao.md: problemas numerados por severidade (BLOQUEANTE / IMPORTANTE /
MENOR), cada um com arquivo:linha, cenário de falha e correção sugerida; veredito
APROVAR / APROVAR COM RESSALVAS / REFAZER.

RESTRIÇÃO
Não altere código. Só aponte.
```

---

# FASE 1 — Decisões fundacionais

Não entra código de produção nesta fase.

## P01 — Estado da arte e nicho

```
[REGRAS COMUNS]

OBJETIVO
Determinar se existe, e qual é, o nicho do VMM diante das ferramentas existentes, e redigir
uma "statement of need" no formato do JOSS.

TAREFAS
1. Levantar e analisar, com fontes, no mínimo: JIGSAW (Engwirda), VoroCrust (Sandia), Geogram
   (Voronoi restrito/CVT), Voro++, CGAL (o que oferece e o que não oferece para Voronoi recortado 2D/3D),
   OpenFOAM polyDualMesh, ferramentas de malha do MPAS, TetGen (saída Voronoi), Qhull,
   PolyMesher. Acrescentar outras que encontrar.
2. Montar uma matriz ferramenta × critério. Critérios: 2D/3D; Voronoi recortado por domínio
   não convexo; multirregião com interfaces conformes; arestas vivas; domínio STL; pesos/potência;
   CVT; métricas de volumes finitos prontas (área/volume, vetor área, distância entre geradores,
   não-ortogonalidade); rótulos de patch; formatos de saída; API (linguagem, estilo);
   licença; atividade do projeto (último release, commits); documentação.
3. Identificar lacunas reais em relação aos requisitos R1–R7.
4. Propor de 1 a 3 nichos possíveis, com prós e contras, e recomendar um.
5. Redigir o rascunho da "statement of need" (até 1 página) para o nicho recomendado.

ENTREGÁVEL
planning/P01_estado_da_arte.md

CRITÉRIO DE CONCLUSÃO
Todas as ferramentas analisadas com fonte; matriz completa; recomendação justificada.
```

## P02 — Licença e backend geométrico

```
[REGRAS COMUNS]

OBJETIVO
Decidir a licença do VMM e a estratégia de backend geométrico, com base em fatos verificados.

TAREFAS
1. Verificar, na documentação oficial do CGAL (versão instalada no WSL e a versão atual),
   a licença de cada pacote usado ou candidato: Kernel, Triangulation_2/3, Delaunay,
   Regular_triangulation, Periodic_2/3, Polygon_2, Boolean_set_operations_2, Convex_hull_3
   (halfspace_intersection_3), Polygon_mesh_processing, AABB_tree, Mesh_3, Nef_3.
2. Levantar as licenças de alternativas: Geogram, Voro++, Clipper2, outras relevantes.
3. Avaliar os cenários:
   (a) VMM GPL v3+ com CGAL;
   (b) VMM permissivo (BSD/MIT/Apache) com backend não GPL;
   (c) núcleo permissivo + backend CGAL opcional (plugin GPL);
   (d) licença comercial do CGAL.
   Para cada cenário: impacto em usuários acadêmicos e industriais, no JOSS e no esforço técnico.
4. Verificar as afirmações feitas anteriormente sem fonte:
   - as triangulações periódicas do CGAL exigem domínio quadrado ou cúbico?
   - o sphinx-gallery aceita exemplos em C++ (renderizar sem executar)?
   - PMP::extrude_mesh existe? Qual versão?
5. Propor como o backend fica isolado (concepts/policies, sem herança — R3) para que a troca
   de backend seja possível.
6. Comparação técnica CGAL × Geogram para Voronoi recortado 3D (DEC-002): funções disponíveis,
   recorte por superfície não convexa, suporte a multirregião e interfaces, robustez (predicados
   exatos), desempenho relatado, paralelismo, integração via CMake, licença.
7. Verificar se o código-fonte do VoroCrust está obtenível (repositório, download, termos) e
   qual é a sua forma de uso (biblioteca, executável, API).

ENTREGÁVEL
planning/P02_licenca_backend.md

CRITÉRIO DE CONCLUSÃO
Tabela de licenças com links; cenários comparados; comparação CGAL × Geogram; situação do
VoroCrust; recomendação; afirmações anteriores confirmadas ou refutadas.
```

## P03 — Diagnóstico da VMMLib (com evidência)

```
[REGRAS COMUNS]

OBJETIVO
Saber exatamente o que a VMMLib tem, o que funciona e o que pode ser aproveitado,
para decidir a base de código.

TAREFAS
1. Configurar, compilar e rodar os testes (ctest) no WSL indicado. Registrar os comandos, a saída
   resumida, as falhas e os avisos.
2. Medir a cobertura (gcovr) por arquivo e por classe.
3. Inventariar as classes: nome, header, responsabilidade, se tem teste próprio, cobertura, e se
   viola o R3 ou o AGENTS.md (virtual, herança, enum de despacho fechado, ordem de includes).
4. Listar os headers duplicados (a mesma classe na raiz e em subpastas) e o código morto.
5. Avaliar a qualidade dos módulos geométricos (clipping, Delaunay, Voronoi, CVT): robustez,
   casos patológicos cobertos, determinismo.
6. Classificar cada módulo: APROVEITAR / REFATORAR / DESCARTAR, com justificativa.
7. Comparar três caminhos: evoluir a VMMLib incrementalmente; migrar para a estrutura
   VoronoiGridMaker; reescrever. Recomendar um, com uma estimativa relativa de esforço.

ENTREGÁVEL
planning/P03_diagnostico_vmmlib.md (+ planning/P03_inventario.csv)

CRITÉRIO DE CONCLUSÃO
Build e testes executados (ou a falha documentada); inventário completo; recomendação.
Não alterar nenhum código nesta etapa.
```

## P04 — Problema-âncora, consumidor da malha e metas

```
[REGRAS COMUNS]

OBJETIVO
Tirar o projeto do abstrato: definir os casos reais que guiam as prioridades.

TAREFAS
1. Propor 2 ou 3 problemas-âncora de escoamento ambiental com regiões fixas (água, sólido, gás),
   em ordem crescente de dificuldade. Exemplo inicial: trecho de rio 2D com margem de solo e
   leito, 2 a 3 regiões. Para cada um: geometria, regiões, patches, ordem de grandeza do
   número de células, dados de entrada.
2. Levantar os solvers de volumes finitos que o público-alvo usa e os formatos que eles leem
   (OpenFOAM polyMesh, VTK, CGNS, Gmsh, formato próprio). Recomendar os formatos da
   primeira versão.
3. Propor metas mensuráveis: tempo de geração por número de células (2D e 3D), memória,
   limites de qualidade, determinismo.
4. Preparar um QUESTIONÁRIO curto para o João validar os casos, os formatos e as metas.

ENTREGÁVEL
planning/P04_ancora_metas.md

CRITÉRIO DE CONCLUSÃO
Casos descritos o suficiente para virar testes de integração; questionário pronto.
```

## P05 — Linha de base: requisitos, diretrizes e roteiro

```
[REGRAS COMUNS]

OBJETIVO
Consolidar as decisões da Fase 1 numa linha de base aprovável.

ENTRADAS
P01 a P04 e as respostas do João às decisões pendentes de cada um.

TAREFAS
1. Reescrever os requisitos (R1…Rn), numerados, verificáveis, cada um com justificativa e
   critério de verificação.
2. Reescrever docs/architecture/project_guidelines.tex:
   - princípios como REGRAS VERIFICÁVEIS (não como lista de dogmas);
   - política de enums (dado × despacho), de exceções, de padrão C++ (20 × 23), de nome e
     namespace, de dependências;
   - política de testes: R1/R2 + testes de propriedade (volumes somam o domínio, fechamento
     por célula, conformidade, ortogonalidade) + golden files + casos patológicos;
   - portabilidade (CI pública) sem conflito com o AGENTS.md local.
3. Roteiro em entregas verticais publicáveis, cada uma com escopo, critério de aceitação e
   versão:
   (a) 2D multirregião conforme + métricas de volumes finitos + exportação;
   (b) 3D com uma região e domínio analítico;
   (c) 3D com domínio STL;
   (d) 3D multirregião.
4. Lista de riscos, com mitigação.
5. Propor ajustes aos prompts P06 em diante desta sequência, se necessário.

ENTREGÁVEL
planning/P05_BASELINE.md + project_guidelines.tex atualizado

CRITÉRIO DE CONCLUSÃO
O João aprova a linha de base. A partir daqui, mudanças de escopo exigem revisão explícita dela.
```

---

## P05a — Provas de conceito antes da arquitetura

```
[REGRAS COMUNS]

OBJETIVO
Reduzir o risco da arquitetura (P06) com provas de conceito curtas e descartáveis, em 2D e 3D.

TAREFAS
1. Prova 2D: domínio não convexo com um buraco e duas regiões. Células de Voronoi recortadas pelo
   domínio e pela interface, com o CGAL escondido atrás de uma função interna. Verificar os
   invariantes da DEC-011: soma das áreas = área do domínio; fechamento de cada célula;
   consistência owner/neighbour; interface conforme.
2. Prova 3D mínima: uma caixa, uma região, células por interseção de semiespaços. Verificar soma dos
   volumes e fechamento. Serve para testar o modelo de dados genérico em dimensão; NÃO substitui o
   P15a.
3. Firewall de compilação (DEC-007): um header público sem CGAL e um .cpp com CGAL; mostrar que o
   header compila sem o CGAL no caminho de includes.
4. Estrutura de adjacência (DEC-015): expor os vizinhos de cada célula em forma compacta (tipo CSR)
   e mostrar que dela se obtém o padrão de uma matriz esparsa sem reconstruir a geometria.
5. Relatório: o que funcionou, o que falhou, o que muda no P06.

LIMITES
Código em prototypes/P05a/, fora da biblioteca e sem compromisso de API. Prazo: o João o fixa ao
aprovar o plano de iterações. Nada daqui entra no código de produção sem passar pelo P06.

ENTREGÁVEL
planning/P05a_provas_conceito.md + prototypes/P05a/

CRITÉRIO DE CONCLUSÃO
Invariantes verificados (ou falha documentada) nas duas provas; recomendações concretas para o P06.
```

---

# FASE 2 — Entrega (a): 2D multirregião

Os prompts desta fase seguem a DEC-011: estrutura nova, com migração seletiva da VMMLib, invariantes geométricos como critério principal e a VMMLib como oráculo.

## P06 — Arquitetura da entrega (a)

```
[REGRAS COMUNS]

OBJETIVO
Derivar a arquitetura mínima da entrega (a), a partir da linha de base e da VMMLib existente.

TAREFAS
1. Modelo de dados genérico em dimensão (Dim = 2|3), orientado a dados (DOD):
   - Storage, IDs fortes, conectividade CSR;
   - Medium e Region (registro aberto), rótulos de patch e interfaces;
   - campos previstos para o futuro (peso do sítio; offset periódico por face), com custo zero
     quando não usados.
2. Concepts e policies principais (domínio, fonte de sítios, backend, clipping, reordenação,
   escrita), sem herança (R3). Factories de compilação e registro de execução.
3. Árvore de diretórios final, restrita ao que a entrega (a) usa. Mapa de migração
   VMMLib → nova árvore (mover / refatorar / descartar), por arquivo.
4. Decidir o destino do esqueleto VoronoiGridMaker/ (podar, arquivar ou remover).
5. Diagramas: pipeline e dependências entre módulos (sem ciclos).
6. Invariantes geométricos, casos do oráculo e critério de retirada da VMMLib (DEC-011).
7. Estruturas de adjacência e views para a aplicação consumidora (DEC-015) e API do subsistema de
   erros (DEC-016).
8. Incorporar o que o P05a ensinou.

ENTREGÁVEL
planning/P06_arquitetura_a.md (+ diagramas)

CRITÉRIO DE CONCLUSÃO
Todo requisito da entrega (a) tem um módulo responsável; mapa de migração completo.
Sem código.
```

## P07 — Infraestrutura de build, testes e CI

```
[REGRAS COMUNS]

OBJETIVO
Deixar o projeto compilável, instalável e testável fora da máquina do João.

TAREFAS
1. CMake: target exportado, install(), find_package(<Nome>), opções de exemplos, testes e
   documentação; dependências conforme a P02.
2. Árvore de tests/ espelhando a biblioteca (R2), com a descoberta automática existente
   (ut_<Pasta>.cpp).
3. Helpers de teste: gerador de conjuntos de sítios aleatórios com semente; verificadores de
   invariantes (propriedade); utilitário de comparação com golden files e tolerância.
4. GitHub Actions: Linux gcc e clang, Debug/Release, sanitizers, cobertura (relatório).
   macOS e Windows opcionais.
5. Validar localmente, no WSL, e documentar como reproduzir a CI.
6. Executar a DEC-012: higiene do repositório, flags de build, verificação portável dos headers do
   CGAL, teste "nenhum header público inclui o CGAL", cobertura de linhas, ramos e funções, versão
   mínima das dependências no CMake e registro das versões usadas.
7. Padrão C++23 (DEC-014).
8. Gerar os golden files do oráculo (DEC-011) num build sem -ffast-math e sem NATIVE_ARCH, com
   compilador, flags e versões no cabeçalho de cada arquivo.
9. Benchmark leve: tempo e memória de 2 ou 3 casos fixos, como linha de base.

ENTREGÁVEL
Alterações de build/CI + planning/P07_infra.md (comandos, evidências)

CRITÉRIO DE CONCLUSÃO
Build limpo e testes existentes rodando no WSL, com evidência; workflow de CI válido.
```

## P08 — Núcleo: Core, Error, Geometry

```
[REGRAS COMUNS]

OBJETIVO
Migrar e refatorar o núcleo conforme o mapa da P06.

TAREFAS
1. Core: tipos, IDs fortes, constantes, concepts base.
2. Error: implementar o subsistema de erros da DEC-016 (std::expected para falhas recuperáveis,
   exceção própria sem herança; IErrorLogger, ThreadLocalBufferLogger e VMMException não migram
   como estão); rever os enums de despacho. Mensagens pt/en por catálogo.
3. Geometry: primitivas e algoritmos básicos genéricos em dimensão onde fizer sentido.
4. GTest por classe (R1), na árvore espelhada (R2), mais testes de propriedade onde couber.
5. Remover duplicatas e código morto identificados na P03.

ENTREGÁVEL
Código + testes + planning/P08_relatorio.md (cobertura por classe, evidência de execução)

CRITÉRIO DE CONCLUSÃO
Todos os testes passando no WSL com evidência; cobertura dentro da política da P05.
```

## P09 — Domínio 2D multirregião

```
[REGRAS COMUNS]

OBJETIVO
Implementar a definição do domínio 2D por meios e regiões (R5).

TAREFAS
1. Registro aberto de Medium e Region; número variável de regiões por meio.
2. Composição por precedência (diferença na ordem declarada), sobre as formas 2D existentes
   (Rectangle, Polygon, Ellipse, Ring etc.).
3. Rótulos de patch por trecho do contorno externo.
4. Validação: sobreposição ambígua, vazios não intencionais, componentes desconexas por
   região, orientação.
5. GTest por classe + testes de propriedade (a soma das áreas das regiões é igual à área
   da união).

ENTREGÁVEL
Código + testes + planning/P09_relatorio.md

CRITÉRIO DE CONCLUSÃO
Testes passando com evidência; casos patológicos incluídos.
```

## P10 — Sítios, Voronoi e clipping 2D multirregião conforme

```
[REGRAS COMUNS]

OBJETIVO
Gerar a malha 2D multirregião com interfaces conformes (R6).

TAREFAS
1. Fontes de sítios por região (densidades distintas), determinísticas por semente
   (distribuição própria e portável, não std::uniform_*_distribution).
2. Espelhamento de sítios em relação às interfaces e ao contorno (policy padrão).
3. Clipping multirregião: cada célula pertence a exatamente uma região; policy de corte como
   garantia de conformidade onde o espelhamento não basta.
4. Tratamento de junções triplas (três regiões num ponto) e de ângulos agudos.
5. Testes: GTest por classe + testes de propriedade (conformidade, soma das áreas, fechamento,
   ortogonalidade nas interfaces) + casos patológicos (sítios quase cocirculares, colados à
   interface, regiões finas).

ENTREGÁVEL
Código + testes + planning/P10_relatorio.md

CRITÉRIO DE CONCLUSÃO
Invariantes verificados em ≥ 1000 configurações aleatórias com semente, com evidência.
```

## P11 — Topologia, classificação, métricas de volumes finitos, reordenação

```
[REGRAS COMUNS]

OBJETIVO
Entregar a malha pronta para um solver de volumes finitos.

TAREFAS
1. Topologia CSR (célula→faces, face→células, face→vértices), sem supor um par (i, j) único.
2. Classificação: células e faces internas, de contorno (por patch) e de interface
   (por par de regiões). Views por meio, região, interface e patch.
3. Métricas: área, centroide, vetor área da face, distância entre geradores, ponto de
   interseção face–segmento, não-ortogonalidade, skewness. Relatório de qualidade com limites
   configuráveis.
4. Reordenação (RCM e outras policies), com permutação e permutação inversa explícitas.
5. GTest por classe + testes de propriedade.

ENTREGÁVEL
Código + testes + planning/P11_relatorio.md

CRITÉRIO DE CONCLUSÃO
Testes passando com evidência; métricas validadas contra casos analíticos (malha cartesiana,
hexagonal).
```

## P12 — IO e integração com os problemas-âncora

```
[REGRAS COMUNS]

OBJETIVO
Exportar a malha e validar a entrega (a) de ponta a ponta.

TAREFAS
1. Escritores: VTK (.vtu/.vtp) com regiões, patches e métricas como campos; os formatos
   de solver escolhidos na P04 (por exemplo, OpenFOAM polyMesh com patches e zonas por região).
2. Separação entre visualização e persistência (conforme as diretrizes).
3. Testes de integração com os problemas-âncora 2D da P04; golden files.
4. Medir as metas de desempenho da P04 e registrar os resultados.
5. Se houver solver disponível (por exemplo, OpenFOAM checkMesh), rodar a verificação da malha
   exportada.

ENTREGÁVEL
Código + testes + planning/P12_relatorio.md (inclui tabela de desempenho)

CRITÉRIO DE CONCLUSÃO
Problemas-âncora gerados, exportados e verificados, com evidência.
```

## P13 — Documentação da entrega (a)

```
[REGRAS COMUNS]

OBJETIVO
Documentação pública no modelo PETSc + galeria no estilo nestle, com a paleta "Estuário".

TAREFAS
1. Sphinx + tema PyData (docs_sphinx/ existente) + Breathe/Doxygen.
2. Referência da API com seções fixas (Synopsis, Parameters, Notes, Level, See Also,
   Location, Examples); a lista "Examples" é gerada varrendo os ex_*.cpp. Formato (por função ou
   por classe) conforme a P05.
3. Galeria: cada examples/<nome>/ex_<nome>.cpp com um bloco de cabeçalho (título e descrição);
   o build roda os exemplos, e o PyVista (offscreen) gera figuras e miniaturas; páginas com
   texto, figura, código e download.
4. Paleta Estuário (planning/00_requisitos_iniciais.md) no site e nas figuras.
5. i18n pt/en (sphinx-intl). Guia do usuário e quick start.
6. Workflow de CI: build da documentação → GitHub Pages. Um exemplo que falha quebra o build.
7. Gate do AGENTS.md: só entram na galeria exemplos de classes cujos testes de classe e de
   integração passam.

ENTREGÁVEL
docs + workflow + planning/P13_relatorio.md

CRITÉRIO DE CONCLUSÃO
Site gerado localmente com evidência; todos os exemplos executados e ilustrados.
```

## P14 — Release 0.1

```
[REGRAS COMUNS]

OBJETIVO
Preparar a primeira versão pública (entrega (a)).

TAREFAS
1. Revisão crítica completa (código, API, testes, documentação) contra a linha de base (P05).
   Listar os problemas por severidade.
2. Congelar a API pública da 0.1; README, CHANGELOG, CONTRIBUTING, cabeçalhos de licença.
3. Checklist do JOSS (statement of need, comparação, testes, documentação, instalação).
4. Lista do que fica para a 0.2.
5. Revisar esta sequência de prompts para a Fase 3.

ENTREGÁVEL
planning/P14_release_0_1.md

CRITÉRIO DE CONCLUSÃO
O João aprova a publicação.
```

---

# FASE 3 — 3D (esboço)

Os prompts completos desta fase serão escritos depois da P14, com o que for aprendido na entrega (a).

- **P15a — Prova de conceito do recorte 3D (DEC-003, DEC-010):** implementação curta e descartável, com o **CGAL**. O VoroCrust (executável) serve só de referência externa, quando possível. Valida a hipótese central: construir as células de Voronoi convexas por interseção de semiespaços e recortar apenas as células necessárias contra o domínio 3D. Casos mínimos:
  - domínio convexo;
  - domínio não convexo;
  - arestas vivas;
  - células de fronteira complexas;
  - múltiplos componentes;
  - casos quase degenerados;
  - determinismo quanto à ordem de inserção dos sítios;
  - teste de estresse do recorte exato.

  Não entra no código de produção. A arquitetura 3D definitiva só é decidida depois deste resultado.

  - **Critérios de sucesso:** todos os casos com os invariantes da DEC-011 dentro da tolerância; mesma topologia para ordens de inserção diferentes; tempo por célula compatível com as metas do P04.
  - **Prazo:** fixado pelo João ao aprovar o plano de iterações.
  - **Contingência:** se a hipótese falhar, o relatório compara as alternativas (recorte exato de todas as células de fronteira com outros pacotes do CGAL; restringir a primeira entrega 3D a domínios analíticos; outra estratégia que surgir) e propõe uma DEC.
- **P15 — Arquitetura 3D:** reuso do núcleo genérico; backend 3D conforme a P02.
- **P16 — 3D, uma região, domínio analítico:** Delaunay 3D, células por semiespaços, detecção das células de contorno, clipping, métricas. Entrega (b).
- **P17 — Domínio STL/CAD:** leitura, reparo e validação da superfície, rótulos de patch, clipping contra superfície triangulada. Entrega (c).
- **P18 — 3D multirregião conforme:** espelhamento e corte, junções triplas, arestas vivas. Entrega (d). Revisitar o VoroCrust e a literatura antes de implementar.
- **P19 — Documentação e release 3D.**

---

# Histórico de versões

| Versão | Data | Mudança | Origem |
|---|---|---|---|
| 1 | 2026-09-28 | Sequência inicial: P01–P14 completos, esboço da Fase 3 | discussão de requisitos |
| 2 | 2026-09-28 | Registro de decisões (DECISIONS.md); protocolo de iterações para os prompts de código; prompt REV de revisão independente; versionamento | pedido do João |
| 3 | 2026-09-28 | P02 ganha a comparação CGAL × Geogram e a verificação do VoroCrust; P15a (prova de conceito 3D) antes da arquitetura 3D | DEC-002, DEC-003 |
| 4 | 2026-09-28 | P15a com CGAL, VoroCrust só como referência, bateria de testes da hipótese do backend 3D | DEC-010 |
| 5 | 2026-09-28 | Revisões pelo João (REV opcional); planning/ como fonte única; ambiente e versões declarados; entrega arquivo a arquivo; P05a (provas de conceito 2D e 3D); P15a com critérios, prazo e contingência; limite de tentativas; iterações de movimentação separadas; DEC-012 no P07; golden files e benchmark a partir do P07; Fase 2 segue a DEC-011 | DEC-011 a DEC-016 |
