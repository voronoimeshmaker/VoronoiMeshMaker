Sítios
======

Cada região recebe uma ou mais fontes de sítios (``sites_for``), todas
determinísticas pela semente, com um gerador portável (o mesmo resultado em
qualquer plataforma):

* ``UniformRandomSource``: sorteio com distância mínima (espaçamento constante);
* ``AdaptiveQuadtreeSource``: espaçamento variável h(x) por quadtree adaptativa;
* ``RandomCountSource``: N pontos uniformes, sem distância mínima;
* ``CartesianGridSource`` e ``HexagonalGridSource``: grades;
* ``ExplicitSites``: sítios dados pelo usuário.

``InterfacePairs`` (opcional, DEC-028) coloca pares espelhados ao longo das
interfaces; onde o espelhamento funciona, as faces de interface ficam
ortogonais. A conformidade é sempre garantida pelo refinamento comum.
