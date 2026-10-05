# CVT com partição fixa — execução autorizada

2026-10-04. Pedido: implementar CVT na biblioteca, incluindo volumes derivados
 dos horizontes, admitindo perda das colunas. Sem commit/push.

1. C01: núcleo Lloyd 2D/3D sobre partição imutável; sítios explícitos, energia
   geométrica uniforme, relaxação com busca de passo e convergência reportada.
2. C02: partição volumétrica a partir de LayeredMesh, preservando interfaces
   regionais e patches; descartar organização em colunas no resultado CVT.
3. C03: testes analíticos por operação, interfaces oblíquas, regiões não convexas,
   horizonte facetado/pinch-out, degenerações e execução determinística.
4. C04: integração C++/configuração, cobertura, sanitizadores e desempenho.
5. C05: Sphinx pt/en, exemplos após testes e relatório final de limitações.

Decisões: densidade uniforme (não propriedade física); sem inserir/remover sítios
implicitamente; regiões dos sítios fixas; a partição não é reconstruída de seus
sítios. Movimentos inválidos são reduzidos por busca de passo. Centroides fora
 de regiões não convexas não são aceitos automaticamente. Falta de passo válido
é estagnação, não convergência. Iterações preservam suporte e extensão das
interfaces por recorte contra a mesma partição e validação independente.

Estado: C01 em implementação. Não anunciar CVT concluído antes dos testes.
