Domínio, meios e regiões
========================

Um domínio é uma lista ordenada de camadas (DEC-018). Cada camada é uma região,
ligada a um meio, ou um buraco; uma camada posterior pinta por cima das
anteriores.

* **Formas:** ``Rectangle``, ``PolygonShape`` (com buracos), ``Circle``,
  ``Ellipse``, ``RegularNGon`` ou qualquer tipo que satisfaça o concept
  ``Shape2D``. O ``ShapeRegistry`` constrói formas pelo nome, a partir de
  parâmetros.
* **Patches:** cada aresta de forma pode levar um rótulo; a aresta de contorno
  recebe o rótulo da forma de maior precedência que a cobre (sem rótulo:
  ``boundary``).
* **Partição:** o backend constrói um arranjo exato de todas as arestas; cada
  segmento de contorno ou interface existe uma única vez e é compartilhado
  pelas duas regiões, o que torna as interfaces conformes por construção.
* **Validação:** ``validate_partition`` aponta vazios (erro, salvo com região de
  fundo), regiões anuladas pela ordem (erro), regiões fragmentadas (aviso) e
  lascas (aviso ou erro).
