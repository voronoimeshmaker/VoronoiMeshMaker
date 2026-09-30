# P08 — Núcleo: Core, Error, Geometry

- **Data:** 2026-09-29
- **Ambiente:** o do `planning/P07_infra.md` §0 (WSL `Ubuntu-26.04-Test`, g++ 15.2, CMake 4.2.3, CGAL 6.2.1, Boost 1.92, GoogleTest 1.17).
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Entregas

| Módulo | Headers (`vmm/include/vmm/…`) | Conteúdo |
|---|---|---|
| core | `types.hpp`, `csr.hpp`, `tolerance.hpp`, `random.hpp` | `Real = double`; `Id<Tag>` de 32 bits (`CellId`, `FaceId`, `VertexId`, `RegionId`, `MediumId`, `PatchId`, `SiteId`); `Vec<D>` como tipo próprio (para que os operadores sejam achados por ADL); `Csr<T>`, `build_adjacency`, `sparse_pattern`; `Tolerance` relativa a L (R17); `Random` portável (mt19937_64 com distribuições escritas à mão) |
| error | `error_code.hpp`, `error.hpp`, `exception.hpp`, `catalog.hpp`, `log.hpp` | códigos estáveis por categoria (enum de dado, DEC-024); `Error` com contexto, entidade (`EntityRef`) e `std::source_location`; `Result<T> = std::expected<T, Error>`; `vmm::Exception` sem base (`static_assert` no teste); catálogo pt/en; *sink* de log por callback, sem `virtual` |
| geometry | `polygon.hpp` | `Box2`, funções de anel, `PolygonWithHoles2`, `SegmentIndex2` (índice em grade para o caminho rápido do construtor) |

**Migração (P06 §15):**
- **Refeitos no modelo novo:** `Core/type.h`, `Core/constants.h` (a tolerância absoluta `kEpsilon` foi eliminada), `ErrorHandling/*`, `Boundary2DTypes/Data/Queries`.
- **Descartados:** `IErrorLogger` e `ThreadLocalBufferLogger` (violam o R3) e o enum `ErrorConfig::Policy` (despacho).

## 2. Testes (R1, R2)

**[F]** Um `ut_<Classe>.cpp` por classe não trivial:
- **core:** `Csr`, `Random` (inclui o valor de referência do padrão para `mt19937_64`), `Tolerance`, `Vec`;
- **error:** `Error`, `Exception`, `Catalog` (todo código tem texto em pt e em en; códigos ordenados e únicos), `Log`;
- **geometry:** `Box2`, `Ring`, `PolygonWithHoles2`, `SegmentIndex2` (comparado com força bruta em 500 consultas aleatórias).

Resultado: 45 casos de teste, todos passando.

## 3. Decisões de implementação

1. **`Vec<D>` deixou de ser um apelido de `std::array`.** **[F]** Com o apelido, `a + b` fora do namespace `vmm` não compilava, porque o ADL procura em `std`. O tipo próprio mantém a agregação e a ordenação lexicográfica.
2. **`Tolerance::from_length` devolve `std::optional`, e não `Result`.** O `core` não depende do `error` (grafo do P06 §5).

## 4. Cobertura

Ver `planning/P13_relatorio.md` §4 (medida única para toda a biblioteca, R25).
