# VoronoiMeshMaker (VMM)

Biblioteca em **C++23** que gera malhas de volumes finitos de Voronoi em 2D, com
várias regiões e interfaces conformes, prontas para um solver. A biblioteca está
em [`vmm/`](vmm/README.md).

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build -L vmm
```

Dependências: CGAL ≥ 6.2, Boost ≥ 1.83 e GoogleTest ≥ 1.14 (só nos testes).
Opções: `VMM_BUILD_TESTS` e `VMM_BUILD_EXAMPLES` (ambas `ON` por padrão).

## Estrutura

| Pasta | Conteúdo |
|---|---|
| `vmm/` | biblioteca, testes, exemplos, ferramentas e documentação |
| `cmake/` | opções do projeto e verificação dos headers do CGAL |
| `planning/` | requisitos, decisões (`DECISIONS.md`) e relatórios da migração |
| `prototypes/` | provas de conceito do P05a |

## Documentação

`vmm/docs/build_docs.sh <build> [venv]` gera o site em português e em inglês.
As regras do projeto estão na página "Diretrizes do projeto" (`vmm/docs/guidelines.rst`).

## Licença

Núcleo sob BSD-3-Clause; backend CGAL sob GPL-3.0-or-later. Veja `LICENSES.md`.
