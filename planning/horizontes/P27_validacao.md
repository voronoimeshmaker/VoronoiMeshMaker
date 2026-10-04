# P27 — Validação consolidada e desempenho

## Entradas e precondições

Depende de P26. Não adicionar ainda exemplos ao manual.
Ler [protocolo comum](02_protocolo.md), [requisitos](01_requisitos.md),
[ordem e dependências](00_sequencia.md) e os relatórios aprovados das etapas anteriores.
Não iniciar implementação antes de fechar as decisões necessárias.

## Tarefas

1. Executar matriz analítica e patológica aprovada em P21.
2. Verificar suporte geométrico das interfaces e classificação regional das células,
   além de área/volume, fechamento, orientação e adjacência.
3. Construir testes negativos: fronteira deslocada com mesma área, face duplicada,
   lacuna, rótulo regional incorreto e células atravessando horizonte devem ser detectados
   pelo conjunto de verificadores. Não atribuir ao check_invariants atual checagens que não faz.
4. Repetição determinística, permutações admitidas de entrada, escala e translação.
5. Testar geração 2D/3D antiga, partições, sítios, métricas, IO e reordenação.
6. Rodar check_requirements e firewall. Medir cobertura por arquivo novo/modificado;
   R25 é 90% linhas e 80% ramos; exceções precisam justificativa e revisão.
7. Executar Debug/Release e sanitizadores, e compiladores suportados disponíveis.
   Registrar indisponibilidade sem chamar a matriz incompleta de validada.
8. Medir três tamanhos com número de colunas e de camadas independentes: tempo, pico de
   memória, contagens de faces/vértices e custo por célula. Comparar com metas de P21 e
   casos leves anteriores; não presumir custo idêntico ao Voronoi 3D.
9. Classificar problemas por severidade e corrigir antes do manual.

## Entregáveis

relatorios/P27_validacao.md com comandos reproduzíveis, tabelas de resultados,
cobertura, desempenho e limitações.

## Critério de conclusão

Todos os casos obrigatórios passam com evidência da árvore ativa.
Não há falha aberta de separação regional, conformidade ou compatibilidade.
A ausência de ferramenta/compilador é registrada como validação pendente.

Ao terminar, registrar resultado e pendências e submeter à revisão prevista no protocolo.

## Testes do suporte fixo

Adicionar testes 2D/3D com diferentes distribuições de sítios usando a mesma partição:
as faces podem mudar, mas permanecem na reta/plano e cobrem a mesma fronteira finita.
Testar interfaces oblíquas, extremos e junções; invariância de área sozinha não basta.
Esses testes verificam a reconstrução com fronteiras fixas, não comprovam execução de CVT.
Testes de iterações CVT só podem ser declarados após existir um otimizador implementado (T01).
