# VoronoiMeshMaker 0.1 (`vmm/`)

Malhas de volumes finitos de Voronoi em 2D, multirregião, com interfaces
conformes, prontas para um solver. Esta é a biblioteca nova (arquitetura do
`planning/P06_arquitetura_a.md`); a `VMMLib/` legada permanece como oráculo até
a retirada (DEC-011).

- **Domínio por precedência** (regiões e buracos, formas com rótulos de patch),
  convertido numa partição exata e validada.
- **Sítios por região**, determinísticos e portáveis; espaçamento variável.
- **Malha conforme**: topologia por rótulos, vértices canônicos, invariantes da
  DEC-011 verificados em cada malha.
- **Pronta para volumes finitos**: owner/neighbour, vetores de área, distâncias,
  não ortogonalidade, skewness, intervalos de faces internas e de contorno,
  volumes internos e de contorno, adjacência CSR e padrão esparso, RCM.
- **Arquivos**: formato nativo com ida e volta exata e VTK XML.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DVMM_BUILD_VMMLIB=OFF
cmake --build build && ctest --test-dir build -L vmm
```

Documentação: `vmm/docs/build_docs.sh <build> [venv]`. Licença: `vmm/LICENSE`
(BSD-3-Clause no núcleo; GPL no backend CGAL).
