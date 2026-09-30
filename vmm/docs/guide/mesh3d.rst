.. SPDX-License-Identifier: BSD-3-Clause

======================
Malhas 3D (versão 0.3)
======================

A versão 0.3 gera malhas de volumes finitos de Voronoi em 3D para **uma região**. Várias regiões com interfaces conformes chegam na versão 0.5 (P18); domínios STL, na 0.4 (P17).

Domínio
-------

- O domínio é uma superfície triangulada fechada, orientada para fora, com um patch por triângulo (DEC-036). ``TriangleSurface::make`` verifica a superfície; o backend confere, em aritmética exata, que ela não se auto-intercepta.
- Formas prontas: ``Cuboid``, ``Sphere`` (icosaedro subdividido), ``Cylinder``, ``Extrusion`` de um contorno 2D (as arestas rotuladas viram patches das paredes) e ``SurfaceShape`` para uma superfície dada. O ``ShapeRegistry3D`` constrói formas pelo nome.

Sítios
------

- Fontes 3D: ``UniformRandomSource3D`` (espaçamento mínimo), ``RandomCountSource3D``, ``CartesianGridSource3D`` e ``ExplicitSites3D``. A mesma semente gera os mesmos sítios em qualquer plataforma.

Construção
----------

- Cada célula nasce convexa, da interseção de semiespaços dos vizinhos de Delaunay; os vértices de Voronoi são circuncentros calculados em aritmética exata, iguais em todas as células que os compartilham.
- Só as células que tocam o contorno são recortadas pelo domínio, também em aritmética exata. Uma célula que o domínio divide em pedaços fica inteira (DEC-035) e é contada em ``BuildStats3D::fragmented_cells``.
- A malha não depende da ordem dos sítios (idêntica bit a bit) e passa pelos mesmos invariantes do 2D antes de ser devolvida.

Uso
---

A saída é o mesmo ``Mesh<3>`` genérico: faces internas, faces de contorno por patch, owner/neighbour, vetores de área, métricas, adjacência e reordenação funcionam como no 2D. ``write_vtu`` grava células ``VTK_POLYHEDRON``; ``write_native`` grava o formato ``.vmesh``.


.. code-block:: cpp

   vmm::MeshRequest3D request;
   auto& d = request.declaration;
   const auto water = *d.media().add("water");
   const auto tank = *d.add_region("tank", water, vmm::Cylinder({0, 0, 0}, 1, 2, {"wall", "floor", "lid"}));
   request.sources = {vmm::sites_for_3d(tank, vmm::UniformRandomSource3D(0.1))};
   const auto result = vmm::generate_mesh_3d(request);
   if (!result) return 1;              // result.error().message() explica o motivo
   return vmm::write_vtu(result->mesh, "tanque.vtu") ? 0 : 1;
