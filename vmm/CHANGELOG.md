# Changelog

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
