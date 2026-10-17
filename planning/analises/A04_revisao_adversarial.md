# A04 — Revisão adversarial do VoronoiMeshMaker (estado 1.1.0 preparado)

- **Data:** 2026-10-16
- **Commit analisado:** `fb02bb7` ("revisao", 2026-10-08), branch `main`, clone do GitHub `voronoimeshmaker/VoronoiMeshMaker`. Árvore limpa (`git status` vazio antes e depois dos testes).
- **Revisor:** independente. Não li nenhuma análise de terceiros; li só `vmm/`, `planning/`, `.github/`, o build e os meus próprios arquivos em `adv/`.
- **Convenção:** **[F]** fato verificado (arquivo:linha, comando ou log) · **[I]** inferência · **[R]** recomendação.

## 0. Ambiente, o que executei e o que não executei

**Ambiente (não é o WSL `Ubuntu-26.04-Test` do AGENTS.md; pelo próprio AGENTS.md, nada aqui vale como evidência da árvore local do João, só do commit `fb02bb7`):**
Ubuntu 24.04.5 LTS, Intel Xeon 2,8 GHz, 2 núcleos, 7 GB; g++-14 14.2.0; CGAL 6.2.1 (instalado do tarball); Boost 1.83.0; GTest 1.14 (Ubuntu); CMake 4.4.4 no build e 3.28.3 do sistema para o teste da versão mínima; Ninja 1.11.1; Python 3.13; clang++-18 (o 19 não está instalado).
Build: `build-gcc14`, Release, LTO e `-march=native` ligados (padrão da DEC-034), `BUILD EXIT 0 in 2298s`, 0 avisos do compilador no `build.log` (só as notas do `lto-wrapper`).

**Executado [F]:**
1. `ctest --test-dir build-gcc14 -L vmm -j 2 --output-on-failure --timeout 3600` → **324/324 aprovados**, 222 s de parede (23 com o rótulo `slow`, 392 s·proc). Log: `adv/ctest_full.log`.
2. `python3 -I vmm/vmm/tools/ci/check_requirements.py --root vmm/vmm` → `0 problem(s)`.
3. `vmm_benchmark B1 1000000` → 10⁶ células, construção de 15,2 s, 16,3 s no total, pico de 642 MB (673 B/célula), invariantes PASS, checksum `30df5ed2ebaeaf4a`.
4. `vmm_benchmark B4 0 200000` → 2·10⁵ células 3D, construção de 140,5 s, 1515 B/célula, PASS, checksum `9802537f1663e195`.
5. O programa de sondagem `adv/src/probe.cpp`, ligado às bibliotecas estáticas do build (`g++-14 -std=c++23 -O2 -march=native -ffp-contract=off -flto=1 … libvmm_facade.a libvmm_io.a libvmm_backend_cgal.a libvmm_core.a -lgmp -lmpfr`). Modos: `utm`, `ortho`, `aligned`, `lscale k camadas dem`, `cvtdom`, `cvt2d`, `meander`, `dupes`, `utmal`, `hairpin`, `bomb`, `io`.
6. `vmm-mesh` com configurações próprias em `adv/cfg/` (dique 2D e 3D, grades 3D em esfera, cubo e duas camadas) e o exemplo da galeria `cvt_horizons.cfg`.
7. Configuração com o CMake 3.28.3 do sistema (`adv/cfg328`) e controle com o 4.4.4 (`adv/cfg444`).
8. `clang++-18 -std=c++23` com `<expected>`, para conferir a justificativa da DEC-042.
9. `git log`, `git diff da5c03e fb02bb7 -- vmm/include` e `git ls-remote --tags origin` (retornou vazio, código 0).

**Não executado:**
- cobertura com gcovr;
- sanitizadores (ASan/UBSan);
- Clang 19, que não está instalado;
- build da documentação (Doxygen/Sphinx);
- Debug;
- B4 com 10⁶ células (tempo);
- a malha em colunas acima de 8100 colunas (tempo).

**Não consegui verificar:** o estado da CI do GitHub para `fb02bb7`. A API do GitHub respondeu `403 — GitHub access to this repository is not enabled for this session`, então não afirmo se a CI passou ou falhou.

**Ataques que não acharam defeito (só para contexto):**
- 2D em coordenadas UTM (origem 5·10⁵, 7,5·10⁶; lados de 100 m, 1 km e 10 km): invariantes PASS.
- Malha em colunas em UTM, inclusive com declive de 0,5: geração OK.
- Grades cartesianas 3D (cubo com 6859 células, esfera com 3887, duas regiões): PASS.
- Interfaces fixas no CVT com o domínio extraído das colunas: a partição não tem triângulos de área zero, e o CVT aceitou os 3 passos.
- O Clang 18 com a libstdc++ 14 de fato não tem `std::expected`, como a DEC-042 diz.

---

## 1. Objeções por severidade

### BLOQUEANTE

#### B1. A geração em colunas (o recurso central da 1.1 para o MohidNG) tem custo quase quadrático no número de colunas e também cresce com a resolução do MDT
- **Evidência [F]:**
  - `vmm/src/backend/cgal/column_overlay.cpp:49-65`: `locate` aloca `std::vector<int> winding(columns)`, percorre **todas** as arestas da base e depois todas as colunas, **para cada triângulo** do refinamento (`:69-77`).
  - `:114-118`: para cada triângulo de saída e cada lado, percorre todas as arestas de contorno com `Segment_2::has_on` exato.
  - Medido com `./probe lscale k 10 1` (base cartesiana, MDT 1×1, 10 camadas):

    | Colunas | Geração (s) |
    |---:|---:|
    | 1024 | 1,60 |
    | 4096 | 19,37 |
    | 8100 | 70,25 |

    O expoente fica em ≈1,9.
  - A resolução do MDT também pesa (`./probe utmal`): 1024 colunas com MDT de 2,5 m (160×160) levaram 31,97 s e geraram **112 960 faces** (110 por célula). Com MDT de 11×11 sobre 2823 colunas, 36,6 s.
- **Cenário de falha [I]:** um estuário com 10⁵ colunas leva ≈70·(10⁵/8100)^1,9 ≈ 2,3 h só na classificação; com 10⁶ colunas, dias. Isso vale também para o `vmm-mesh` com `horizons = ...`. A página `horizons.rst` diz só que a classificação "pode dominar o custo em malhas grandes"; o relatório P27 mediu até 31 752 células.
- **Por que importa:** o MohidNG "tem muitas colunas". A meta de 10⁵–10⁷ células é inatingível por ordens de grandeza, e o 2D/3D Voronoi atinge as metas da DEC-020 (B1 em 16 s aqui). A 1.1 entrega uma API estável cujo custo inviabiliza o uso pretendido.
- **Correção [R]:**
  - classificar os triângulos por propagação na própria CDT: busca em largura a partir das arestas restritas, com o lado de cada aresta da base dando a coluna;
  - ou indexar as arestas por caixas ou grade, para localizar em O(log n);
  - levar o patch como informação da restrição (`insert_constraint` com id), não por busca;
  - inserir na CDT só as células do MDT que cruzam a caixa de cada coluna;
  - acrescentar um caso de colunas ao `vmm_benchmark` com meta de tempo (hoje só há o script do protótipo).

#### B2. A 1.1 congela API e formatos novos sem decisão registrada; um header que se diz "interno" passa a ser estável pela regra da DEC-041
- **Evidência [F]:**
  - `planning/DECISIONS.md` vai até a DEC-042 (último commit em 2026-09-30). `grep -i "horizon|cvt|vlayers"` acha só a menção da DEC-033:484 ("o otimizador de Lloyd … fica para a 0.2").
  - Não há DEC para `HorizonGrid`, região = região da base × intervalo, o formato `VMM_LAYERS 1`/`.vlayers`, o `VMM_HORIZONS 1`/`.hgrid`, as chaves `horizons`/`cvt_*`, o Lloyd com densidade uniforme e a reconstrução completa, nem para a "autorização para executar todas as etapas sem aprovação" (`planning/horizontes/00_sequencia.md:17-19`).
  - A DEC-041 (`DECISIONS.md:578-604`) e `vmm/docs/api.rst:11-20` dizem que é estável tudo o que está em `include/vmm`, exceto `backend2d.hpp`/`backend3d.hpp`, `vmm::detail` e os contadores de `BuildStats`.
  - `include/vmm/backend/column_overlay.hpp:16,34` diz "Internal backend transport" e "Internal API", mas não entra em nenhuma das duas listas. Pela regra vigente, `cgal_column_overlay`, `ColumnEdge`, `ColumnTriangle` e `ColumnOverlay` ficam congelados na série 1.x.
  - Também congelam:
    - os campos públicos de `LayeredData` (`mesh/layered.hpp`: `std::vector<std::size_t> column, layer, face_level`, sentinela `lateral_face = SIZE_MAX`);
    - `CvtReport` e `CvtOptions`;
    - as chaves `cvt_*`;
    - a semântica de `Mesh3D::site` nas malhas em colunas (ver B3).
  - A estabilidade do `.vlayers` não está declarada em lugar nenhum (a DEC-041 só cita o `.vmesh` v1).
- **Cenário de falha:** publicada a 1.1, corrigir B1 ou B3 (por exemplo, trocar `std::size_t` por `Id<ColumnTag>`, guardar o gerador da base ou mudar o transporte do overlay) vira quebra de API, que só é permitida na 2.0.
- **Correção [R]:** antes de qualquer tag, registrar as DEC da 1.1 (horizontes, `.vlayers`, CVT, chaves de configuração), incluindo explicitamente na lista de internos `column_overlay.hpp`, ou movê-lo para `src/` ou `vmm::detail`. Avaliar publicar colunas e CVT em `vmm::experimental` ou declará-los "sem garantia na 1.1", como foi feito com o backend na 0.x.

#### B3. A malha em colunas descarta a ortogonalidade da base e reaproveita `Mesh3D` com outro significado para `site` e `cell_input_site`
- **Evidência [F]:**
  - `src/facade/layered.cpp:105`: `md.sites.push_back((1/volume[c])*moment[c])` (o "sítio" é o centroide).
  - `:109`: `md.cell_input_site.push_back(SiteId::from_index(col))`, isto é, o índice da coluna, repetido em todas as camadas.
  - `LayeredData` não guarda o gerador 2D da base.
  - Medido (`./probe ortho`, base aleatória com 1058 colunas e horizontes planos): não ortogonalidade máxima da base 2D de 6,2·10⁻¹³ rad. Nas 3117 faces laterais internas da malha em colunas, **máxima de 0,269 rad e média de 0,054 rad**, com as métricas do próprio VMM.
  - O mesmo tipo `Mesh3D`, pela DEC-032 e por `invariants.cpp:66-75`, tem as faces internas de uma região "ortogonais por construção".
  - `cvt_energy(layered.mesh())` devolve 1,74·10⁹ sem recusar. O `@note` de `cvt.hpp` pede "actual Voronoi generators", mas nada impõe isso.
- **Cenário de falha [I]:** um solver TPFA (o do João ou o MohidNG) que use `site()` e `distance` como na malha Voronoi tem erro de fluxo O(θ) nas faces laterais, sem aviso. A face lateral é vertical e ortogonal à reta entre os geradores da base, mas o VMM entrega os centroides. Além disso, `check_invariants` aplica a essa malha o teste específico de Voronoi `dot(d, S) <= 0 → bad face` (`invariants.cpp:68-71`). Com células de pinch-out ou base fragmentada, uma malha geometricamente válida pode ser rejeitada.
- **Correção [R]:**
  - guardar por coluna o gerador da base, ou um ponto de referência por célula com rótulo explícito;
  - distinguir no tipo, ou num campo, uma malha "Voronoi" de uma "derivada";
  - documentar que `site` é o centroide nas colunas;
  - não aplicar o teste de orientação de Voronoi nesse caso;
  - fazer `cvt_energy` recusar malhas cujos sítios não sejam geradores.

  Isso tem de ser decidido antes do congelamento (B2).

### IMPORTANTE

#### I1. As consultas de colunas são O(N) por chamada (O(N²) no laço natural de um solver)
- **Evidência [F]:** `src/mesh/layered.cpp:123-129` (`cells_in_column` percorre todas as células e ordena) e `:130-138` (`vertical_neighbours` percorre todas as faces internas). Medido com 8100 colunas e 81 000 células:
  - `cells_in_column` para todas as colunas: 0,42 s;
  - `vertical_neighbours` em 200 células: 0,106 s, ou ≈43 s para todas.
- **Cenário [I]:** com 10⁶ células, ≈1,8 h para montar as colunas e ≈2 h para os vizinhos verticais; com 10⁷, dias.
- **Correção [R]:** CSR de coluna para células, montado em `from_data` (ordem por coluna e camada), e vizinhos verticais pelo `CellFaceIndex`. Devolver `std::span`.

#### I2. Explosão de faces: a "face horizontal" de uma coluna é um leque de triângulos e as laterais são partidas pela grade do MDT
- **Evidência [F]:**
  - `facade/layered.cpp:145-165`: cada triângulo do refinamento gera um topo, uma base e três laterais. Só as faces internas da mesma célula são canceladas (`:137`).
  - Medido: com horizontes planos, 4,00 faces de topo por coluna (`./probe ortho`).
  - Com MDT de 2,5 m sobre uma base de 12,5 m, 110 faces por célula (112 960 faces para 1024 células).
  - O P27 registrou 135 954 faces para 31 752 células.
- **Por que importa [I]:** um modelo em colunas espera uma face entre camadas por coluna (fluxo vertical, sigma). O custo de memória e de fluxo passa a depender da resolução do MDT, e não da malha. Faces coplanares vizinhas não são fundidas nem quando o horizonte é plano.
- **Correção [R]:**
  - fundir as faces coplanares com o mesmo par de células (sempre possível em horizonte plano por coluna);
  - oferecer o modo "um plano por coluna", com amostragem do horizonte no gerador ou no centroide da base, quando a fidelidade ao MDT não for exigida;
  - expor `faces_between(c, c_above)`.

#### I3. O refinamento comum falha quando o MDT está alinhado com uma base cartesiana de espaçamento não diádico
- **Evidência [F]:** `./probe lscale 10 1 10` e `./probe aligned 0|1e-15|1e-13|1e-9` (base com espaçamento 0,1 e grade do MDT com nós em k·0,1) falham com `[VMM-202] polígono inválido (menos de 3 vértices, auto-interseção ou coordenada não finita): rounded overlay triangle is degenerate: (0,0.2) (0.1,0.2) (0.09999999999999999,0.2)` (`facade/layered.cpp:69-74`). Controles:
  - espaçamentos diádicos (1/8, 1/16) e UTM com inteiros (10 m e 10 m, 20 m e 10 m) passam;
  - `./probe dupes` mostra 30 vértices da base com espaçamento 0,1 deslocados de k·h por alguns ulps (as arestas "verticais" da base não são exatamente verticais).
- **Cenário:** base com espaçamento de 0,1 m, 0,25° ou 0,001° e MDT na mesma grade, que é o caso mais comum de "malha alinhada ao raster", produz uma falha dura com uma mensagem enganosa (fala em polígono de menos de 3 vértices).
- **Correção [R]:**
  - arredondamento por snap rounding (`CGAL::Snap_rounding_2`), ou fusão dos pontos exatos a 10⁻¹² L antes da triangulação, como o 2D já faz;
  - teste de propriedade "base cartesiana × MDT com o mesmo espaçamento" para h ∈ {0,1; 0,3; 1/3};
  - código e mensagem de erro próprios.

#### I4. Obstáculo fino (dique, píer) com borda sobre um bissetor derruba a geração em 2D e 3D, mesmo com uma só região
- **Evidência [F]:**
  - `adv/cfg/dike_a.cfg` (rio em U com parede de 0,1 m, `sites = grid`, `spacing = 0.5`, `origin = 0.25 0.25`): `vmm-mesh: [VMM-402] non-conforming interface: 0 interface sub-intervals, 2 bisector pieces`.
  - `dike2d_hole.cfg` (buraco retangular de 0,1 de espessura): mesma falha, com 4 peças.
  - `dike3d.cfg` (laje-buraco de 0,1 numa caixa, grade de 0,5): `face (21, 22) seen by one cell only (cell 22)`.
  - Controles que passam: a mesma parede deslocada 0,01 (`dike2d_hole_shift`, `dike3d_shift`) ou outra origem da grade (`dike2d_hole_origin`).
  - A falha acontece exatamente quando a borda do obstáculo coincide com o bissetor de dois sítios em lados opostos.
- **Por que importa:** diques, espigões e paredes finas são comuns em rios e estuários, e grade cartesiana ou hexagonal com obstáculos alinhados é configuração natural. A DEC-028 diz "conformidade garantida por construção"; o teste de 1000 configurações (`tests/integration/RandomConfigurations`) só usa sítios Poisson (`UniformRandomSource`), círculos e retângulos, e nunca exerce esse caso. A R26 ("casos patológicos") é declarada OK no P14/P19.
- **Correção [R]:**
  - tratar a coincidência bissetor/segmento no recorte rotulado (empate resolvido por regra simbólica, com o rótulo do segmento prevalecendo);
  - incluir grades e obstáculos finos alinhados no teste de propriedade;
  - mensagem que não fale em "interface" quando há uma só região.

#### I5. O CVT não escala: reconstrução e validação completas a cada tentativa, verificação de suporte O(F_b·S) e passo global
- **Evidência [F]:**
  - `src/facade/cvt.cpp:96-117`: cada `build` chama `build_mesh_*`, `check_invariants` e `fixed_support`.
  - `:71-89` e `:47-70`: para cada face de contorno ou interface, cada vértice, ponto médio e centro, percorre **todos** os segmentos ou triângulos da partição.
  - `:156-180`: um único passo para todos os sítios; um sítio inadmissível divide o passo de todos por 2 (até 24 vezes), e cada tentativa é uma reconstrução completa.
  - Medido (`./probe cvt2d`):

    | Caso | Base (s) | CVT |
    |---|---:|---|
    | disco poligonal de 4000 lados, 2000 sítios | 0,99 | 3,29 s em 2 iterações |
    | disco poligonal de 16 000 lados, 2000 sítios | 6,37 | 18,25 s em 1 iteração |

  - B4 aqui: 140 s para 2·10⁵ células 3D. Uma iteração de CVT 3D custa pelo menos uma construção, então 50 iterações (o padrão) em 10⁶ células 3D levam horas.
  - O próprio benchmark (`planning/cvt/01_entrega.md`) vai só até 256 células 2D e 64 células 3D.
- **Cenário [I]:** um contorno de linha de costa com 10⁵ vértices e 10⁴ faces de contorno dá ~3·10⁹ testes por tentativa.
- **Correção [R]:** índice espacial (a `SegmentIndex2` e a AABB já existem) em `fixed_support`; passo por sítio, ou projeção do alvo na região; reuso da partição preparada entre as iterações; meta de tempo no `vmm_benchmark`.

#### I6. O CVT pelo `vmm-mesh` com `horizons` descarta a malha de entrada e recomeça de sítios aleatórios; com 0 iterações devolve uma malha Voronoi aleatória
- **Evidência [F]:**
  - `src/app/run.cpp:308-324`: os sítios vêm de `RandomCountSource3D(counts[i])` com `seed`, e nem os centroides nem os geradores da base são usados.
  - O exemplo da galeria `cvt_horizons.cfg` (8 iterações) termina com `converged false`.
  - Com `cvt_iterations = 0`, a saída é `0 accepted iterations`, energia 4,60·10⁻¹, isto é, um Voronoi 3D de sítios aleatórios e não a malha em colunas.
  - O resultado não tem colunas (documentado em `cvt.rst`).
  - Não existe caminho no CLI para "CVT da base 2D e depois extrusão", que é o que preserva as colunas e é o fluxo natural para o MohidNG. Na API em C++ ele é possível compondo `optimize_cvt(Partition2D)` e `generate_layered_mesh`, mas não é documentado.
- **Correção [R]:** no CLI com `horizons`, o padrão deveria otimizar a base 2D antes da extrusão. O CVT volumétrico ficaria opcional (`cvt_mode = volume`) e deveria partir dos centroides da malha de entrada. Documentar a composição na API.

#### I7. Células fragmentadas (DEC-035) passam em silêncio: o aviso é descartado e o contador é "interno"
- **Evidência [F]:**
  - `builder2d.cpp:325-329` registra o aviso por `log()`, mas `include/vmm/error/log.hpp:4` diz "Without a sink, messages are dropped", e nenhum código chama `set_log_sink` (`grep` em `tools/`, `src/` e `examples/`).
  - O contador `BuildStats::fragmented_cells` é declarado sem garantia pela DEC-041, enquanto a DEC-035 manda o usuário "refinar os sítios onde elas aparecem".
  - Medido (`./probe hairpin`, rio em U, grade de 0,75): **12 de 27 células fragmentadas**, `invariants passed=1`.
- **Cenário [I]:** em rios finos e meandrantes, uma célula com pedaços nas duas margens de um dique liga hidraulicamente as margens pelo seu volume. O solver não tem um meio estável de detectar isso.
- **Correção [R]:** expor `fragmented_cells` (ou a lista de células) como API estável; fazer o `vmm-mesh` instalar um sink para stderr; opção de falhar ao fragmentar (`BuildOptions::reject_fragmented`).

#### I8. A versão mínima declarada do CMake (3.28) não configura o projeto
- **Evidência [F]:** com `cmake 3.28.3` (o CMake de sistema do Ubuntu 24.04), `-S vmm -B adv/cfg328 -DCMAKE_PREFIX_PATH=cgal-inst`, a configuração falha em `cmake/VerifyCGALHeaders.cmake:48` (`check_cxx_source_compiles`): `Target "cmTC_…" links to: CGAL::CGAL but the target was not found` e `Configuring incomplete`. O mesmo comando com o CMake 4.4.4 termina em `Generating done`. `CMakeLists.txt:1` e `vmm/CMakeLists.txt:6` declaram `cmake_minimum_required(VERSION 3.28)`.
- **[I]** A CI usa o `cmake` do runner `ubuntu-24.04`, normalmente mais novo que o apt 3.28. Por isso a versão mínima nunca foi testada.
- **Correção [R]:** subir o mínimo para a versão realmente testada, ou corrigir a verificação (por exemplo, `try_compile` com `INCLUDE_DIRECTORIES` em vez de `CMAKE_REQUIRED_LIBRARIES` com um alvo importado ou alias); job de CI com o CMake mínimo.

#### I9. A CI não cobre o que a linha de base e as DEC prometem
- **Evidência [F]:**
  - `.github/workflows/vmm-ci.yml:23-26`: só gcc-14 (Debug, Release, sanitize) e clang-19 (Release), todos em `ubuntu-24.04`. A R12 (`P05_BASELINE.md:51`) e a DEC-022 dizem "CI com GCC e Clang nas versões mínimas e nas mais recentes"; não há GCC 15/16, Clang 20+, macOS nem Windows.
  - CGAL fixo em 6.2.1: "≥ 6.2" no README, mas a 6.2.0 nunca foi testada.
  - O job com sanitizadores pula os 23 testes `slow` (`:65`).
  - `vmm-docs.yml:44` instala o Sphinx e companhia **sem fixar versões**, e o build usa `-E -W`.
  - A evidência da 1.1 com Clang e sanitizadores nos relatórios é parcial: P27 "Clang19 … 25/25 testes dirigidos", "ASan/UBSan … 25/25 dirigidos"; CVT "Debug ASan/UBSan … CVT 13/13", "CVT Clang 19 13/13".
  - Não consegui ver o resultado da CI para `fb02bb7` (403).
- **Correção [R]:** matriz com o mais recente de cada compilador; um job com CMake e CGAL mínimos; fixar as dependências Python; rodar a suíte completa com sanitizadores pelo menos num agendamento noturno.

#### I10. DEC-034 (LTO e `-march=native` por padrão) sem ganho medido, contradizendo a R24 e prejudicando a distribuição
- **Evidência [F]:**
  - `vmm/CMakeLists.txt:19-20` liga as duas opções.
  - A R24 (`P05_BASELINE.md:73`) diz "NATIVE_ARCH e LTO desligados por padrão" e nunca foi revisada, apesar da regra da própria linha de base ("só muda por nova versão deste arquivo").
  - P07 §4 (`P07_infra.md:80`): "Ganho de até ~5 % no B1 e nenhum no B3, dentro da variação entre execuções".
  - A DEC-034 admite que as estáticas carregam IR do GCC ("quem as instala e liga precisa do mesmo compilador com LTO"). Confirmei que as `.a` exigem `-flto` na ligação: o meu programa só ligou com `-flto=1`.
- **Cenário [I]:** um MohidNG em outro compilador ou versão (ou com parte em Fortran) que faça `find_package(VoronoiMeshMaker)` contra um pacote instalado com os padrões não liga; um binário levado para outro nó do cluster morre com "illegal instruction".
- **Correção [R]:** padrão portátil (OFF/OFF) e as opções rápidas opt-in, ou ao menos `install()` recusando o pacote com LTO ou native sem uma confirmação explícita.

#### I11. Versionamento sem âncora: nenhuma tag; `main` já é "1.1.0, não publicada"
- **Evidência [F]:**
  - `git tag` e `git ls-remote --tags origin` vazios.
  - `vmm/CHANGELOG.md` lista "1.0.0 — 2026-09-30" e "0.2.0 — 2026-09-30".
  - `CMakeLists.txt:8` e `core/version.hpp` dizem 1.1.0, enquanto o README (`README.md:42`) diz "A versão 1.1 preparada na árvore de trabalho". O P20 §7 pedia `git tag -a v1.0.0`, que não existe.
  - O 3D foi publicado como 0.2.0 e congelado como 1.0.0 no mesmo dia, sem nenhum usuário externo. A DEC-025 previa "nenhuma quebra de API entre 0.5 e 1.0", e esse período foi de zero dias.
- **Cenário:** um consumidor que clone `main` hoje recebe "1.1.0", que segundo o próprio projeto não foi publicada; não há como fixar a 1.0.0 por tag.
- **Correção [R]:** marcar `v1.0.0` no commit correto (identificá-lo e registrá-lo); voltar `main` para `1.1.0-dev`, ou pré-release, até a 1.1 ser aprovada.

#### I12. Relatórios desatualizados e contraditórios com o código e o histórico
Ver a tabela da §2. O principal: `planning/horizontes/relatorios/P27_validacao.md:41` ("CVT, que não está implementado"), `P29_entrega.md:10` ("Não foi implementado CVT") e `horizontes/00_sequencia.md:97` ("T01/CVT permanece futuro") convivem com `optimize_cvt` em `HEAD`. Vários relatórios dizem "sem commit, tag ou push", mas tudo está em `main` (8191e26, e866c96, 2a8110e, 42750c8).
- **Por que importa:** `planning/` é declarado fonte de verdade, e hoje um leitor não consegue saber o estado real a partir dele.
- **Correção [R]:** uma seção "Estado em <commit>" no topo de cada sequência, e o relatório final da 1.1 reconciliando P21–P29 e C01–C05.

#### I13. Evidência não reprodutível nem auditável
- **Evidência [F]:**
  - Todos os logs citados (`build/horizons_full_tests.log`, `build/cvt_final_tests.log` e outros) ficam em `build/`, fora do git.
  - A cobertura da 1.1 combina execuções de estados diferentes da fonte: `01_entrega.md` diz "a suíte completa anterior mais a execução atualizada dos 13 casos de CVT", e `P27_continuacao_0959.md` pede para não citar aqueles números como cobertura final.
  - Os relatórios afirmam "mesmos checksums" (P16:60, P17:53, P18:71-77), mas **nenhum valor de checksum aparece em `planning/`** (`grep` sem resultado). Obtive aqui B1 = `30df5ed2ebaeaf4a` e B4(2·10⁵) = `9802537f1663e195`, e ninguém consegue comparar com os do WSL.
  - O AGENTS.md restringe a evidência a uma única distribuição WSL de uma máquina. As sequências da 1.1 rodaram de forma "autônoma, sem aprovação intermediária".
  - As mensagens de commit não informam o conteúdo ("Primeiras correções" com +1887 linhas; "Correções", "atualização", "revisao").
- **Correção [R]:**
  - versionar resumos de logs, ou artefatos da CI, por commit;
  - guardar os checksums esperados num arquivo do repositório e testá-los (com tolerância de plataforma documentada);
  - cobertura de uma única execução limpa;
  - mensagens de commit com o escopo e a sequência (P2x/C0x).

#### I14. `read_native` deixa escapar `std::bad_alloc` com um arquivo malformado
- **Evidência [F]:** `src/io/native.cpp:200` (`d.points.resize((*n)[0])` com o valor lido do arquivo). `./probe bomb` com `points 4000000000000` resulta em `std::exception escaped read_native: std::bad_alloc`.
- **Por que importa:** a R11 e a DEC-016 exigem falhas previsíveis por `Result`. Um `.vmesh` truncado ou corrompido derruba o processo consumidor, e a assinatura não avisa. O `read_layered` cresce por `push_back` e não tem esse problema.
- **Correção [R]:** limitar as contagens ao tamanho do fluxo restante, ou reservar de forma incremental; teste negativo com contagem gigante.

#### I15. Testes que executam mais do que verificam, e lacunas de escala e degenerescência
- **Evidência [F]:**
  - Os testes de CVT (`tests/integration/Cvt/ut_Cvt.cpp`) usam de 1 a 4 sítios; nenhum testa a qualidade da malha otimizada, um número realista de sítios ou o custo.
  - O teste de 1000 configurações só usa sítios Poisson (I4).
  - Os golden files (`tests/data/golden/O1–O4`) cobrem só o 2D com uma região, gerados pela VMMLib e congelados (DEC-033); não protegem o 3D, a multirregião, as colunas nem o CVT.
  - Não há teste de desempenho com limite.
  - O maior caso de colunas nos testes tem dezenas de colunas.
  - As permutações de sítios (R18) não são testadas nas colunas nem no CVT.
- **[I]** O B1 (meta de 10⁶ em 30 s) passa aqui (16 s), mas nada impede uma regressão quadrática como B1/I1/I5 de entrar sem alarme.
- **Correção [R]:**
  - testes de propriedade com grades, obstáculos finos, MDT alinhado e coordenadas UTM;
  - golden files do próprio VMM, regeneráveis e versionados (2D multirregião, 3D, colunas), com checksum;
  - `ctest -L perf` com tempo máximo por caso de escala média (10⁴–10⁵).

#### I16. Licença efetiva GPL para quem gera malhas
- **Evidência [F]:** `vmm/CMakeLists.txt:124-126`: `vmm_facade` (alvo `VoronoiMeshMaker::vmm`) liga `vmm_backend_cgal` como `PUBLIC`, e esse backend é GPL-3.0-or-later (`column_overlay.cpp:1`). Não há backend alternativo (a DEC-041 diz "backends de terceiros não são suportados", o que contradiz o espírito da DEC-002).
- **[I]** Qualquer executável que gere malha (o MohidNG, se ligar o VMM) fica sujeito à GPL. O "núcleo BSD" sozinho não gera malha.
- **Correção [R]:** deixar isso explícito no README ("uso prático = GPL-3.0") e verificar a compatibilidade com a licença do MohidNG antes de integrar.

#### I17. Lacunas de produto para escoamento ambiental e MohidNG
**[I]**, a partir do código e da documentação:
- **Coordenadas:** não há suporte a coordenadas geográficas nem metadados de CRS no `.vmesh`/`.vlayers`; áreas e volumes são planos.
- **Camadas z-level:** não há camadas cartesianas com corte pelo fundo. Só há frações entre horizontes, que equivalem a sigma por intervalo; z-level exige o usuário pré-recortar os horizontes com max(fundo, z_k).
- **Espessura mínima:** não há controle de espessura mínima nem fusão de células finas em pinch-out (o P29 admite células "muito finas"); isso afeta a CFL e o wetting/drying.
- **Paralelismo:** não há partição de domínio (METIS ou outra) nem saída distribuída; a geração é de 1 thread.
- **Formato e tamanho:** o formato nativo é só ASCII. Medido: 10⁶ células 2D dão 220,8 MB, com escrita em 2,9 s e leitura em 3,3 s. [I] Em 3D com 10⁷ células, dezenas de GB. Não há HDF5, NetCDF nem UGRID.
- **Solo + rio:** não há colagem conforme entre uma malha Voronoi 3D (solo) e uma em colunas (rio).
- **Anisotropia:** não há malha anisotrópica ao longo do canal.
- **Correção [R]:** levantar com o MohidNG quais desses pontos são requisitos da 1.x e registrar as DEC correspondentes; os mais baratos e urgentes são espessura mínima, CRS como metadado e saída binária.

### MENOR

#### M1. O verificador de R1/R4 não enxerga os headers da raiz de `include/vmm`
- `check_requirements.py:78-79` pula os headers do topo (`if module.endswith(".hpp"): continue`).
- O R4 só casa `#include <vmm/(\w+)/` (`:96-103`). Por isso, `include/vmm/app/run.hpp:24,27` e `src/app/run.cpp:34` incluem `vmm/cvt.hpp`, `vmm/vmm.hpp` e `vmm/layered.hpp` (fachada) sem acusação, embora a tabela `ALLOWED_DEPS["app"]` não inclua `facade` nem `backend`.
- A afirmação "R4 OK" vale só para os subdiretórios. **[R]** Mapear os headers da raiz para "facade" no verificador.

#### M2. Estilo inconsistente no código da 1.1
- `cvt.hpp`, `cvt.cpp`, `cvt_domain.cpp`, `layered.*` e `column_overlay.*` não têm o bloco de cabeçalho `File/Description` que o resto do código tem (compare com `invariants.cpp:1-5`).
- Usam formatação comprimida (`a=b`, `if(...)` numa linha, testes dentro e fora do namespace anônimo em `ut_Cvt.cpp:95-97`).
- A documentação de `layered.hpp` usa "Parameters:/Notes:" em texto, em vez de `@param`.
- Os IDs crus são `std::size_t`, quando o projeto tem `Id<Tag>` (`core/types.hpp`), e há um sentinela `SIZE_MAX`.
- [R] Aplicar o mesmo `clang-format` e a mesma convenção de IDs antes do congelamento (B2).

#### M3. Tolerâncias divergentes
- `cvt.cpp:73` usa `1e-10*L`; `mesh/layered.cpp:50` e o builder usam `1e-12*L` (DEC-020).
- Nenhuma tolerância considera o módulo das coordenadas: em UTM, o ulp de 7,5·10⁶ é ~9·10⁻¹⁰ m. Medido: não ortogonalidade "de arredondamento" de 7·10⁻⁶ rad em UTM com 100 m, contra 2,5·10⁻¹¹ na origem. Com o `QualityLimits::max_nonorthogonality = 1e-8` padrão, `quality_report` acusaria violações falsas.
- [R] Tolerâncias derivadas de max(L, |x|·ε), documentadas.

#### M4. A DEC-028 contradiz a implementação
O texto da DEC-028 diz "vértices recalculados … nunca por proximidade" (`DECISIONS.md:403`), mas `builder2d.cpp:455-490` funde vértices por proximidade (agrupamento guloso numa grade hash), e o próprio comentário diz "points closer than the tolerance become one vertex". O agrupamento guloso não é transitivo. [R] Corrigir o texto ou a implementação, com teste de cadeia a–b–c.

#### M5. A DEC-032 exagera o que é exato no 3D
A DEC-032 atribui a ortogonalidade ao "backend exato", mas no 3D as células convexas interiores são construídas "por semiespaços … (núcleo, double)" (`P15_arquitetura_3d.md:108`), e o P18 §5 registra células inválidas em double com degenerescência maciça. Nos meus testes as grades 3D passaram; a afirmação deveria ser "topologia exata, construção em double verificada".

#### M6. Determinismo entre plataformas (R18) não verificado
Só há Linux e libstdc++. As formas curvas usam `std::sin`/`std::cos` (`shapes.cpp:164-182`), que não têm arredondamento garantido entre libms. Os checksums não estão registrados (I13). [R] Polígonos de círculo com tabela de ângulos racionais, ou registro de que a R18 vale só por plataforma.

#### M7. Incorporação por `add_subdirectory`/FetchContent é hostil
- `cmake/ConfigOptions.cmake` força `CMAKE_BUILD_TYPE` no cache com `FORCE`.
- `VMM_BUILD_TESTS=ON` por padrão exige GTest (`find_package(GTest 1.14 REQUIRED)`).
- `VMM_WERROR=ON` por padrão quebra o build do consumidor com compiladores mais novos.
- [R] Padrões condicionados a `PROJECT_IS_TOP_LEVEL`.

#### M8. Acessores com verificação de limites
Os acessores do caminho quente de `Mesh` usam `.at()` (`mesh.hpp:114-130`: `owner`, `point`, `site`, `region`). Isso tem custo no laço de fluxos do solver. [R] `operator[]` com assert em Debug.

#### M9. `LayeredMesh{}` é construtível por padrão
O construtor padrão contorna a validação de `from_data` (usado em `ut_Cvt.cpp:174`) e quebra a promessa "imutável e validada".

#### M10. Critério de convergência do CVT praticamente inalcançável
O critério é a norma do máximo, com 10⁻⁶·L e 50 iterações. No benchmark do próprio projeto, nenhum caso convergiu. No meu teste do meandro (400 sítios, 30 iterações), o resíduo ficou em 3,6·10⁻⁴. "Sucesso" sem convergência é o caso normal. [R] Critério em RMS ou por energia relativa, e aceleração (Anderson, L-BFGS).

#### M11. Mensagens de erro enganosas
- VMM-202 "menos de 3 vértices, auto-interseção ou coordenada não finita" para um arredondamento do overlay (I3).
- VMM-402 "interface não conforme" num domínio de uma região (I4).
- O `benchmark.py` do CVT depende de `/usr/bin/time`, que não existe neste ambiente.

---

## 2. Afirmações dos relatórios que não se sustentam

| # | Afirmação | Onde | O que encontrei |
|---|---|---|---|
| 1 | "Não é teste de CVT, que não está implementado"; "Não foi implementado CVT"; "T01/CVT permanece futuro" | `horizontes/relatorios/P27_validacao.md:41`; `P29_entrega.md:10,29`; `horizontes/00_sequencia.md:97` | `optimize_cvt`/`cvt_domain` em `include/vmm/cvt.hpp` e `src/facade/cvt*.cpp`, com 13 testes, em `HEAD` [F] |
| 2 | "Sem commit, tag ou push"; "1.1 preparada na árvore de trabalho" | P21–P29, `cvt/01_entrega.md`, `README.md:42` | Tudo commitado em `main` (8191e26…fb02bb7); nenhuma tag existe [F] |
| 3 | R24 "NATIVE_ARCH e LTO desligados por padrão"; P14 R24 "OK … OFF" | `P05_BASELINE.md:73`; `P14_release_0_1.md` R24 | `vmm/CMakeLists.txt:19-20` liga as duas (DEC-034); a linha de base não foi revisada [F] |
| 4 | DEC-034 "build padrão rápido" | `DECISIONS.md:487` | O P07 mediu ganho "dentro da variação" (`P07_infra.md:80`) [F] |
| 5 | "A CI testa as versões mínimas e as mais recentes" | DEC-022; R12 (`P05_BASELINE.md:51`) | Só gcc-14 e clang-19 em ubuntu-24.04 (`vmm-ci.yml:23-26`) [F] |
| 6 | CMake mínimo 3.28 | `CMakeLists.txt:1` | A configuração falha com o 3.28.3 (I8) [F] |
| 7 | "vértices … nunca por proximidade" | DEC-028 (`DECISIONS.md:403`) | Fusão por proximidade em `builder2d.cpp:455-490` [F] |
| 8 | Ortogonalidade "pelo backend exato" | DEC-032 | Células 3D em double (`P15_arquitetura_3d.md:108`); na malha em colunas, 0,27 rad medidos no mesmo tipo `Mesh3D` (B3) [F] |
| 9 | "os headers internos dizem que são internos"; a lista de internos | DEC-041; `docs/api.rst:14-20` | `column_overlay.hpp` diz "Internal API" e não está na lista, logo é estável pela regra (B2) [F] |
| 10 | "o usuário pode refinar os sítios onde elas [fragmentadas] aparecem" | DEC-035 | O contador é interno (DEC-041) e o aviso é descartado sem sink (`log.hpp:4`), e nenhum sink é instalado (I7) [F] |
| 11 | "mesma malha (checksum)", "mesmos checksums" | P16:60, P17:53,80, P18:71-77 | Nenhum valor registrado em `planning/`; não é verificável por terceiros [F] |
| 12 | R26 "casos patológicos" OK; R16 "1000 configurações" | P14 §1, P19 §2 | Só sítios Poisson; um obstáculo fino alinhado derruba 2D e 3D (I4) [F] |
| 13 | Cobertura 1.1 de "97,3% / 90,0%; R25 aprovado" | `cvt/01_entrega.md` | Combina uma suíte completa anterior com 13 casos posteriores; o P27 pede para não citar números dirigidos como finais [F]; não reexecutei a cobertura |
| 14 | "Build completo Clang 19 aprovado", "Clang19 … 25/25 dirigidos" | `cvt/01_entrega.md`; `P27_validacao.md` | A suíte completa com Clang e com sanitizadores na 1.1 não está documentada [F]; a CI não está acessível para mim |
| 15 | "Volume zero não materializa célula", "todas as faces são planas" | `P22_P26_implementacao.md`; `layered.hpp` | Sustentado no código (`facade/layered.cpp:103,151`; faces laterais verticais), mas com explosão de faces não mencionada (I2) |
| 16 | "A classificação … pode dominar o custo em malhas grandes" | `horizons.rst`; P29 item 1 | Subestimado: medi expoente de ≈1,9 e 70 s para 8100 colunas; a extrapolação dá horas ou dias na escala do MohidNG (B1) [F/I] |
| 17 | "1.0: nenhuma quebra de API entre 0.5 e 1.0" como critério de estabilidade | DEC-025, `P05_BASELINE.md` §4 | 0.2.0 e 1.0.0 com a mesma data, sem tag nem período de uso (I11) [F] |
| 18 | "Dependências: CGAL ≥ 6.2" | `README.md` | Só a 6.2.1 é testada (CI e relatórios) [F] |
| 19 | Metas DEC-020 2D (10⁶ em ≤30 s, ≤1 KB/célula) | P07 §4 | **Sustentada** aqui: 16,3 s e 673 B/célula [F]. Meta 3D: 2·10⁵ células em 140 s nesta máquina (≈2,7× mais lenta que o WSL no B1); 10⁶ não medido |

## 3. Perguntas que o autor precisa responder

1. Qual é o tamanho-alvo real do MohidNG (colunas × camadas) e o tempo aceitável de geração? Com essa resposta, B1 e I1 passam a ser critério de aceitação da 1.1 ou não.
2. O fluxo TPFA do MohidNG e do seu solver usa `site()` como gerador? Se usa, a malha em colunas precisa guardar o gerador da base (B3). Se usa o centroide, por que a garantia de ortogonalidade da DEC-032 continua sendo vendida para `Mesh3D` em geral?
3. O MohidNG precisa de uma face única entre camadas por coluna (I2)? E de z-level com corte pelo fundo, ou só de sigma por intervalo?
4. Qual é a licença do MohidNG? É compatível com ligar estaticamente um backend GPL-3.0 (I16)?
5. Os itens da 1.1 (`column_overlay.hpp`, `LayeredData`, `CvtReport`, `.vlayers`, `.hgrid`, chaves `cvt_*`) devem mesmo ficar congelados na 1.x, ou a 1.1 deve publicá-los como experimentais (B2)?
6. Qual commit é a 1.0.0? Por que não há tags, e quando `main` deixa de anunciar "1.1.0 não publicada"?
7. O CVT volumétrico, que perde as colunas, tem algum consumidor? Se o uso é o MohidNG, o padrão do CLI não deveria ser o CVT da base 2D seguido da extrusão (I6)?
8. Falha dura em entrada degenerada (dique alinhado, MDT alinhado) é aceitável, ou o VMM deve resolver empates simbolicamente (I3, I4)?
9. Por que manter LTO e `-march=native` como padrão, se o P07 não mediu ganho (I10)? Como o pacote será distribuído para o cluster do MohidNG?
10. Qual é a versão mínima real do CMake (I8)? E a do CGAL (6.2.0 ou só 6.2.1)?
11. Quem revisa as execuções autônomas ("sem aprovação intermediária")? Onde ficam os logs que sustentam as afirmações, para quem não tem acesso ao WSL (I13)?
12. A R18 (mesma topologia entre plataformas) é um requisito real? Se for, falta CI em outra plataforma e checksums versionados (M6).
13. Células fragmentadas devem poder ser proibidas pelo usuário (I7)? Em rios com diques, uma célula com pedaços nas duas margens é aceitável para o MohidNG?

## 4. Veredito

**Não publicar a 1.1 como está.** O núcleo 1.0 (2D e 3D Voronoi) está bem sustentado nos meus testes: 324/324, a meta do B1 atingida, UTM e grades 3D sem falhas. Mas a 1.1 tem três problemas que precisam ser resolvidos antes de a tag congelar a API pela DEC-041:
- a geração em colunas, que é o recurso pensado para o MohidNG, tem custo quase quadrático (B1, I1);
- ela entra sem nenhuma decisão registrada e com headers ditos internos que viram estáveis (B2);
- ela troca o significado de `Mesh3D::site` e perde a ortogonalidade sem avisar (B3).

Depois disso, tratar I3, I4, I8, I11 e I14 antes de uma 1.1.0 final.
