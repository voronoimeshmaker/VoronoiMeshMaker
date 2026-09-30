.. SPDX-License-Identifier: BSD-3-Clause

=========
Malhas 3D
=========

Malhas de volumes finitos de Voronoi em 3D, com uma ou várias regiões, buracos e interfaces conformes, a partir de formas analíticas ou de arquivos STL.

Domínio
-------

- O domínio é uma superfície triangulada fechada, orientada para fora, com um patch por triângulo (DEC-036). ``TriangleSurface::make`` verifica a superfície; o backend confere, em aritmética exata, que ela não se auto-intercepta.
- Formas prontas: ``Cuboid``, ``Sphere`` (icosaedro subdividido), ``Cylinder``, ``Extrusion`` de um contorno 2D (as arestas rotuladas viram patches das paredes) e ``SurfaceShape`` para uma superfície dada. O ``ShapeRegistry3D`` constrói formas pelo nome.

Sítios
------

- Fontes 3D: ``UniformRandomSource3D`` (espaçamento mínimo), ``AdaptiveOctreeSource3D`` (espaçamento variável h(x), por octree), ``RandomCountSource3D``, ``CartesianGridSource3D`` e ``ExplicitSites3D``. A mesma semente gera os mesmos sítios em qualquer plataforma.

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

Domínio de um arquivo STL (versão 0.4)
--------------------------------------

- ``read_stl_surface`` lê um STL ASCII ou binário e o repara numa superfície fechada: solda pontos a menos de 10⁻⁹ da diagonal da caixa, remove triângulos colapsados e repetidos e orienta cada componente de forma consistente. Buracos e arestas com mais de dois triângulos são erros (``InvalidSurface``), nunca preenchidos às cegas.
- Num STL ASCII, cada bloco ``solid nome`` vira um patch; um STL binário tem um patch só. ``write_stl`` grava uma superfície nos dois formatos.
- A superfície entra na declaração com ``SurfaceShape``. Cada célula de contorno é recortada só contra os triângulos próximos dela, em aritmética exata; o custo por célula não cresce com o tamanho do arquivo.

.. code-block:: cpp

   const auto surface = vmm::read_stl_surface("terreno.stl");
   if (!surface) return 1;             // surface.error().message() explica o motivo
   vmm::MeshRequest3D request;
   const auto soil = *request.declaration.media().add("soil");
   const auto ground = *request.declaration.add_region("ground", soil, vmm::SurfaceShape(*surface));
   request.sources = {vmm::sites_for_3d(ground, vmm::UniformRandomSource3D(0.05))};
   const auto result = vmm::generate_mesh_3d(request);

Várias regiões
--------------

- As regiões e os buracos são declarados por precedência, como no 2D (DEC-018): cada camada pinta por cima das anteriores; ``add_hole`` remove. O backend autorrefina as superfícies de todas as camadas em aritmética exata e classifica cada triângulo pelos dois lados; faces coplanares de camadas diferentes são tratadas.
- Cada região tem o seu próprio diagrama de Voronoi (DEC-028). Numa interface, a face entre duas células é a interseção dos seus pedaços sobre cada triângulo (refinamento comum), de modo que a interface é conforme e a sua área é conferida nos invariantes.
- As faces de interface não são faces de Voronoi e podem ser bem não ortogonais. ``SiteGenerationOptions3D::interface_pairs`` (``InterfacePairs3D``) põe pares de sítios espelhados através de cada interface: a face entre os dois sítios de um par fica ortogonal, e a não ortogonalidade média nas interfaces cai de cerca de 0,5 rad para menos de 0,05 rad.

.. code-block:: cpp

   vmm::MeshRequest3D request;
   auto& d = request.declaration;
   const auto rock = *d.media().add("rock");
   const auto water = *d.media().add("water");
   const auto block = *d.add_region("block", rock, vmm::Cuboid({0, 0, 0}, {1, 1, 1}));
   const auto lens = *d.add_region("lens", water, vmm::Sphere({0.5, 0.5, 0.5}, 0.3));  // pinta por cima
   request.sources = {vmm::sites_for_3d(block, vmm::UniformRandomSource3D(0.08)),
                      vmm::sites_for_3d(lens, vmm::UniformRandomSource3D(0.05))};
   request.sites.interface_pairs = vmm::InterfacePairs3D(0.05);           // faces de interface ortogonais
   const auto result = vmm::generate_mesh_3d(request);
