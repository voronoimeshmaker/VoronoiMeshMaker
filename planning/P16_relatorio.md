# P16 — Relatório: 3D com uma região e domínio analítico (entrega b)

- **Data:** 2026-09-30
- **Sequência:** `planning/sequencia_prompts.md`, Fase 3, P16; arquitetura em `planning/P15_arquitetura_3d.md`
- **Ambiente:** WSL `Ubuntu-26.04-Test`; g++ 15.2.0; CMake 4.2.3; CGAL 6.2.1; Boost 1.92; Release com LTO,
  `-march=native` (DEC-034) e `-ffp-contract=off` (DEC-037); 1 thread.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. O que foi entregue

| Módulo | Arquivos | Conteúdo |
|---|---|---|
| geometry | `surface.hpp/.cpp` | `Box3`, `TriangleSurface` (fechada, orientação consistente, reorientada para fora; área, volume, componentes, pertinência por número de enrolamento, distância) |
| domain | `shapes3d`, `declaration3d`, `partition3d` | `Cuboid`, `Sphere`, `Cylinder`, `Extrusion` (contorno 2D sem buracos), `SurfaceShape`, `ShapeRegistry3D`; `Declaration3D`; `Partition3D` (triângulos com região de cada lado e patch) |
| sites | `sources3d` | `SiteSet3D`; fontes `UniformRandomSource3D`, `RandomCountSource3D`, `CartesianGridSource3D`, `ExplicitSites3D`; `generate_sites_3d`, `validate_sites_3d` |
| backend | `backend3d.hpp`, `src/backend/cgal/backend3d.cpp` | `Backend3D`, `cgal_backend_3d()`: partição com teste exato de auto-interseção, Delaunay 3D, circuncentro, teste de contorno (AABB), recorte exato rotulado |
| voronoi | `builder3d` | `build_mesh_3d` (passos do P15 §5, montagem em fluxo), `invariant_reference(Partition3D)` |
| io | `vtu` | `write_vtu(Mesh3D)` com `VTK_POLYHEDRON` |
| fachada | `vmm.hpp` | `MeshRequest3D`, `MeshResult3D`, `generate_mesh_3d` |
| error | `error_code.hpp`, `catalog.cpp` | `InvalidSurface` (211), textos pt/en |

Também entraram o exemplo `ex_voronoi3d` na galeria, com figura em corte, a página `guide/mesh3d.rst` em português e
inglês e os casos B4 e B5 no benchmark.

**Desvios do P15 [F]:**
- `SiteSet3D` é uma classe própria, não `SiteSetD<D>`: assim o `SiteSet` 2D e o seu teste ficam intocados (API 0.1).
- `Extrusion` não aceita buracos na 0.3 (erro `InvalidShapeParameter`); buracos 3D entram com a precedência, no P18.
- Os rótulos exatos do recorte usam o *visitor* da corefinement, como o P15 recomendou: uma classe própria com os
  métodos do conceito do CGAL, sem herança. Nenhum triângulo ficou sem rótulo nos testes.

## 2. Testes e cobertura

- **[F] 224/224 testes** em Debug, em Release e no build de cobertura. São 69 testes novos: um arquivo GTest por
  classe pública nova, o teste do backend 3D, o do construtor 3D e os casos 3D do escritor VTU e do verificador de
  invariantes.
- **[F] Integração (`tests/integration/Voronoi3D`):** os casos do P15a pela API da biblioteca, cada um com os
  invariantes da DEC-011:
  - cubo, esfera, prisma em L, cunha de 3° e estrela (células partidas ficam inteiras, DEC-035);
  - grade cartesiana cosférica e sítios a 10⁻⁹ L do contorno;
  - malha idêntica bit a bit para outra ordem dos sítios e mesma topologia nas escalas 10⁻³ e 10⁶;
  - fachada, ida e volta pelo formato nativo e escritor VTU.
- **[F] Cobertura (R25):** 97,4% das linhas e 91,8% dos ramos no total, nenhum arquivo abaixo do mínimo. Os arquivos
  3D: `builder3d.cpp` 96%/93%, `backend3d.cpp` 92%/90%, `shapes3d.cpp` 99%/89%, `surface.cpp` 99%/94%,
  `sources3d.cpp` 100%/93%.
- **[F] Verificação estática** (`check_requirements.py`): nenhum problema.
- **[F] Documentação:** gerada em português e inglês, com o portão de testes (224/224) e a figura 3D da galeria.

## 3. Desempenho (Release, 1 thread)

| Caso | Células | Construção | Delaunay | Células convexas | Recorte exato | Memória no pico | Invariantes |
|---|---|---|---|---|---|---|---|
| B4, cubo | 10⁶ | 212 s | 5 s | 137 s | 57 s (5,4% recortadas) | 1379 B/célula | PASS |
| B5, esfera (1280 triângulos) | 10⁵ | 44 s | 0,5 s | 16 s | 27 s (11,5% recortadas) | 1546 B/célula | PASS |

- **Metas do P04 (3D):** 10⁶ células em até 10 min (atingido: 3,5 min) e até 4 KB por célula (atingido: 1,4 KB).
  O P15a gastava 6,6 KB por célula; a montagem em fluxo resolveu.
- **Circuncentros:** o valor é tirado do intervalo do número preguiçoso do CGAL, com avaliação exata só quando o
  intervalo é largo. Isso cortou 37% da fase de células convexas (medido em 10⁵ células), com a mesma malha
  (mesmo checksum).
- **2D sem regressão:** B1 5,6 s, B2 0,34 s, B3 8,4 s, com os mesmos checksums de antes.
- **[I] Onde está o tempo:** a fase de células convexas (65% em B4) é dominada pela construção dos circuncentros no
  núcleo preguiçoso do CGAL; o recorte exato custa cerca de 1 a 2,3 ms por célula recortada (cópia do domínio,
  P15 §6). A paralelização, adiada pelo João, é o ganho natural nas duas fases.

## 4. Achados

1. **[F] FMA muda decisões geométricas (DEC-037, proposta).** Em Release, com `-march=native`, o GCC funde
   `a*b + c` e um teste de colinearidade que vale exatamente zero mudou de sinal; a triangulação de uma extrusão na
   escala 10⁻³ falhou só nesse build. Com `-ffp-contract=off` os resultados voltaram a coincidir entre Debug e
   Release. O custo foi de cerca de 1% (B4: de 209,8 s para 211,8 s), com os mesmos checksums.
2. **[F] Faces de contorno por triângulo do domínio.** Uma célula encostada numa face plana do cubo recebe um pedaço
   por triângulo do domínio (duas faces coplanares, mesmo patch). A malha é válida e os invariantes passam. **[R]**
   Fundir os pedaços coplanares de mesmo patch numa face só, se o solver preferir.
3. **[F] Custo dos testes em Debug.** O CGAL sem otimização é lento: os testes 3D de integração somam cerca de 1 min
   em Debug (não cronometrei o Release); o teste de 1000 configurações 2D já levava 635 s em Debug antes do P16.
4. **[F] Script de cobertura.** O `gcovr` percorria a raiz do repositório e lia arquivos `.gcda` antigos da pasta
   `build/` (fora do git). Agora procura só no diretório do build.

## 5. Pendências para o João

1. **DEC-037:** aprovar a compilação sem contração FMA.
2. **Numeração:** a DEC-025 chama esta entrega de 0.3, mas a 0.2 (escritores MODFLOW, PFLOTRAN e TOUGH) não foi
   feita. O pacote continua 0.1.0; o `CHANGELOG` registra o 3D como "não publicado".
3. **Facilidade de uso:** decidir entre a função auxiliar que troca o `if (!result)` por uma exceção com mensagem
   clara, o executável com arquivo de configuração e os bindings em Python (nova dependência, precisa de uma DEC).
4. **Próxima etapa:** P17 (domínio STL, entrega c) ou a prova curta da interface 3D (P18a) antes do P18.
