.. SPDX-License-Identifier: BSD-3-Clause

Colunas, horizontes e interfaces fixas (planejado)
==================================================

Estado do recurso
-----------------

Esta página registra o contrato da extensão planejada. A geração por colunas e horizontes, a retesselação geral e um otimizador CVT não são apresentados como funcionalidades disponíveis da API 1.0.

A geração multirregião existente constrói Voronoi por região, recorta as células e compatibiliza as interfaces. Esse refinamento comum não equivale a uma operação geral de retesselação de uma malha pronta.

Separação geométrica dos domínios
---------------------------------

O VMM fornece geometria, topologia e identificadores. O mohid-ng consome a malha e associa as propriedades físicas. Uma região identifica um domínio geométrico; uma camada de discretização não cria necessariamente uma região nova.

A geração planejada compartilha uma base Voronoi 2D entre as faixas verticais. Uma transformação posterior poderá desfazer as colunas, mas não poderá deslocar as fronteiras entre regiões. Essa malha não é necessariamente um Voronoi euclidiano 3D.

Suporte fixo e faces variáveis
------------------------------

Em 3D, o plano de uma interface plana deve ficar fixo. As faces das células nessa interface podem mudar de forma, tamanho, quantidade e conectividade, sem sair do plano. Em 2D, a reta de cada trecho da interface deve ficar fixa; as arestas podem mudar sua subdivisão sem sair desse trecho.

Também devem ser preservados a extensão da fronteira, seus limites, as junções e o par de regiões de cada lado. Permanecer no mesmo plano ou na mesma reta infinita não basta. Os dois lados precisam compartilhar uma subdivisão conforme, sem lacunas nem sobreposições.

Para horizontes não planos, a representação geométrica de referência ainda deve ser definida no contrato: não se deve substituir uma superfície curva por um único plano. O erro de aproximação inicial e a preservação posterior são verificações distintas.

Contrato para CVT
-----------------

Qualquer CVT futuro deve manter a partição regional e seus suportes fixos a cada iteração. Mover sítios e reconstruir células pode reorganizar as faces da interface, mas não mover a interface. Devem ser verificados suporte, cobertura, pertença regional e conformidade; conservar apenas áreas ou volumes totais não demonstra essa propriedade.

Planejamento e documentação
---------------------------

A sequência de trabalho está em ``planning/horizontes/00_sequencia.md``, com etapas P21 a P29. As decisões ainda abertas estão em ``01_requisitos.md``; o contrato futuro de tesselação e CVT está em ``T01_tesselacao_futura.md``. Exemplos de uso serão acrescentados somente depois dos testes por classe e de integração.

