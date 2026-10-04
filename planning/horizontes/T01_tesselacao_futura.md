# T01 — Contrato para tesselação posterior, fora da primeira entrega

Data: 2026-10-03. Estado: planejamento de extensão futura; implementação não autorizada.
Não é etapa obrigatória de P21–P29.

## Situação atual

O VMM gera Voronoi por região, recorta pelo domínio e compatibiliza interfaces por
refinamento comum. Isso não constitui uma API geral para retesselar uma Mesh3D pronta.
A reordenação existente muda numeração, não é retesselação.

O João admite que uma transformação posterior modifique as colunas; exige que a fronteira
entre domínios permaneça. Esse requisito vale para todas as interfaces regionais.

## Decisão futura necessária

“Tesselação” ainda não especifica uma operação: subdividir poliedros, tetraedrizar,
regerar Voronoi com novos sítios ou remalhar são algoritmos e contratos diferentes.
Não escolher nenhum deles sem novo pedido e definição do objetivo.
Definir também se a operação preserva contornos externos, camadas internas não obrigatórias
e quais relações de proveniência devem existir.

## Restrições obrigatórias

1. Usar interfaces como restrições geométricas, nunca apenas rótulos pós-processados.
2. Novas faces podem subdividir a fronteira aceita; sua união deve conservar o mesmo suporte,
   orientação regional e curvas de junção.
3. Nenhuma célula pode atravessar uma interface. Cada célula resultante tem região válida.
4. Ambos os lados usam faces compatíveis, sem faces pendentes, lacunas ou sobreposições.
5. Conservar volumes por região e fechamento de cada célula; testar geometricamente a
   fronteira, porque volumes e áreas totais sozinhos não bastam.
6. Produzir nova malha imutável e atualizar proveniência. Não devolver índices de coluna,
   camada ou vizinhança vertical obsoletos.
7. Não interpolar propriedades físicas nem resolver evolução temporal.

## Roteiro futuro, após autorização específica

- T01.1: definir operação e casos de aceitação.
- T01.2: prova isolada, incluindo interface oblíqua e junção de regiões.
- T01.3: arquitetura que preserve restrições e proveniência.
- T01.4: implementação e GTest por classe.
- T01.5: integração, invariantes e IO.
- T01.6: cobertura, desempenho e documentação após os testes.

P23 deve manter os dados de interface acessíveis para essa evolução, sem antecipar sua
implementação ou impor que toda malha transformada continue organizada em colunas.

## CVT com interfaces fixas — requisito confirmado, algoritmo ainda a definir

Antes de implementar CVT, inventariar o código público atual. Um comentário histórico sobre
Lloyd não comprova a existência de um otimizador pronto.
Se CVT for incluído numa entrega futura, cada iteração deverá:

1. Manter uma referência imutável da partição regional e dos planos/retas de interface.
2. Calcular deslocamentos de sítios por região, sem permitir troca involuntária de domínio.
3. Tratar centroides fora da região (possíveis em células não convexas) com política explícita;
   não usar atualização de Lloyd irrestrita que atravesse interfaces.
4. Reconstruir e recortar as células contra a mesma partição, seguida de compatibilização
   dos dois lados. Não reconstruir a fronteira a partir das novas bissetrizes dos sítios.
5. Permitir que vértices das faces de interface deslizem dentro dos suportes fixos;
   preservar extremos, bordas e interseções dos suportes.
6. Conferir em TODAS as iterações: resíduos de plano/reta relativos à escala, cobertura
   da extensão original, ausência de sobreposição/lacuna, pertença regional e conformidade.
7. Verificar critério de convergência separadamente da validade geométrica. Uma redução
   da energia CVT não compensa uma fronteira que se moveu.
8. Testar 2D com reta oblíqua e interface poligonal; 3D com plano oblíquo e interface
   facetada; junções, fronteiras finitas, células não convexas e várias iterações.
9. Incluir caso em que a subdivisão da interface de fato mude, demonstrando que o
   teste não exige indevidamente coordenadas ou IDs de vértices idênticos.

A escolha de energia, tratamento de densidade geométrica, política de deslocamento,
convergência e API do CVT fica para o contrato dessa extensão. Não introduzir propriedades
físicas do mohid-ng para definir essas escolhas.
