# Changelog

## Não publicada

### Mudou
- Clang mínimo 19 (DEC-042): com a libstdc++, o Clang 18 não tem `std::expected`, e a
  biblioteca não compila com ele. O job do Clang na CI passa a usar o `clang-19`.

### Corrigido (CI)
- Clang 19 com `-march=native` em CPUs com AVX10.1: o aviso `invalid feature combination`
  virava erro por causa do `-Werror`. Agora é silenciado quando o `-march=native` está ligado.
- Build com sanitizadores: sem a verificação `vptr` do UBSan, que acusa um downcast dentro
  dos iteradores do `Arrangement_2` do CGAL; o vmm não tem funções virtuais (R3).
- Controle negativo do firewall: recebe o diretório de includes do CGAL, para funcionar com
  o CGAL fora dos diretórios do sistema.
- Documentação: `build_docs.sh` é chamado com `bash` (o arquivo não tem permissão de execução no git).
- A CI publica em anotações os testes que falharam e as linhas de erro.

## 1.0.0 — 2026-09-30

API estável (DEC-041) e facilidade de uso (DEC-040).

### Novo
- `vmm::value_or_throw`: devolve o valor de um `Result` ou lança `vmm::Exception` com o
  mesmo erro, para quem prefere exceções a testar cada resultado.
- Arquivos de configuração e o executável `vmm-mesh` (instalado em `bin/`): uma malha 2D ou 3D
  a partir de um arquivo `chave = valor`, sem escrever C++. No C++: `MeshConfig`,
  `make_request_2d/3d`, `run_config`, `ConfigRegistries` e os registros abertos de fontes de
  sítios `SiteSourceRegistry2D/3D`. Formas extras dos arquivos 3D: `stl` e `extrusion`.
- `vmm/core/version.hpp` (`version_string`, `version_major`...).
- Galeria: exemplos `value_or_throw`, `soil2d` e `layers3d` (os dois últimos executados pelo
  `vmm-mesh`).

### Mudou
- Versionamento semântico: o pacote aceita qualquer 1.x em `find_package(VoronoiMeshMaker 1.0)`
  (`SameMajorVersion`).
- Interno, sem garantia de estabilidade: os membros do backend (exceto `build_partition`), os
  tipos de rótulo e de recorte, `vmm::detail` e os contadores de `BuildStats2D/3D`. A referência
  da API lista o que é estável.

### Corrigido
- Um teste 3D usava uma referência a um temporário num `for` por intervalo, que o GCC 14
  destrói antes do laço (o GCC 15 prolonga a vida do temporário; P2718). A CI com GCC 14
  falhava na compilação por `-Werror=dangling-reference`.
- CMake sem varredura de módulos C++ (`CMAKE_CXX_SCAN_FOR_MODULES OFF`): o projeto não usa
  módulos, e o Clang precisaria de `clang-scan-deps`.

## 0.2.0 — 2026-09-30

Malhas 3D: entregas (b), (c) e (d) numa só versão (DEC-038).

### 3D com várias regiões (entrega d)

#### Novo
- Partição 3D por precedência (DEC-018): regiões e buracos (`Declaration3D::add_hole`),
  autorrefinamento exato de todas as camadas e classificação de cada triângulo pelos dois
  lados; faces coplanares de camadas diferentes; arredondamento para double sem interseções.
- `build_mesh_3d` com várias regiões: um diagrama de Voronoi por região (DEC-028) e o
  refinamento comum dos pedaços sobre cada triângulo de interface; `BuildStats3D` com
  `interface_faces` e `interface_slivers`.
- `AdaptiveOctreeSource3D`: sítios 3D com espaçamento variável h(x), por octree.
- `InterfacePairs3D` (pares espelhados através das interfaces, E2) em
  `SiteGenerationOptions3D::interface_pairs`.
- Âncora A3 (`anchors::a3`, `anchors::heightfield_block`), benchmark B7, exemplo
  `ex_anchor_a3`; a galeria corta as malhas de várias regiões na vertical.

#### Mudou
- A limpeza das faces divide laços que se tocam num vértice e roda também depois da
  inserção de vértices em "T".

### Domínio STL (entrega c)

#### Novo
- `read_stl` / `write_stl` (ASCII com um patch por `solid`, e binário) e `read_stl_surface`.
- `repair_surface`: solda de pontos, remoção de triângulos colapsados e repetidos,
  orientação consistente por componente; buracos e arestas não manifold são erros.
- Recorte local: cada célula de contorno é recortada só contra os triângulos do domínio
  próximos dela (corefinement e classificação exata por pedaço), com o recorte contra o
  domínio inteiro como reserva; `BuildStats3D::local_clips`, `CellClip3::local`.
- `TriangleSurface` com hierarquia de caixas: `contains` (pseudonormais) e `distance` em
  O(log n).
- Terreno sintético `anchors::terrain_block`, benchmark B6 e exemplo `ex_stl_domain`.

### 3D com uma região (entrega b)

#### Novo
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

#### Mudou
- `ShapeOutline::make` rejeita contornos que se tocam ou se cruzam.
- Build padrão Release com LTO e `-march=native` (DEC-034).

## 0.1.0 — 2026-09-29 (2D; não publicada separadamente, incluída na 0.2.0)

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
