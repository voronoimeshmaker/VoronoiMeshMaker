# P24 — Geração inicial com horizontes planos

## Entradas e precondições

Depende de P23 e de seu plano de iterações.
Ler [protocolo comum](02_protocolo.md), [requisitos](01_requisitos.md),
[ordem e dependências](00_sequencia.md) e os relatórios aprovados das etapas anteriores.
Não iniciar implementação antes de fechar as decisões necessárias.

## Tarefas

1. Implementar objetos de entrada e sua validação: nomes/IDs, ordem vertical, dimensões,
   valores finitos, espessuras e limites de capacidade.
2. Implementar subdivisão por intervalo conforme Q04, sem transformar cada subdivisão
   em região nova.
3. Construir vértices compartilhados e faces laterais e horizontais, preservando patches
   laterais da base e identificações superior/inferior.
4. Gerar conectividade owner/neighbour, orientação e ordenação triangular superior,
   com contornos agrupados por patch.
5. Manter coluna/camada/região identificáveis, incluindo regiões laterais da base.
6. Devolver Result com contexto e entidade em falhas; nunca entregar malha parcial.
7. Cobrir funções públicas de cada classe por GTest antes da integração.
8. Verificar volumes analíticos área da base × espessura, fechamento e interfaces.
9. Exercitar uma região com várias camadas e regiões distintas compartilhando horizontes.

## Entregáveis

Código, testes por classe, relatório por iteração e referência geométrica independente
para os casos planos.

## Critério de conclusão

Casos planos corretos, determinísticos e conformes; nenhum domínio atravessado.
Entradas inválidas rejeitadas; API antiga compila e seus testes pertinentes continuam passando.

Ao terminar, registrar resultado e pendências e submeter à revisão prevista no protocolo.
