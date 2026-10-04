.. SPDX-License-Identifier: BSD-3-Clause

Colunas, horizontes e interfaces fixas
======================================

Estado da extensão
------------------

A versão 1.1 oferece geração por colunas a partir de uma malha 2D, validada pelos testes registrados nos relatórios P27. Retesselação geral e otimizador CVT não estão implementados.

Separação geométrica dos domínios
---------------------------------

O VMM fornece geometria, topologia e identificadores. O mohid-ng consome a malha e associa as propriedades físicas. A extrusão combina a região horizontal com o intervalo entre horizontes; subdivisões do mesmo intervalo mantêm a mesma região. Esta malha deriva de Voronoi 2D e não é necessariamente um Voronoi euclidiano 3D.

Referência geométrica
---------------------

``HorizonGrid::make`` recebe eixos x/y estritamente crescentes, nomes únicos e cotas nodais por horizonte, ordenadas com x variando mais rapidamente. Cada retângulo usa a diagonal sudoeste–nordeste. As cotas definem superfícies afins por triângulo; ``elevation`` consulta essa representação. A grade deve cobrir toda a base.

``HorizonGrid::sample`` recebe funções z(x,y), avaliadas uma única vez nos nós. Depois disso, a referência são as cotas armazenadas, não a função original. A resolução da grade controla a aproximação inicial; mudar sítios da malha base não reamostra a superfície. Exceções lançadas pelas funções de entrada propagam para o chamador.

Os horizontes são ordenados de baixo para cima. Igualdade permite encontros e desaparecimento parcial de intervalos, inclusive dentro de uma coluna original. Inversões e valores não finitos são rejeitados. Células de volume zero não são criadas; falhas de representabilidade são reportadas.

Geração, consultas e persistência
---------------------------------

``generate_layered_mesh`` recebe ``Mesh2D``, ``HorizonGrid`` e, opcionalmente, frações por intervalo. Cada lista deve aumentar estritamente de 0 a 1; a omissão cria uma camada por intervalo. Frações igualmente espaçadas produzem subdivisões uniformes; outras listas permitem graduação. O refinamento comum inclui a base e os triângulos da referência, com suporte a concavidades, buracos e componentes separados.

``LayeredMesh::mesh`` retorna a geometria 3D. ``data`` contém grade, frações, número de colunas, coluna/camada por célula e nível por face. ``cells_in_column`` ordena células pela camada; ``vertical_neighbours`` consulta adjacência através das superfícies de subdivisão. Os sítios 3D são referências calculadas por momentos volumétricos, não geradores de Voronoi 3D.

Colunas e camadas usam índices iniciados em zero. ``lateral_face`` marca faces laterais; outros níveis identificam superfícies de subdivisão. Em encontros coincidentes, uma face compartilhada pode representar vários níveis geométricos coincidentes e guarda o menor nível incidente. A renumeração de ``LayeredMesh`` preserva os metadados. Uma transformação futura que destrua colunas precisará produzir metadados coerentes com a nova topologia.

``write_layered`` e ``read_layered`` persistem a malha com metadados no formato ``VMM_LAYERS 1`` (extensão ``.vlayers``), que incorpora uma malha nativa. O formato ``.vmesh`` existente permanece compatível, mas sozinho não guarda a referência de horizontes. A saída VTU representa a geometria volumétrica.

Na configuração de ``vmm-mesh``, ``dimension = 2`` define a base e a chave global ``horizons`` aponta para um arquivo relativo à configuração. ``read_horizons`` lê ``VMM_HORIZONS 1``: linhas dos eixos (quantidade seguida de valores), quantidade de horizontes, nome entre aspas e linha de cotas para cada horizonte, quantidade de listas de frações e suas linhas. A saída é 3D e inclui ``.vlayers``.

Suporte fixo e faces variáveis
------------------------------

Em 3D, o plano de cada faceta regional permanece fixo. Faces podem mudar de forma, tamanho, quantidade e conectividade dentro desse suporte. Em 2D, a reta de cada trecho permanece fixa, com subdivisão variável. Também se preservam extensão, extremos, junções e regiões incidentes; preservar apenas área ou volume não basta.

Esse contrato vale para qualquer CVT futuro em cada iteração. Os testes atuais mudam sítios e reconstruem malhas com a mesma referência; não executam CVT. Para superfícies facetadas, cada faceta de referência e suas junções devem permanecer fixas.

Limitações e validação
----------------------

A construção exata do refinamento comum termina em coordenadas ``double``. Interseções que arredondam ao mesmo ponto são unificadas globalmente; uma coluna que desapareça por completo nessa conversão é rejeitada. Geometrias extremamente finas ou próximas de limites de precisão podem falhar. Não há garantia de escala arbitrária nem promessa de ortogonalidade 3D.

``LayeredMesh::from_data`` verifica metadados e amostras de suporte e contenção nas faces. Isso não substitui uma prova de conformidade de malhas externas arbitrariamente modificadas. Os testes do gerador verificam também invariantes geométricos e volumes integrados independentemente. A classificação do refinamento comum percorre as arestas da base para cada triângulo e pode dominar o custo em malhas grandes.

A sequência e os relatórios estão em ``planning/horizontes``. Os exemplos foram adicionados após os testes de classes e integração. O contrato de transformação futura está em ``T01_tesselacao_futura.md``.


Exemplos executáveis
--------------------

A galeria inclui ``horizons`` (C++) e ``horizons_flat`` (configuração). O caso plano produz arquivos ``.vlayers`` idênticos nas duas interfaces; o caso de terreno mostra superfícies facetadas e um encontro na borda. As cores representam regiões geométricas, sem propriedades físicas.
