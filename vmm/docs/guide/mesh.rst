Malha, iteradores e adjacência
==============================

``Mesh2D`` é imutável depois de construída (R7):

* faces ``[0, internal_face_count())`` são internas, com ``owner < neighbour`` e
  o vetor de área apontando do owner para o neighbour, em ordem triangular
  superior; as demais são de contorno, agrupadas em faixas contíguas por patch
  (DEC-029);
* ``internal_faces()``, ``boundary_faces()`` e ``patch_faces(p)`` são intervalos
  iteráveis de ``FaceId``;
* ``CellFaceIndex`` dá ``faces_of(c)``, ``internal_cells()`` (volumes sem face de
  contorno) e ``boundary_cells()``;
* ``cell_adjacency(mesh)`` devolve a adjacência em CSR e ``sparse_pattern`` o
  padrão de uma matriz esparsa com diagonal, sem geometria (DEC-015).

.. code-block:: cpp

   const vmm::CellFaceIndex index(mesh);
   for (const vmm::CellId c : index.internal_cells()) {
       for (const vmm::FaceId f : index.faces_of(c)) { /* fluxo pela face f */ }
   }
   for (const vmm::FaceId f : mesh.boundary_faces()) { /* condição de contorno */ }

Métricas (``compute_metrics``): medida e centroide das células, vetor de área e
centroide das faces, distância entre geradores, ponto de interseção, não
ortogonalidade, skewness e razão de aspecto. ``quality_report`` resume por
região. Reordenação: ``RcmOrdering``, ``HilbertOrdering``,
``LexicographicOrdering`` e ``renumber``.
