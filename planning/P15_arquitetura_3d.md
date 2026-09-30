# P15 — Arquitetura 3D

- **Data:** 2026-09-30
- **Sequência:** `planning/sequencia_prompts.md`, Fase 3, P15 ("reuso do núcleo genérico; backend 3D conforme a P02")
- **Base:** `planning/P15a_relatorio.md` (prova de conceito, 37/37 verificações), `planning/P06_arquitetura_a.md`
  (arquitetura 2D), P02 §4–§6, DEC-002, DEC-006, DEC-007, DEC-018, DEC-020, DEC-025, DEC-028, DEC-032, DEC-034.
- **Natureza:** documental, como o P06. O código 3D começa no P16.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Ponto de partida

**[F] O que já existe e serve ao 3D sem mudança** (usado pelo P15a como está):
- `Vec<3>`, `Id<Tag>`, `Csr`, tolerâncias relativas (`core/`);
- `Mesh<3>`, `MeshData<3>`, `CellFaceIndex`, `cell_adjacency`, `sparse_pattern` (`mesh/`, `core/csr.hpp`);
- `check_invariants<3>`, `compute_metrics<3>`, `quality_report<3>` (`mesh/`);
- `renumber<3>` e as ordenações lexicográfica, Hilbert (Morton em 3D) e RCM (`reorder/`);
- `write_native<3>` / `read_native<3>` (`io/native.hpp`).

**[F] O que é só 2D hoje:** formas e declaração (`domain/`), `SiteSet` e fontes de sítios (`sites/`), `Backend2D`,
`builder2d`, `write_vtu` e a fachada `generate_mesh_2d`.

**[F] O que o P15a estabeleceu:** a hipótese (células convexas por semiespaços, recorte exato só das células de
contorno) vale nos oito casos pedidos, com quatro exigências de robustez: vértices canônicos exatos, fusão a
10⁻¹² L com descarte de lascas e inserção de vértices em "T", validação da célula antes do recorte e canonização da
saída do CGAL.

## 2. Decisões desta etapa

| DEC | Assunto | Status |
|---|---|---|
| DEC-035 | Célula partida pelo domínio fica inteira, com aviso (2D e 3D) | **APROVADA** (30/09, João) |
| DEC-036 | Domínio 3D representado por superfícies trianguladas fechadas, com patch por triângulo | **PROPOSTA** |

Detalhes em `planning/DECISIONS.md`.

## 3. Estrutura: o que entra em cada módulo

As dependências entre módulos não mudam (`vmm/tools/ci/check_requirements.py`, `ALLOWED_DEPS`). O 3D entra como
arquivos novos nos módulos existentes, sem módulo novo.

```
vmm/include/vmm/
  geometry/ surface.hpp        TriangleSurface: pontos, triângulos, rótulo por triângulo; área, volume, caixa (double)
  domain/   shapes3d.hpp       concept Shape3D; Box, Sphere, Cylinder, Extrusion (de uma forma 2D), SurfaceShape
            declaration3d.hpp  Declaration3D: camadas por precedência, como o 2D (DEC-018)
            partition3d.hpp    Partition3D: superfícies fechadas por região, rótulos de patch e de interface
            validator3d.hpp    validate_partition(Partition3D): vazios, regiões anuladas, lascas
  sites/    site_set.hpp       SiteSetD<D>; SiteSet continua sendo o 2D (alias, API 0.1 preservada)
            sources3d.hpp      fontes 3D: aleatória uniforme, grade cartesiana, contagem, explícita
  backend/  backend3d.hpp      struct Backend3D (callables, só tipos do VMM)
  voronoi/  builder3d.hpp      build_mesh_3d, BuildStats3D
  io/       vtu.hpp            write_vtu(Mesh3D): células VTK_POLYHEDRON (tipo 42)
  vmm.hpp                      generate_mesh_3d(MeshRequest3D), cgal_backend_3d()
vmm/src/backend/cgal/          delaunay3d.cpp, circumcentre3d.cpp, clip3d.cpp, partition3d.cpp
```

**[R] API 0.1 congelada (P14 §3):** nenhum nome 2D muda. As generalizações entram como templates novos com
aliases para os nomes atuais (`using SiteSet = SiteSetD<2>`).

## 4. Backend 3D (DEC-002, DEC-006, DEC-007)

Mesmo modelo do `Backend2D`: um struct de ponteiros para função, com tipos só do VMM, e o CGAL confinado em
`vmm_backend_cgal`.

```cpp
struct LabelledPolyhedron3 {            // célula convexa: faces poligonais com rótulo
    std::vector<Vec3> points;
    Csr<std::uint32_t> faces;           // laços anti-horários vistos de fora
    std::vector<EdgeLabel> face_labels; // vizinho j, face do domínio ou caixa envolvente
};
struct CellClip3 {
    LabelledPolyhedron3 cell;           // faces agrupadas por rótulo, laços em ordem canônica
    Real volume = 0;                    // exato, arredondado
    std::size_t components = 0;         // > 1: célula partida (DEC-035)
    std::string error;                  // entrada inválida ou falha do backend
};
struct PreparedDomain3 {                // estado opaco do backend (árvore AABB, malha exata)
    std::shared_ptr<const void> state;
};
struct Backend3D {
    Result<Partition3D> (*build_partition)(const Declaration3D&);
    Result<PreparedDomain3> (*prepare)(const Partition3D&, RegionId);
    std::vector<CellPair> (*delaunay_pairs)(std::span<const Vec3>);
    std::optional<Vec3> (*circumcentre)(const std::array<Vec3, 4>&);  // exato, arredondado; nullopt se coplanares
    bool (*touches_boundary)(const PreparedDomain3&, const Box<3>&);  // Box<3>: generalização do Box2 atual
    CellClip3 (*clip_cell)(const LabelledPolyhedron3&, const PreparedDomain3&);
    BackendInfo (*info)();
};
```

- **`PreparedDomain3`:** o recorte precisa de estruturas do CGAL (malha `Epeck`, árvore AABB) que não podem
  atravessar o firewall. O handle opaco com `std::shared_ptr<const void>` guarda esse estado sem herança nem tipo do
  CGAL na API. **[R]** Ele é interno: não aparece na fachada pública.
- **`clip_cell`:** o backend valida a célula (fechada, sem auto-interseção), triangula as faces em leque a partir do
  centroide, recorta com `corefine_and_compute_intersection`, agrupa os triângulos por rótulo e canoniza a saída
  (P15a, achados 2–4).
- **Rótulos exatos [R]:** o P15a descobria a origem dos triângulos por tolerância (10⁻¹⁰ L), com ambiguidades em
  casos degenerados. O backend deve levar o rótulo pelo *visitor* da corefinement do CGAL, implementado como classe
  própria que satisfaz o conceito `PMPCorefinementVisitor` (composição, sem herdar do `Default_visitor`; AGENTS.md).
  Se o conceito exigir métodos demais para manter, a alternativa é o teste exato de coplanaridade com `Epeck`.

## 5. Construção da malha 3D (P16)

Passos de `build_mesh_3d`, na ordem, todos derivados do P15a:

1. **Ordem canônica** dos sítios (lexicográfica): independência da ordem de entrada, malha idêntica bit a bit (R18).
2. **Vizinhos:** `delaunay_pairs`.
3. **Célula convexa** por semiespaços, com três rótulos de plano por vértice (núcleo, double).
4. **Vértices canônicos:** um vértice sobre três bissetores da célula i vira `circumcentre(i, j, k, l)`, com os
   índices ordenados e cache por chave; quatro sítios coplanares deixam o vértice como está (contado).
5. **Fusão a 10⁻¹² L** (DEC-020), descarte de faces com largura 2A/(maior aresta) abaixo desse limite e inserção de
   vértices em "T" na aresta vizinha.
6. **Classificação:** a célula só é recortada se a sua caixa toca o contorno (`touches_boundary`).
7. **Recorte exato** das células de contorno (`clip_cell`).
8. **Montagem em fluxo:** a face interna (i, j) é gravada quando o owner i é processado, e a cópia do neighbour só
   confere área e contagem de peças. Só a fronteira de faces ainda sem par fica em memória, com as faces de contorno
   agrupadas por patch no fim. Faces internas em ordem triangular superior, patches contíguos (DEC-029).
9. **Verificação:** `check_invariants<3>` em todo teste de integração e no benchmark.

**Estatísticas (`BuildStats3D`):** as mesmas contagens do P15a, a saber:
- recortes, falhas e entradas inválidas;
- faces sem par e diferença de contagem de peças;
- vértices fundidos, vértices em "T" e vértices não canonizados;
- células partidas;
- tempos por fase.

Qualquer face sem par ou falha de recorte vira erro (`InterfaceNotConforming` / `BackendFailure`), como no 2D.

## 6. Custo do recorte e localidade

- **[F] P15a:** 1 a 2,3 ms por célula recortada, dominado pela cópia do domínio a cada chamada.
- **[F] P15 (medido em 30/09, protótipo P15a, variável `P15A_SHARED_DOMAIN`):** compartilhar o domínio com
  `do_not_modify(true)` em vez de copiá-lo ficou de 22 a 142 vezes mais lento, e piorou com o número de células:

  | Caso | Cópia por célula | Domínio compartilhado |
  |---|---|---|
  | cubo, 2000 sítios | 0,99 ms | 95 ms |
  | icosfera, 2000 sítios | 2,27 ms | 51 ms |
  | estrela, 150 sítios | 2,33 ms | 59 ms |
  | estrela, 3000 sítios | 1,76 ms | 250 ms |

  A cópia continua sendo o melhor caminho conhecido.
- **[R] Para domínios grandes (esfera fina, STL no P17):** recortar contra um **pedaço local fechado** do domínio. O
  domínio é dividido uma vez em blocos sobrepostos, cada um fechado pelo corte com a sua caixa, e cada célula usa o
  bloco que contém a sua caixa. Isso limita o custo pelo tamanho do bloco, não do domínio. Medir no P16 com uma
  icosfera de 20 480 triângulos antes de adotar.

## 7. Células partidas (DEC-035)

Em domínio não convexo, a célula convexa pode atravessar duas partes do domínio (P15a: 46 de 3·10⁴ células na
estrela). A célula fica **inteira**, com todos os pedaços, com um aviso no log e a contagem em
`BuildStats::fragmented_cells`, como o 2D já faz. Os invariantes continuam valendo. **[R]** Listar essas células no
relatório de qualidade (2D e 3D), para o usuário refinar os sítios onde elas aparecem.

## 8. Sítios 3D

- `SiteSetD<D>` generaliza o `SiteSet` atual; `SiteSet` segue sendo o 2D (API 0.1).
- **Fontes 3D (P16):** aleatória uniforme com espaçamento mínimo, contagem, grade cartesiana e sítios explícitos. A
  pertinência ao domínio usa o número de enrolamento generalizado em double: serve só para gerar sítios, nunca para
  a topologia (a mesma regra do 2D para `contains`).
- **Espaçamento variável e pares de interface** (DEC-028 E2): P18, junto com a multirregião.

## 9. Saída

- **Formato nativo:** já genérico em D.
- **VTK XML [R]:** `write_vtu(Mesh3D)` com células `VTK_POLYHEDRON` (tipo 42), usando `faces` e `faceoffsets` do
  VTK XML, e os mesmos campos por célula do 2D (região, meio, sítio de entrada, volume, razão de aspecto, maior não
  ortogonalidade).

## 10. Memória e paralelismo

- **Meta do P04:** 4 KB por célula no pico. O P15a ficou em 6,6 KB porque guardava todas as células antes da
  montagem. **[I]** A montagem em fluxo (§5, passo 8) e os vértices em grade de fusão única devem levar para perto de
  1,5 KB por célula (a malha em si ocupa cerca de 1 KB por célula em 3D, P05a.3).
- **Paralelismo:** adiado pelo João para uma etapa posterior (P14 §6). O desenho mantém as células independentes
  (passos 3–7); o cache de circuncentros e a grade de fusão são os dois pontos que vão precisar de versão
  concorrente.

## 11. Testes e benchmark

- **GTest por classe** (R1): `TriangleSurface`, cada forma 3D, `Declaration3D`, `Partition3D`, o validador,
  `SiteSetD<3>`, cada fonte 3D, o backend 3D (cada função do struct) e `build_mesh_3d`.
- **Integração:** os oito casos do P15a viram `vmm/tests/integration/Voronoi3D`, com os mesmos critérios, mais o
  escritor `.vtu` 3D e a ida e volta pelo formato nativo.
- **Benchmark [R]:** caso B4 (cubo, 10⁶ sítios) e B5 (esfera fina, 10⁵ sítios), com tempo por fase e pico de
  memória, junto de B1–B3 (DEC-021).
- **Galeria:** um exemplo 3D (cubo e prisma em L) só depois dos testes (R30).

## 12. Roteiro ajustado da Fase 3

| Etapa | Entrega | Escopo |
|---|---|---|
| P16 | 0.3 (entrega b) | uma região, domínio analítico (caixa, esfera, cilindro, extrusão), sítios 3D, `build_mesh_3d`, VTU 3D, testes e benchmark B4/B5 |
| P17 | 0.4 (entrega c) | domínio STL: leitura, reparo e validação, patches por rótulo, blocos locais para o recorte |
| P18 | 0.5 (entrega d) | multirregião por precedência (booleanas exatas do PMP), interfaces conformes, junções triplas, âncora A3 |
| P19 | — | documentação e release 3D |

**[I] Maior risco da Fase 3: P18.** No 2D, a interface conforme veio do refinamento comum com topologia por
rótulos (DEC-028). Em 3D, as faces das duas regiões que tocam uma superfície de interface precisam ser refinadas em
comum, o que é uma sobreposição de polígonos sobre superfícies trianguladas. **[R]** Uma prova curta (P18a),
como foi o P15a, antes de fixar o desenho.

## 13. Pendências para o João

1. **DEC-036:** aprovar o domínio 3D como superfícies trianguladas fechadas com patch por triângulo.
2. **Prazo do P16** (o plano pede um prazo por etapa).
3. **P18a:** aceitar uma prova curta da interface 3D conforme antes do P18.

## 14. Rastreabilidade: requisito → módulo 3D

| Requisito | Onde |
|---|---|
| R3 sem herança, sem enum de despacho | `Backend3D` como struct de funções; *visitor* do CGAL por composição |
| R5/R6 meios, regiões, interfaces conformes | `Declaration3D`, `Partition3D` (P16 com uma região; P18 multirregião) |
| R7 malha imutável | `Mesh<3>` existente |
| R9 adjacência para matrizes | `cell_adjacency`, `sparse_pattern` existentes |
| R16 invariantes | `check_invariants<3>` existente |
| R17 tolerâncias relativas | fusão a 10⁻¹² L; rótulos e lascas relativos a L |
| R18 determinismo | ordem canônica dos sítios; saída do backend canonizada |
| R19 firewall | `PreparedDomain3` opaco; CGAL só em `vmm_backend_cgal` |
| R27 benchmark | B4 e B5 |
| R28 saída | nativo 3D; VTU com `VTK_POLYHEDRON` |
