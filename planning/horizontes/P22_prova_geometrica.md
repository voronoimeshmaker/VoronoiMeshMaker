# P22 — Prova de conceito da geração conforme

## Entradas e precondições

Depois do contrato P21. Protótipo isolado, sem alteração da API estável.
Ler [protocolo comum](02_protocolo.md), [requisitos](01_requisitos.md),
[ordem e dependências](00_sequencia.md) e os relatórios aprovados das etapas anteriores.
Não iniciar implementação antes de fechar as decisões necessárias.

## Tarefas

1. Criar prova em prototypes/P22a, apenas após aprovação do plano de iterações.
2. Extrudar uma base de duas ou mais células através de horizontes planos e inclinados.
3. Provar uma triangulação compartilhada e determinística de cada horizonte, sem fendas
   laterais ou diagonais incompatíveis entre células vizinhas.
4. Para superfícies não planas, comparar representações candidatas. Interpolação por célula
   independente pode mudar a fronteira: definir uma superfície de referência comum.
5. Calcular volumes por integração independente da rotina de métricas e verificar fechamento.
6. Testar um encontro de horizontes se Q03 o incluir. Se estiver fora do escopo,
   provar sua detecção e rejeição; não entregar células de volume zero.
7. Examinar base não convexa, com buracos e fragmentos conforme Q08. Não usar leques
   de triangulação sem demonstrar validade para esses polígonos.
8. Verificar compatibilidade de faces planas, orientação e métricas com Mesh<3>.
9. Medir custo por célula, faces por célula e memória em três tamanhos pequenos.

## Entregáveis

Protótipo reproduzível, resultados analíticos, falhas conhecidas e comparação de alternativas
em relatorios/P22_relatorio.md. Não promover código exploratório automaticamente.

## Critério de conclusão

Hipótese demonstrada para todos os casos do contrato; erros de entrada detectados.
Representação preserva suporte das interfaces, não só seus volumes/áreas.
Se a hipótese falhar, apresentar alternativas e revisar o contrato antes da arquitetura.

Ao terminar, registrar resultado e pendências e submeter à revisão prevista no protocolo.
