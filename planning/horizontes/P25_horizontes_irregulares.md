# P25 — Superfícies irregulares e topologias aprovadas

## Entradas e precondições

Depende de P24. Implementar somente casos aceitos em P21.
Ler [protocolo comum](02_protocolo.md), [requisitos](01_requisitos.md),
[ordem e dependências](00_sequencia.md) e os relatórios aprovados das etapas anteriores.
Não iniciar implementação antes de fechar as decisões necessárias.

## Tarefas

1. Implementar entrada e amostragem/interpolação de horizontes conforme Q01/Q02.
2. Construir a representação de referência uma vez e compartilhá-la entre os dois domínios.
3. Implementar faces planas ou trianguladas conforme Q05; não tratar quadrilátero empenado
   como plano na rotina de volume ou no escritor.
4. Garantir que novos pontos em interfaces sejam obtidos no suporte fixado, inclusive nas
   junções com fronteiras laterais. Verificar compatibilidade de subdivisões vizinhas.
5. Implementar intervalos de espessura variável e distribuição vertical aprovada.
6. Se Q03 autorizar desaparecimento, construir fechamento e adjacência na linha de encontro,
   eliminando células de espessura nula por regra explícita, preservando os domínios restantes.
   Não confundir encontro permitido com inversão/cruzamento indevido.
7. Se Q03 não autorizar, retornar diagnóstico claro antes da montagem.
8. Cobrir topologias de base acordadas em Q08; rejeitar explicitamente as demais.
9. Testar escalas e translações, horizontes quase coincidentes, inclinações fortes,
   regiões finas e funções/valores não finitos. Usar referências analíticas e geométricas.

## Entregáveis

Código e GTest por classe; relatório com comparação de suporte geométrico antes/depois,
medidas por região e limites da aproximação inicial.

## Critério de conclusão

Fronteiras conformes e preservadas no sentido de Q09; volumes positivos e fechamento.
Nenhum arredondamento remove silenciosamente região ou muda a associação das células.

Ao terminar, registrar resultado e pendências e submeter à revisão prevista no protocolo.
