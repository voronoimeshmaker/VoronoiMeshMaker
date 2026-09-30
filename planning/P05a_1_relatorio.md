# P05a.1 — Relatório: firewall de compilação e adjacência compacta

- **Data:** 2026-09-29
- **Sequência:** v5.2, prompt P05a, iteração P05a.1 (plano: `planning/P05a_plano.md`)
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Ambiente e versões (DEC-012)

Executado **diretamente** na distribuição oficial, não na nuvem como previa o plano §1 **[F]**:

| Item | Valor |
|---|---|
| WSL | `Ubuntu-26.04-Test` (Ubuntu 26.04.1 LTS, kernel 6.18.33.2-microsoft-standard-WSL2) |
| Compilador | g++ 15.2.0 (Ubuntu 15.2.0-16ubuntu1), `-std=c++23` |
| CMake / Ninja | 4.2.3 / 1.13.2 |
| CGAL | 6.2.1, pacote `/usr/local/lib/cmake/CGAL`, headers `/usr/local/include` (`CGAL_VERSION_NR` 1060211000, checado por `static_assert` no backend) |
| Boost | 1.92 (`BOOST_LIB_VERSION` compilado no backend) |
| Número exato do Epeck | GMP (`__gmp_expr<__mpq_struct…>`) |
| Commit | `d3edfe7` + árvore de trabalho; `prototypes/P05a/` é novo (não versionado) |

**[F]** Existe um segundo CGAL (6.1.1) em `/usr/include/CGAL` e um Boost 1.90 em `/usr/include/boost`. O GCC procura em `/usr/local/include` antes de `/usr/include`, e o `static_assert` confirma que os headers usados são os da 6.2.1.

## 2. Arquivos

| Arquivo | Papel |
|---|---|
| `prototypes/P05a/CMakeLists.txt` | projeto próprio, fora do build principal; alvos `p05a_core` (sem CGAL), `p05a_backend_cgal`, `p05a_firewall`, `p05a_firewall_negative` (fora do `all`) e testes CTest |
| `prototypes/P05a/cmake/ScanIncludes.cmake` | lista, com `c++ -M`, todos os headers abertos por cada header público e falha se algum for do CGAL, Boost, GMP ou MPFR |
| `prototypes/P05a/include/p05a/types.hpp` | `Real = double`, `Id<Tag>` (IDs fortes `CellId`, `FaceId`, `RegionId`, `PatchId`), `Vec<D>` |
| `prototypes/P05a/include/p05a/csr.hpp` | `Csr<T>`, `build_adjacency`, `sparse_pattern`, `is_structurally_symmetric` |
| `prototypes/P05a/include/p05a/backend.hpp` | interface do backend só com tipos do VMM e padrão |
| `prototypes/P05a/src/backend_cgal/delaunay_pairs.cpp` | pares vizinhos das Delaunay 2D e 3D (CGAL, Epick) e versões compiladas |
| `prototypes/P05a/firewall_negative/p05a/contaminated.hpp` | controle negativo: header que inclui o CGAL |
| `prototypes/P05a/apps/p05a_1_csr.cpp` | prova da tarefa 4 |
| `prototypes/P05a/apps/p05a_core_only.cpp` | executável ligado só a `p05a_core` |
| `prototypes/P05a/run_evidence.sh` | roteiro de evidência (configura, compila, testa e roda num build fora da árvore) |

Nenhum arquivo de `VMMLib/`, `tests/`, `cmake/` ou do `CMakeLists.txt` raiz foi alterado **[F]**.

## 3. Firewall (tarefa 3, DEC-007)

Três checagens independentes:

1. **Por macro:** uma unidade de tradução por header público, compilada **sem** ligar `CGAL::CGAL`; falha com `#error` se `CGAL_VERSION_NR` ou `BOOST_VERSION` estiverem definidos.
2. **Por dependências:** `c++ -M` em cada header; falha se aparecer qualquer caminho `/CGAL/`, `/boost/`, `gmp` ou `mpfr`.
3. **Controles negativos:** o mesmo teste aplicado a `contaminated.hpp` precisa falhar (CTest `WILL_FAIL` na compilação; `EXPECT_LEAK` na varredura).

**[F]** Resultado:
```
1/7 Test #1: p05a_firewall_scan ....................... Passed
    -- scanned 7 header(s), 1407 dependency entries, 0 CGAL/Boost/GMP/MPFR entries
2/7 Test #2: p05a_firewall_scan_negative_control ...... Passed
    -- negative control detected as expected, first entry: .../contaminated.hpp -> /usr/local/include/CGAL/Exact_predicates_inexact_constructions_kernel.h
3/7 Test #3: p05a_firewall_compile_negative_control ... Passed   (error: #error "p05a/contaminated.hpp reaches CGAL or Boost")
4/7 Test #4: p05a_core_only ........................... Passed   (core-only 4x3 grid: nnz 46 (expected 46) -> PASS)
```

**[F] Limitação:** "compilar sem o CGAL no caminho de includes", ao pé da letra, não é possível nesta máquina. `/usr/include` e `/usr/local/include` são caminhos padrão do compilador e ambos contêm `CGAL/`. Por isso a prova usa a lista de headers efetivamente abertos (checagem 2), que é uma evidência equivalente e mais direta. **[R]** O P07 deve usar a mesma técnica na CI, e não confiar na ausência de `-I`.

## 4. Adjacência compacta (tarefa 4, DEC-015)

**[F]** Saída de `p05a_1_csr`:
```
2D: cells 2000 | Delaunay pairs 5976 | nnz 13952 | degree min 3 max 11 mean 5.976 | CSR bytes/cell 31.9
3D: cells 1000 | Delaunay pairs 7415 | nnz 15830 | degree min 6 max 29 mean 14.830 | CSR bytes/cell 67.3
```
Em 2D e 3D, o padrão é estruturalmente simétrico, é igual ao calculado diretamente dos pares (com `std::set`), tem `nnz = células + 2·pares` e nenhuma célula fica isolada. O padrão é montado só com a CSR, sem coordenadas. Nas provas 2D e 3D (P05a.2 e P05a.3), a mesma CSR também é derivada de owner/neighbour da malha (`cell_adjacency`), e a simetria é verificada lá.

## 5. Desvios do plano

- O backend ganhou um segundo `.cpp` com CGAL (`region_clip.cpp`, usado no P05a.2). O plano falava em "único arquivo". A regra continua valendo: CGAL só em `src/backend_cgal/*.cpp`.
- O backend chega ao núcleo como um `struct` de ponteiros de função (`Backend2D`, `Backend3D`), montado pelo executável. Com isso, `p05a_core` compila e liga sem nenhum símbolo do CGAL. É composição sem `virtual` (R3).

## 6. Comando

```bash
prototypes/P05a/run_evidence.sh /tmp/p05a_build
```
