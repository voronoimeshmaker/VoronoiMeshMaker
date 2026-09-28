```
REVISÃO INDEPENDENTE — P03 (Diagnóstico da VMMLib) do projeto VoronoiMeshMaker (VMM) — prompt v2

CONTEXTO
Você está revisando um entregável que NÃO escreveu. O VMM é uma biblioteca C++ que recebe um
domínio e um conjunto de sítios geradores e produz uma malha de Voronoi pronta para volumes finitos
(DEC-001). O P03 diagnostica o código atual (pasta VMMLib/) para decidir a base da nova arquitetura.

Repositório público: https://github.com/voronoimeshmaker/VoronoiMeshMaker (commit 7402178, branch main).
Os documentos de planning/ NÃO estão no repositório público: eles vêm anexados.
Ambiente oficial do projeto: WSL Ubuntu-26.04-Test (ver AGENTS.md). O P03 foi executado FORA dele
(numa cópia do GitHub, na nuvem, com CGAL 5.6.1); build, testes e cobertura estão marcados como
indicativos. O P02 analisou licenças no CGAL 6.2.x.

ARQUIVOS ANEXADOS
- pacote_revisao_P03.md: AGENTS.md, DECISIONS.md (DEC-001 a DEC-012), o relatório P03, o inventário
  CSV, o experimento de ordem de inserção e os trechos de código citados, com números de linha;
- P01_estado_da_arte.md e P02_licenca_backend.md.
Se puder, clone o commit 7402178 para conferir o código por conta própria; se não puder, use os
trechos do pacote e diga isso.

REGRAS
- Não altere nenhum arquivo. Não reabra decisões APROVADAS (DEC-001 a DEC-010). Você pode criticar as
  PROPOSTAS (DEC-011, DEC-012).
- Marque toda afirmação como FATO VERIFICADO (com comando ou arquivo:linha), INFERÊNCIA ou RECOMENDAÇÃO.
  Não afirme que algo compila, passa ou falha sem ter executado.
- Se não puder executar algo (build, testes, experimento), marque NÃO VERIFICADO e avalie apenas a
  plausibilidade metodológica.
- Critério para contagens: uma contagem é DIVERGENTE se a diferença não for explicada por uma
  diferença de método que você consiga descrever (por exemplo, incluir ou não enums e tipos aninhados).
  Declare sempre o método que usou.

TAREFAS
1. Conformidade com o prompt P03 (seção P03 do pacote): as tarefas 1 a 10 foram cumpridas? A ordem das
   seções 1 a 12 foi respeitada? O que não foi obtido está marcado como NÃO VERIFICADO?
   O P03 registra a versão do CGAL usada e discute a divergência em relação ao P02 (5.6.1 × 6.2.x)?
2. Verificação por amostragem (arquivo:linha ou comando):
   a) 115 tipos; 6 com ut_<Classe>.cpp; 44 nunca citados em testes;
   b) 32 headers de encaminhamento; 25 vazios; 51 não alcançados pelo build; 4 tipos mortos;
   c) 55 executáveis versionados; saída de exemplos e paper na árvore de fontes; testes no build;
   d) vazamento do CGAL (Core/type.h; DelaunayBuilder2D.hpp; ClippedVoronoiDiagram2D guardando a
      triangulação) e conformidade atual com a DEC-007 (headers públicos sem CGAL);
   e) VoronoiCellBuilder2D só aceita domínio convexo de um anel;
   f) VoronoiFaceConnectivity2D casa faces por proximidade geométrica;
   g) SiteFactory.hpp:932–934 usa std::uniform_real_distribution;
   h) falha em VerifyCGALHeaders.cmake:48; dependência implícita do GMock;
   i) experimento de ordem de inserção (plausibilidade do método e do resultado).
3. Consistência relatório × CSV: contagens, classificações (51/60/4), estados e justificativas.
   Aponte as linhas do CSV inconsistentes (por exemplo, classificação que contradiz outra coluna).
4. Solidez da recomendação (estratégia C, com a VMMLib como oráculo). Avalie A, B e C nestes critérios:
   esforço, risco de regressão, alinhamento com DEC-001/002/004/006/007/009, reversibilidade, dívida
   herdada, tempo até a entrega (a). Há um argumento forte a favor de A que o relatório ignorou?
5. Omissões, por categoria: robustez numérica e tolerâncias; degenerescências; acoplamentos; qualidade
   dos testes; dependências; portabilidade e flags de build; riscos para o P06.
6. Parecer sobre DEC-011 e DEC-012: aprovar, aprovar com mudanças ou rejeitar, com justificativa.

ENTREGÁVEL
planning/reviews/<nome-da-IA>_P03.md, em até 3 páginas:
1. ambiente em que você trabalhou e o que executou;
2. tabela de amostragem (item | CONFIRMADO / DIVERGENTE / NÃO VERIFICADO | método e evidência);
3. problemas por severidade (BLOQUEANTE / IMPORTANTE / MENOR), com localização, descrição e correção;
4. omissões, pelas categorias da tarefa 5;
5. parecer sobre DEC-011 e DEC-012;
6. veredito: APROVAR / APROVAR COM RESSALVAS / REFAZER.
Não repita o conteúdo do P03. Ao terminar, PARE.
```
