# Changelog

## Não publicado — domínio STL (entrega c, planejada como 0.4 na DEC-025)

### Novo
- `read_stl` / `write_stl` (ASCII com um patch por `solid`, e binário) e `read_stl_surface`.
- `repair_surface`: solda de pontos, remoção de triângulos colapsados e repetidos,
  orientação consistente por componente; buracos e arestas não manifold são erros.
- Recorte local: cada célula de contorno é recortada só contra os triângulos do domínio
  próximos dela (corefinement e classificação exata por pedaço), com o recorte contra o
  domínio inteiro como reserva; `BuildStats3D::local_clips`, `CellClip3::local`.
- `TriangleSurface` com hierarquia de caixas: `contains` (pseudonormais) e `distance` em
  O(log n).
- Terreno sintético `anchors::terrain_block`, benchmark B6 e exemplo `ex_stl_domain`.

## Não publicado — 3D com uma região (entrega b, planejada como 0.3 na DEC-025)

### Novo
- Domínio 3D por superfície triangulada fechada, com patch por triângulo (DEC-036):
  `TriangleSurface`, `Box3`, formas `Cuboid`, `Sphere`, `Cylinder`, `Extrusion`,
  `SurfaceShape` e o registro `ShapeRegistry3D`.
- `Declaration3D`, `Partition3D`, sítios 3D (`SiteSet3D` e quatro fontes).
- Backend 3D (`Backend3D`, `cgal_backend_3d()`): Delaunay 3D, circuncentros exatos,
  recorte exato rotulado das células de contorno.
- `build_mesh_3d` e `generate_mesh_3d`: malha de Voronoi 3D de uma região, idêntica
  bit a bit para qualquer ordem dos sítios; célula partida pelo domínio fica inteira
  (DEC-035).
- `write_vtu(Mesh3D)` com células `VTK_POLYHEDRON`; benchmark B4 e B5; exemplo 3D na
  galeria.
- Código de erro `InvalidSurface`.

### Mudou
- `ShapeOutline::make` rejeita contornos que se tocam ou se cruzam.
- Build padrão Release com LTO e `-march=native` (DEC-034).

## 0.1.0 — 2026-09-29 (candidata; publicação depende da aprovação do P14)

### Novo
- Biblioteca `vmm/` com alvos `vmm_core` (BSD, sem CGAL), `vmm_backend_cgal`
  (GPL), `vmm_io` e a fachada `vmm`; `find_package(VoronoiMeshMaker 0.1)`.
- Domínio 2D por precedência, partição exata por arranjo (CGAL) e validador.
- Fontes de sítios por região e pares espelhados opcionais nas interfaces.
- Construtor 2D multirregião conforme (DEC-028), com caminho rápido.
- `Mesh<D>` imutável, métricas de volumes finitos, relatório de qualidade,
  invariantes da DEC-011, reordenação (RCM, Hilbert, lexicográfica).
- Formato nativo `.vmesh` (leitura e escrita) e VTK XML `.vtu`.
- Subsistema de erros próprio (`Result<T>`, catálogo pt/en).
- Benchmark B1–B3, oráculo contra a VMMLib (O1–O4), CI, documentação e galeria.

### Mudou em relação à VMMLib
- Sem tipos do CGAL na API; `Real = double` próprio.
- Domínios não convexos, com buracos e várias regiões.
- Tolerâncias relativas à escala do domínio; nenhuma constante absoluta.
- Sem escritor OpenFOAM (DEC-030).
