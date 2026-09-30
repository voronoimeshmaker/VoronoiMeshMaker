# VoronoiMeshMaker (`vmm/`)

Malhas de volumes finitos de Voronoi em 2D e 3D, com várias regiões, buracos e
interfaces conformes, prontas para um solver (`generate_mesh_2d`,
`generate_mesh_3d`). Esta é a biblioteca nova (arquitetura do
`planning/P06_arquitetura_a.md`), que substituiu a VMMLib legada (DEC-011).
As malhas da VMMLib nos casos O1–O4 ficam congeladas como arquivos de
referência em `tests/data/golden/`.

- **Domínio por precedência** (regiões e buracos, formas com rótulos de patch),
  convertido numa partição exata e validada; em 3D, formas analíticas (caixa,
  esfera, cilindro, extrusão) ou arquivos STL reparados.
- **Sítios por região**, determinísticos e portáveis; espaçamento variável.
- **Malha conforme**: topologia por rótulos, vértices canônicos, recorte exato
  das células de contorno, refinamento comum nas interfaces, invariantes da
  DEC-011 verificados em cada malha; pares de sítios espelhados opcionais para
  faces de interface ortogonais.
- **Pronta para volumes finitos**: owner/neighbour, vetores de área, distâncias,
  não ortogonalidade, skewness, intervalos de faces internas e de contorno,
  volumes internos e de contorno, adjacência CSR e padrão esparso, RCM.
- **Arquivos**: formato nativo com ida e volta exata, VTK XML (polígonos e
  poliedros) e STL (domínios 3D).
- **Sem escrever C++**: o executável `vmm-mesh` gera a malha a partir de um
  arquivo de configuração (`vmm-mesh bloco.cfg`); no C++, `vmm::value_or_throw`
  troca a verificação de cada `Result` por exceções.

Versão 1.0: API estável com versionamento semântico (DEC-041); a referência da
API na documentação diz o que é interno.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build && ctest --test-dir build -L vmm
```

Documentação: `vmm/docs/build_docs.sh <build> [venv]`. Licença: `vmm/LICENSE`
(BSD-3-Clause no núcleo; GPL no backend CGAL).
