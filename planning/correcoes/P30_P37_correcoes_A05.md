# P30–P37 — Correções dos problemas do A05

- **Data:** 2026-10-17
- **Destino no repositório:** `planning/correcoes/P30_P37_correcoes_A05.md`
- **Entradas obrigatórias:** `planning/analises/A01_SIPOC.md` … `A05_consolidado.md`
- **Base:** `main` em `fb02bb7` (1.1.0 preparada, não publicada). Se a árvore local estiver à frente, compare com a análise antes de começar (etapa P30.0).
- **Uso:** cole o bloco **REGRAS COMUNS** e o bloco **CONTEXTO** no início da sessão; depois execute **uma etapa por vez** (P30, P31, …). Cada etapa termina parada, à espera da aprovação do João.

---

## REGRAS COMUNS

```
REGRAS COMUNS
- Repositório: /home/jflavio/Programas/VMM, na distribuição WSL Ubuntu-26.04-Test (siga o AGENTS.md).
  Remoto: https://github.com/voronoimeshmaker/VoronoiMeshMaker
- Não faça commit, tag nem push. Preserve as alterações não commitadas. O João faz os commits.
- Leia antes de começar: AGENTS.md, planning/00_requisitos_iniciais.md, planning/P05_BASELINE.md,
  planning/DECISIONS.md, planning/analises/A01–A05 e o relatório da etapa anterior desta sequência.
  As decisões APROVADAS são obrigatórias; não as reabra. Novas decisões entram no DECISIONS.md com status
  PROPOSTA (numeração sequencial a partir da última existente). Nunca altere o status de uma entrada.
- Declare no início de cada relatório o ambiente e as versões usadas (compilador, CMake, CGAL, Boost,
  GoogleTest, Python e as demais). Só versões estáveis (DEC-012). C++23 (DEC-014).
- R3 / DEC-024: sem virtual, sem herança, sem enum que escolha implementação. Variação por templates,
  concepts, tipos-política, composição e registros abertos de callables. Enum de dado é permitido.
- API (DEC-041): nada da API 1.0 (estado do commit da 1.0.0, ver P30.2) pode ser removido nem mudar de
  assinatura ou de significado. Campos novos só no fim das structs, com valor padrão. O que é novo na 1.1
  segue a decisão D1 (abaixo).
- Tolerâncias relativas (R17). Nenhuma tolerância absoluta em código geométrico.
- Testes: GTest por classe pública não trivial (R1/R2), na árvore espelhada; depois integração; só então
  exemplos na galeria (R30). Testes de propriedade para cada correção geométrica. Cobertura ≥ 90% linhas e
  80% ramos por arquivo novo ou alterado (R25).
- Nada de afirmar que um teste passou, que a cobertura foi atingida ou que um desempenho foi alcançado sem
  evidência do ambiente ativo. Toda afirmação sobre CGAL ou outra biblioteca cita a documentação oficial ou o
  código-fonte, com link e versão.
- Evidência versionada: cada relatório copia para planning/correcoes/evidencias/<etapa>/ um resumo dos logs
  (comando, data, versões, contagem de testes aprovados, tempos) e as somas de verificação dos benchmarks.
  Logs brutos podem ficar em build/, mas o relatório não pode depender só deles.
- Suíte completa antes de declarar uma etapa concluída: Release GCC, Debug GCC com ASan/UBSan, Release
  Clang 19 e cobertura. Execuções "dirigidas" só valem como passo intermediário.
- Distinga sempre: FATO VERIFICADO / INFERÊNCIA / RECOMENDAÇÃO.
- Entregáveis em português; código, identificadores, comentários e Doxygen em inglês, no estilo existente
  (bloco de cabeçalho File/Description, ordem dos includes do AGENTS.md).
- Iterações pequenas (≈ 500 linhas de produção por iteração); a primeira iteração de cada etapa é o plano,
  sem código. Limite de três tentativas por iteração; esgotado, pare e descreva o impasse.
- Ao terminar a etapa: (1) resumo de até 15 linhas; (2) DECISÕES PENDENTES numeradas, com opções e a sua
  recomendação; (3) PARE. Não inicie a etapa seguinte.
```

## CONTEXTO

```
CONTEXTO
O A05 (planning/analises/A05_consolidado.md) concluiu: o núcleo 1.0 (2D/3D Voronoi) é sólido
(324/324 testes, B1 com 10⁶ células em 16 s), mas a 1.1 não deve receber tag. Problemas principais:
- colunas (generate_layered_mesh) com custo ≈ quadrático no número de colunas, consultas O(N) por chamada,
  explosão de faces, sítio = centroide (não ortogonalidade lateral de até 0,27 rad) e falha dura com MDT
  alinhado à base (VMM-202);
- obstáculo fino com borda sobre o bissetor de sítios em grade derruba 2D e 3D (VMM-402);
- pares espelhados (InterfacePairs) não são gerados em interfaces curvas (sem aviso) e falham em quinas;
  o CVT desfaz a ortogonalidade dos pares;
- CVT sem densidade (desfaz a graduação), caro (reconstrução completa por tentativa, varredura linear da
  partição), com passo global e critério de convergência inalcançável;
- CMake 3.28 (mínimo declarado) não configura; sem tags; LTO/-march=native por padrão; CI aquém da R12;
  read_native deixa escapar std::bad_alloc; decisões da 1.1 fora do DECISIONS.md; relatórios
  contraditórios; evidência não versionada.
O consumidor principal é o MohidNG (malha 2,5D: colunas com interfaces planas por coluna, ligação lateral
entre camadas, fluxo de dois pontos sem correção de não ortogonalidade).
```

## DECISÕES ASSUMIDAS

As etapas P32–P35 dependem destas decisões. **Os valores abaixo são as recomendações do A05.** Se o João não as tiver confirmado ou alterado, a P31 as transforma em DECs PROPOSTA e a sequência **para** antes da P32.

| # | Decisão | Valor assumido (recomendação) |
|---|---|---|
| D1 | Estado de colunas e CVT na 1.1 | **experimentais**: declarados em `docs/api.rst` como "sem garantia na 1.x" até nova DEC; o resto da API 1.0 continua estável |
| D2 | Sítio nas malhas em colunas | **gerador da base** em (x, y) e z no meio da célula na vertical do gerador; centroide só como fallback marcado |
| D3 | Padrão de build | **portátil**: `VMM_ENABLE_NATIVE_ARCH=OFF` e `VMM_ENABLE_LTO=OFF` por padrão; preset `fast` liga as duas (substitui a DEC-034) |
| D4 | CMake mínimo | corrigir a verificação e **manter 3.28**; se não for possível sem perda de função, subir para 3.29 |
| D5 | Commit da 1.0.0 | **`da5c03e`** (último `VERSION 1.0.0` com CI verde, antes da 1.1) |
| D6 | Execuções autônomas | permitidas só com evidência versionada e suíte completa no fim de cada etapa |
| D7 | Modo de colunas para o MohidNG | acrescentar o modo **"plano por coluna"** (uma cota por interface e por coluna, como o `SZZ` do MOHID), sem remover o modo atual (linear por partes, fiel ao MDT) |

---

## P30 — Higiene de build, leitura robusta e publicação

```
[REGRAS COMUNS] [CONTEXTO]

OBJETIVO
Eliminar os problemas da Fase 0 do A05 sem mudar a geometria: CMake mínimo, versão rastreável, main
protegido, verificador R1/R4, leitores robustos, aviso de células fragmentadas, relatórios reconciliados e
checksums versionados.

TAREFAS
P30.0 Estado. Compare a árvore local com fb02bb7 (git log, git status, git diff --stat). Liste no relatório
      o que mudou desde a análise; se algo do A05 já foi corrigido, registre e pule.

P30.1 CMake mínimo (F01, I8).
      - Reproduza com o CMake 3.28.3 (pip install "cmake==3.28.3" num venv): a configuração falha em
        cmake/VerifyCGALHeaders.cmake no check_cxx_source_compiles com CMAKE_REQUIRED_LIBRARIES CGAL::CGAL
        ("links to: CGAL::CGAL but the target was not found"); 3.29.6+ configura.
      - Diagnostique a causa exata (propagação de alvos importados/alias ao try_compile no 3.28; veja a
        documentação de try_compile e da política relevante da versão). Cite a fonte.
      - Correção preferida (D4): não ligar alvo no try_compile. Extraia INTERFACE_INCLUDE_DIRECTORIES,
        INTERFACE_COMPILE_DEFINITIONS e INTERFACE_COMPILE_OPTIONS de CGAL::CGAL (resolvendo o
        ALIASED_TARGET) e passe por CMAKE_REQUIRED_INCLUDES / CMAKE_REQUIRED_DEFINITIONS /
        CMAKE_REQUIRED_FLAGS, mantendo o overlay de include que já existe. Se não houver forma limpa,
        suba cmake_minimum_required para 3.29 em CMakeLists.txt e vmm/CMakeLists.txt e no README.
      - Teste: configure com 3.28.3 (ou 3.29.x) e com o CMake mais recente; os dois têm de terminar em
        "Generating done" e compilar o alvo vmm_firewall.

P30.2 Versão rastreável (F04, I11).
      - Confirme o commit da 1.0.0 (D5): todo commit entre 75d42ae e 8191e26 declara VERSION 1.0.0; o
        8191e26 já traz o planejamento dos horizontes. Verifique o conteúdo de da5c03e (headers, CHANGELOG,
        CI verde) e registre a evidência. NÃO crie a tag: escreva no relatório os comandos para o João
        (git tag -a v1.0.0 <commit> -m ...; git push origin v1.0.0).
      - Na árvore: project VERSION 1.1.0 continua, mas core/version.hpp ganha o sufixo de pré-release
        exposto como string ("1.1.0-dev") até a aprovação da 1.1; o teste de versão compara número e
        sufixo. README e vmm/README dizem "1.1 em desenvolvimento".
      - vmm-docs.yml: publicar o Pages só em push de tag v*; em main, gerar o site como artefato com um
        aviso de versão em desenvolvimento (banner no conf.py quando a versão tiver sufixo).

P30.3 main protegido (F05). Escreva no relatório o passo a passo para o João ativar a proteção do main
      (Settings → Branches: PR obrigatório, checks vmm-ci obrigatórios). Ajuste os workflows para rodar
      em pull_request (já rodam) e documente o fluxo em vmm/CONTRIBUTING.md.

P30.4 Verificador R1/R4 (F08, M1). Em vmm/tools/ci/check_requirements.py:
      - headers do topo de include/vmm (cvt.hpp, layered.hpp, vmm.hpp) passam a ser do módulo "facade"
        no R1 (testes em tests/facade/<Classe>/ ou declaração explícita na lista de triviais) e no R4;
      - o R4 passa a reconhecer #include <vmm/xxx.hpp> (raiz) como dependência do módulo facade;
      - ALLOWED_DEPS["app"] ganha "facade" só se a arquitetura permitir; senão, o app deve depender da
        fachada por um header próprio — decida, documente e proponha DEC se mudar o grafo.
      - Teste negativo em tools/ci/negative/ que prove que o verificador acusa um header do topo sem teste.

P30.5 Leitores robustos (I14; R11/DEC-016).
      - Audite read_native, read_layered, read_stl, read_horizons e o leitor de .cfg: nenhuma exceção da
        biblioteca padrão pode escapar (bad_alloc, length_error, out_of_range, invalid_argument).
      - Contagens lidas do arquivo: valide contra o tamanho do arquivo (cada registro tem um número mínimo
        de bytes; n > bytes_restantes/min_bytes → ParseError) e contra o limite de IDs; reserve de forma
        incremental (reserve(min(n, bloco)) + push_back), nunca resize(n) direto.
      - Envolva a fronteira pública de cada leitor em try/catch que converte exceções em Result com
        ErrorCode próprio (ex.: FileCorrupt/ResourceExhausted, se não houver, proponha no catálogo pt/en).
      - Testes negativos por leitor: contagem gigante (points 4000000000000), truncamento em cada seção,
        NaN/Inf, IDs fora da faixa, linha extra, arquivo vazio. Opcional: alvo de fuzzing (libFuzzer,
        desligado por padrão) para read_native.

P30.6 Células fragmentadas visíveis (F16, I7).
      - vmm-mesh instala um sink de log para stderr (set_log_sink) e imprime fragmented_cells do
        BuildStats ao final; opção --quiet.
      - API estável nova (aditiva): MeshResult2D/3D ganha `std::vector<CellId> fragmented_cells` no fim da
        struct (ou função fragmented_cells(const Mesh<D>&) que detecta pedaços desconexos pela geometria,
        sem depender de BuildStats). Opção `reject_fragmented` (padrão false) em MeshRequest2D/3D →
        ErrorCode novo com a lista de células no contexto. Chave .cfg `reject_fragmented = true|false`.
      - Testes: rio em U (polígono do Anexo B.1) com grade 0,75 → contagem > 0; com reject → erro.

P30.7 Relatórios reconciliados (I12). Em planning/horizontes/00_sequencia.md, P27/P29 e
      planning/cvt/00_sequencia.md, acrescente no topo "Estado em <commit>" com o que está implementado
      (CVT existe; tudo está em main). Não reescreva o histórico: acrescente.

P30.8 Checksums versionados (F06, I13). vmm_benchmark imprime a soma de verificação; registre em
      vmm/tests/data/checksums.txt os valores de B1–B7 por plataforma/compilador/versão do CGAL, e um teste
      (rótulo slow) que gera B1 com 10⁵ sítios e compara com o valor registrado para a plataforma (sem
      registro, o teste é pulado com mensagem). Referência medida na nuvem (GCC 14.2, CGAL 6.2.1, Ubuntu
      24.04, Release LTO/native): B1(10⁶) = 30df5ed2ebaeaf4a; B4(2·10⁵) = 9802537f1663e195.

ENTREGÁVEL
planning/correcoes/P30_relatorio.md + evidencias/P30/.

CRITÉRIO DE CONCLUSÃO
Configura com o CMake mínimo e o mais recente; check_requirements com os novos testes negativos; nenhum
leitor deixa escapar exceção nos testes negativos; vmm-mesh mostra células fragmentadas; suíte completa
nas quatro configurações.
```

---

## P31 — Decisões da 1.1

```
[REGRAS COMUNS] [CONTEXTO]

OBJETIVO
Registrar em planning/DECISIONS.md, com status PROPOSTA, todas as decisões que a 1.1 tomou fora do registro
e as decisões D1–D7, para que o João aprove antes do trabalho técnico.

TAREFAS
1. Levante as decisões implícitas em planning/horizontes (H01–H10, Q01–Q10, P21–P29) e planning/cvt
   (00_sequencia.md, "Decisões"). Para cada uma: DEC com Data, Origem (arquivo:seção), Decisão,
   Justificativa, Consequências. No mínimo:
   - HorizonGrid (gráficos z(x,y), grade retangular triangulada fixa, z para cima);
   - região = região da base × intervalo; subdivisões não são interfaces obrigatórias;
   - formatos .hgrid (VMM_HORIZONS 1) e .vlayers (VMM_LAYERS 1) e o estado de estabilidade de cada um;
   - CVT: Lloyd com partição fixa, densidade uniforme, sem inserir ou remover sítios, chaves cvt_*;
   - execução sem aprovação intermediária (H08) e a regra D6.
2. DECs para D1–D7 (texto da tabela "Decisões assumidas"), cada uma citando a DEC que substitui
   (D3 substitui a DEC-034 e revisa a R24; D4 ajusta a R12/R22 se mudar o mínimo).
3. D1 (experimental): proponha o mecanismo. Recomendação: não mover símbolos de namespace (quebraria
   código já escrito); declarar em docs/api.rst a seção "Experimental na 1.x": cvt.hpp, layered.hpp,
   mesh/layered.hpp, io/layered.hpp, geometry/horizons.hpp, as chaves horizons e cvt_*, .hgrid e .vlayers;
   e acrescentar column_overlay.hpp à lista de internos (ou movê-lo para src/backend/cgal/, que é melhor:
   nenhum consumidor deve incluí-lo — verifique e decida).
4. Atualize P05_BASELINE.md com uma nova versão (R24 e R12 conforme D3/D4), sem apagar a anterior.

ENTREGÁVEL
DECISIONS.md com as entradas PROPOSTA; planning/correcoes/P31_relatorio.md com a tabela "DEC × arquivos
afetados × etapa que implementa".

CRITÉRIO DE CONCLUSÃO
Nenhuma decisão da 1.1 sem DEC; o João aprovou ou ajustou D1–D7. PARE: a P32 só começa com D1, D2 e D7
aprovadas.
```

---

## P32 — Colunas: escala, faces, sítios e modo plano por coluna

Esta é a etapa de maior impacto para o MohidNG. Abaixo, a análise técnica que o agente deve seguir.

**Diagnóstico do código atual** (`src/backend/cgal/column_overlay.cpp`, `src/facade/layered.cpp`, `src/mesh/layered.cpp`):

- **A grade do MDT inteira entra na CDT**, com cinco restrições por retângulo, mesmo fora da pegada da base. O custo depende do MDT, não da malha.
- **`locate` classifica cada triângulo pelo seu centroide**, com número de enrolamento sobre **todas** as arestas da base: O(T·E), daí o comportamento quadrático. O patch de cada lado de triângulo também é achado por `Segment_2::has_on` contra todas as arestas de contorno.
- **A saída arredonda os pontos exatos para double** e funde por `std::map` de coordenadas. Triângulos com vértices distintos, mas colineares depois do arredondamento, derrubam a geração ("rounded overlay triangle is degenerate"). É o caso do MDT alinhado a uma base de espaçamento 0,1, cujos vértices estão a alguns ulps de k·0,1.
- **Faces:**
  - cada triângulo do refinamento gera topo, base e três laterais;
  - só as faces internas da mesma célula são canceladas;
  - não há fusão das faces coplanares com o mesmo par de células (até 110 faces/célula medidas).
- **Sítio:** `md.sites` recebe o centroide e `cell_input_site` recebe o índice da coluna. Com isso, as faces laterais deixam de ser ortogonais à reta entre "sítios" (0,27 rad medidos).
- **Consultas:** `cells_in_column` e `vertical_neighbours` percorrem a malha inteira a cada chamada.

```
[REGRAS COMUNS] [CONTEXTO]

PRÉ-REQUISITO: D1, D2 e D7 aprovadas.

OBJETIVO
Tornar a geração em colunas O((E + T) log T) no modo atual (linear por partes), acrescentar o modo "plano
por coluna" em O(N), corrigir a semântica do sítio, fundir faces coplanares, tornar as consultas O(1) por
célula e eliminar a falha de arredondamento do overlay.

TAREFAS

P32.1 Classificação por propagação (B1).
  - Troque o CDT por CGAL::Constrained_triangulation_plus_2<CDT> e insira cada aresta da base como
    restrição, guardando Constraint_id → índice da aresta. Ref.: manual do CGAL 6.2, 2D Triangulations,
    "Constrained Triangulations Plus" (contexts e vertices_in_constraint).
  - Insira só as linhas do MDT que cruzam a caixa envolvente da pegada (recorte por índice i/j na grade),
    e só as diagonais dos retângulos do MDT que tocam a caixa. Meça o ganho.
  - Para cada aresta da base e cada subaresta (u,v) da sua restrição: obtenha as duas faces incidentes
    (cdt.is_edge(u,v,f,i)); a face do lado do owner recebe a coluna owner e a do outro lado recebe a
    neighbour (ou fica fora, se a aresta for de contorno). Determine a convenção de orientação do Mesh2D
    (lado do owner em relação a v0→v1) por teste, não por suposição.
  - Propagação em largura: a partir das faces semeadas, atravesse arestas que NÃO sejam restrições da base
    (as do MDT são atravessáveis). Conflito de rótulo → erro InvalidPolygon com as duas colunas no
    contexto. Face não alcançada = fora da pegada.
  - Patch de contorno: venha da restrição (aresta da base sem neighbour), nunca de busca por has_on.
  - Complexidade alvo: O((E + T) log T) dominado pela CDT. Remova o vetor winding por consulta.

P32.2 Saída em double sem colapso (I3).
  - Opção A (preferida): snap rounding do arranjo das restrições antes da triangulação final. Use o pacote
    CGAL Snap Rounding 2 (snap_rounding_2, ISR) com tamanho de pixel δ = 2^⌊log2(10⁻¹² L)⌋ (potência de 2
    para exatidão). Verifique na documentação os requisitos de núcleo (número racional exato) e cite.
    Reconstrua a CDT a partir das polilinhas arredondadas: todos os vértices passam a ser double exatos e
    nenhum triângulo degenera depois.
  - Opção B (se A for inviável): pré-snap. Antes de inserir, funda coordenadas de linhas do MDT com
    vértices da base a menos de τ = 10⁻¹² L (no eixo da linha) e funda vértices da base que estejam a menos
    de τ de uma linha do MDT; depois de construir, colapse arestas de comprimento < τ de forma consistente
    em todos os triângulos e trate triângulos colineares por troca de aresta (edge flip) no complexo
    arredondado.
  - Mensagem própria (ErrorCode novo ou contexto claro) quando ainda assim não houver representação.
  - Testes de propriedade: base cartesiana × MDT com o mesmo espaçamento para h ∈ {0,1; 0,3; 1/3; 0,125},
    com deslocamentos {0; 1e-15; 1e-13; 1e-9}·L, em origem 0 e em UTM (5e5, 7,5e6); todos geram e
    passam nos invariantes.

P32.3 Fusão de faces (I2).
  - Laterais: as quadriláteras verticais de um mesmo par (owner, neighbour) sobre a mesma aresta da base e
    na mesma faixa de camada formam um único polígono planar (estão no mesmo plano vertical). Una-as numa
    face, ordenando os vértices ao longo da aresta (baixo da esquerda para a direita e topo de volta).
  - Horizontais: triângulos de topo/base com o mesmo par de células e coplanares (|distância ao plano| ≤
    10⁻¹² L para todos os vértices) se unem; o contorno é extraído pelas arestas que aparecem uma só vez no
    conjunto, percorridas em ciclo. Se o conjunto não for simplesmente conexo, mantenha as componentes
    separadas.
  - Todos os vértices do contorno são mantidos (inclusive colineares), para a conformidade com as faces
    vizinhas.
  - Invariantes: vetor área de cada célula fecha em ~ε·L²; soma das áreas inalterada.
  - Meta: horizonte plano → 1 face por coluna por interface de camada; MDT de 2,5 m sobre base de 12,5 m →
    reduzir as 110 faces/célula medidas e registrar o novo número.

P32.4 Sítio = gerador da base (D2, B3).
  - LayeredData ganha (no fim, com padrão) `std::vector<Vec2> base_generator` por coluna e
    `std::vector<std::uint8_t> site_is_centroid` por célula (ou estrutura equivalente). md.sites passa a
    ser (g_x, g_y, z̄), com z̄ = média das cotas de base e topo da célula na vertical de g (interpolação
    no overlay ou em HorizonGrid::sample).
  - Se a espessura da célula na vertical de g for ≤ 10⁻¹² L (pinch-out que exclui o gerador), use o
    centroide, marque site_is_centroid e conte no relatório da geração.
  - Prove por teste: com horizontes planos, a não ortogonalidade das faces laterais medida por
    compute_metrics cai de ~10⁻¹ rad para o nível de arredondamento (≤ 10⁻⁸ rad) na projeção horizontal;
    reporte a inclinação vertical atan(Δz/|Δxy|) separadamente (métrica nova "vertical_skew" por face,
    aditiva).
  - check_invariants: o teste de orientação d·S > 0 continua válido com esse sítio (justifique no
    relatório); para células com site_is_centroid, verifique e documente o que acontece.
  - cvt_energy recusa malhas cujos sítios não sejam geradores (LayeredMesh), com erro claro.

P32.5 Consultas O(1) (I1).
  - LayeredMesh::from_data monta CSR coluna → células (ordenadas por camada) e, por célula, os vizinhos
    verticais (abaixo/acima) a partir de CellFaceIndex e face_level. cells_in_column e vertical_neighbours
    devolvem std::span<const CellId> (ou mantenha a assinatura atual se ela for estável e acrescente as
    versões span; D1 diz se pode mudar).
  - Teste de custo (rótulo perf): para 10⁵ células, consultar todas as colunas em < 50 ms.

P32.6 Modo "plano por coluna" (D7) — o formato natural do MohidNG.
  - Nova política de geração, sem enum de despacho: por exemplo, `generate_layered_mesh(base, horizons,
    fractions, FlatColumns{sampler})`, em que `sampler` é um tipo-política com operator() que devolve a cota
    de cada horizonte por coluna. Forneça dois: amostragem no gerador da base e média sobre o polígono da
    célula (integral exata do linear por partes sobre o polígono, por recorte com a grade do MDT naquela
    célula — reusar o overlay por coluna — ou quadratura de grau alto com aviso de aproximação).
  - Cada célula é um prisma: polígono da base × [z_b, z_t] constante na coluna. Face horizontal = polígono
    da base (uma por interface de camada). Face lateral entre colunas vizinhas i e j sobre a aresta e: um
    retângulo para cada par de camadas (k_i, k_j) cujos intervalos verticais se sobrepõem, com altura
    [max(z_b), min(z_t)]; contorno externo: um retângulo por camada. Camadas de espessura zero são omitidas
    (pinch-out). A malha é conforme e fechada por construção.
  - Custo O(N) sem CDT. Meta: 10⁶ colunas × 10 camadas em < 60 s numa thread.
  - Sítio (g_x, g_y, (z_b+z_t)/2): ortogonalidade horizontal exata; inclinação vertical relatada.
  - Exponha também, por coluna, o vetor de cotas das interfaces (equivalente ao SZZ do MOHID) e o KFloor
    (primeira camada não vazia), como dados de LayeredMesh.

P32.7 Espessura mínima e qualidade (F24, I17).
  - Opções declaradas (struct de opções, sem enum de despacho): min_thickness relativo (fração da espessura
    da coluna) e/ou absoluto em unidades da malha; política para células abaixo do mínimo: omitir
    (pinch-out), fundir com a camada vizinha do mesmo intervalo, ou erro. Política de fundo: célula de fundo
    "cortada" com espessura mínima (como MININITIALLAYERTHICKNESS do MOHID).
  - Relatório de qualidade por coluna: espessura mínima, inclinação máxima das interfaces, vertical_skew
    máximo, número de células omitidas e fundidas.

P32.8 Benchmark e regressão de escala.
  - Caso B8 no vmm_benchmark: base cartesiana e aleatória, 10³, 10⁴, 10⁵ colunas, 10 camadas, MDT 1×1 e
    MDT 4× mais fino que a base, nos dois modos. Meça tempo, pico de memória e faces/célula.
  - Teste perf: 4× colunas não pode custar mais que 6× tempo (detecta regressão quadrática).
  - Linha de base (A02/A04, nuvem, 2 núcleos): 3969 colunas = 16,6 s; 16 129 = 264 s; 8100 colunas ×
    10 camadas = 70 s.

TESTES
GTest por classe alterada (ColumnOverlay, LayeredMesh, LayeredData, novos tipos-política); integração
tests/integration/Layered com os casos de propriedade de P32.2; casos de pinch-out, ilha, buraco, base
aleatória e cartesiana, UTM; ida e volta .vlayers (versão do formato incrementada se o conteúdo mudar —
D1 permite, por ser experimental; documente).

CRITÉRIO DE CONCLUSÃO
10⁵ colunas no modo linear por partes abaixo de uma meta registrada (proponha o número com base na medição;
alvo indicativo < 120 s); 10⁶ colunas no modo plano em < 60 s; nenhuma falha nos testes de propriedade
de P32.2; faces laterais ortogonais na projeção horizontal; suíte completa.
```

---

## P33 — Robustez 2D/3D: empate entre bissetor e restrição

**Diagnóstico:** o recorte 2D de cada célula é um Sutherland–Hodgman em double (`clip_halfplane`, `src/voronoi/builder2d.cpp:53-86`), com o teste `dot(p − m, n) ≤ 0`. Quando uma aresta do polígono da região está exatamente sobre o bissetor de dois sítios, os dois lados avaliam o empate de forma independente. O resultado são peças de bissetor vistas por uma célula só, e a construção falha com `VMM-402` ("0 interface sub-intervals, 2 bisector pieces"), mesmo com uma só região. O 3D falha de forma análoga ("face … seen by one cell only").

```
[REGRAS COMUNS] [CONTEXTO]

OBJETIVO
Tornar a construção 2D e 3D robusta a coincidências exatas entre bissetores e restrições (contorno,
buracos, interfaces), sem perder a ortogonalidade nem a conformidade.

TAREFAS
P33.1 Reproduza com os casos do Anexo B.2 (dique_a, dike2d_hole, dike3d) e os controles deslocados.

P33.2 Predicado de lado exato com desempate simbólico (2D).
  - Para vértices de entrada (vértices da partição e sítios, que são double exatos), classifique o lado
    em relação ao bissetor de (s_i, s_j) com o predicado exato filtrado do CGAL
    (Epick::Compare_distance_2 / compare_distance_to_point(p, s_i, s_j)); cite a documentação.
  - Empate (EQUAL): desempate simbólico consistente (Simulation of Simplicity), por exemplo o ponto vai
    para a célula de menor índice de sítio; os dois lados precisam aplicar a MESMA regra, para que cada
    ponto pertença a exatamente uma das duas células.
  - Pontos construídos (interseções) não são reclassificados numericamente: o lado é herdado
    combinatoriamente da aresta e dos rótulos que os geraram.
  - Aresta da restrição colinear ao bissetor: o trecho mantém o rótulo da restrição (prioridade
    restrição > bissetor). Não se gera peça de bissetor de comprimento ≥ 0 sobre ela. O casamento de peças
    (unmatched_bisector_pieces) ignora as peças que coincidem com restrições.
  - Alternativa aceitável: delegar ao backend o recorte rotulado das células que tocam restrições (DEC-028
    já prevê topologia por rótulos), mantendo o recorte em double só para células interiores longe de
    restrições (distância > c·ε·L ao contorno, verificada com índice espacial).

P33.3 3D: a mesma regra no recorte por semiespaços e no recorte exato das células de contorno
      (backend3d.cpp/builder3d.cpp). Plano de bissetor coplanar a uma face da superfície: a face mantém o
      rótulo da superfície; desempate por índice de sítio.

P33.4 Mensagens: com uma só região, VMM-402 não pode falar em "interface"; diga "contorno coincide com o
      bissetor dos sítios i e j" com as coordenadas.

P33.5 Testes de propriedade (R26), que o teste de 1000 configurações não cobre hoje:
  - grades cartesianas e hexagonais com obstáculos finos (buraco de espessura 0,1·h a 2·h) alinhados
    exatamente com bissetores, em 2D e 3D, com várias origens e rotações de 0°, 90° e 45°;
  - sítios a 10⁻⁹ L de uma restrição; quatro sítios cocirculares com o círculo tocando um vértice da
    restrição;
  - estenda RandomConfigurations com fontes em grade além de Poisson.

CRITÉRIO DE CONCLUSÃO
Todos os casos do Anexo B.2 geram e passam nos invariantes; nenhuma regressão no B1 (tempo ≤ +10%);
suíte completa.
```

---

## P34 — Interfaces ortogonais por polilinha (método dos círculos)

**Diagnóstico:** `InterfacePairs::generate` (`src/sites/sources.cpp:194-227`) trabalha **por segmento**, com uma folga de `clearance × spacing` em cada ponta. Por isso:

- segmentos mais curtos que 2·spacing ficam sem pares; uma interface curva poligonizada (64 segmentos num círculo) não recebe nenhum par, e nada avisa;
- nas quinas e nas pontas sobram faces não ortogonais (4 de 22 numa interface reta medida);
- os pares não sabem uns dos outros.

**Princípio da correção** (o mesmo do VoroCrust em 2D e das malhas PEBI que seguem falhas):

1. Escolha nós sobre a polilinha da interface, **incluindo todos os seus vértices**, porque a geometria é fixa (H06).
2. Centre em cada nó pᵢ um círculo de raio rᵢ, de modo que círculos consecutivos se cruzem e não consecutivos não se cruzem.
3. Os dois pontos de interseção dos círculos i e i+1 são simétricos em relação à corda pᵢpᵢ₊₁. A mediatriz deles é essa corda, e pᵢ é equidistante de todos os pontos sobre o círculo i. Assim, se nenhum outro sítio cair dentro da união dos círculos, cada nó é um vértice de Voronoi e cada corda é uma face de Voronoi.
4. Resultado: a interface inteira é formada por faces ortogonais, inclusive nas quinas.

```
[REGRAS COMUNS] [CONTEXTO]

OBJETIVO
Entregar faces de interface ortogonais em qualquer interface 2D poligonal (retas, curvas poligonizadas,
quinas, junções triplas), com aviso quando não for possível, e preparar a mesma estrutura para o CVT.

TAREFAS
P34.1 Nova política de sítios, aditiva (DEC-041): `InterfaceCircles` (nome a decidir), sem alterar o
      comportamento documentado de InterfacePairs (que pode ser marcado [[deprecated]] numa versão menor
      posterior, por DEC). Chave .cfg nova: interface_sampling = circles (mantém interface_pairs).

P34.2 Grafo das interfaces: a partir de Partition2D, encadeie os segmentos de interface em polilinhas entre
      nós de grau ≠ 2 (junções triplas, encontros com o contorno externo). Todos os vértices da polilinha
      são nós; segmentos longos recebem nós intermediários com espaçamento h(x) (constante ou de um
      SpacingField).

P34.3 Raios: rᵢ = α · min(|pᵢpᵢ₋₁|, |pᵢpᵢ₊₁|), com α ∈ (0,5; 1) (padrão 0,6), garantindo
      rᵢ + rᵢ₊₁ > |pᵢpᵢ₊₁|. Verifique com índice espacial (grade hash ou CGAL AABB_tree 2D/Box_intersection_d;
      cite a documentação) que círculos não consecutivos, de qualquer interface, não se cruzam. Onde se
      cruzam (quinas agudas, interfaces próximas), refine localmente os nós e reduza os raios; registre o
      ângulo mínimo suportado.

P34.4 Pontos: interseções dos círculos i e i+1 (fórmula fechada; em double). Cada ponto vai para a região do
      seu lado da corda. Descarte e refine se um ponto cair fora da sua região ou dentro de outro círculo.
      Nas pontas sobre o contorno externo, só o ponto interior. Nas junções triplas, o nó da junção tem um
      círculo que cruza o primeiro círculo de cada polilinha incidente; trate a degenerescência cocircular
      (vários pontos sobre o mesmo círculo) e garanta que não surjam faces de comprimento < 10⁻¹² L
      (fusão).

P34.5 Exclusão: os sítios das fontes de cada região que caem dentro da união dos círculos são removidos
      (índice espacial), substituindo o critério pair_exclusion_fraction para esta política.

P34.6 Diagnóstico: por interface, nós, pares, refinamentos e a não ortogonalidade máxima e média das faces
      de interface (função aditiva interface_quality(mesh) ou campo no relatório de qualidade). Interface sem
      cobertura → aviso no log, contagem no resultado e mensagem no vmm-mesh.

P34.7 3D: registre no relatório o plano equivalente com esferas (InterfacePairs3D existe) e as diferenças
      (triângulos da superfície, arestas vivas, VoroCrust). Implementação 3D só por DEC posterior.

TESTES
Interface reta, círculo (r = 0,3, 64 segmentos), elipse, isóbata sintética z = sen(x) poligonizada, quinas
de 90°, 45° e 20°, junção tripla, interface tocando o contorno; regiões de mesmo meio. Meta: todas as faces
de interface com não ortogonalidade ≤ 10⁻⁶ rad (medida como no Anexo A), exceto as registradas como não
cobertas. Invariantes em todos.

CRITÉRIO DE CONCLUSÃO
Metas acima atingidas; aviso testado; ida e volta .vmesh; galeria com um exemplo de interface curva;
suíte completa.
```

---

## P35 — CVT: restrições, densidade, escala e convergência

**Diagnóstico** (`src/facade/cvt.cpp`):

- cada tentativa chama `build_mesh_*`, `check_invariants` e `fixed_support` (linhas 96-117);
- `fixed_support` testa cada vértice, ponto médio e centro de face de contorno contra **todos** os segmentos ou triângulos da partição (47-89);
- o passo é global: um sítio inadmissível divide o passo de todos (156-181);
- a densidade é uniforme;
- os pares se movem de forma independente (o CVT desfez a ortogonalidade: 38 de 40 faces de interface acima de 0,01 rad);
- a tolerância de 10⁻⁶ L sobre o máximo do deslocamento não é atingida nem em 200 iterações (7164 células).

```
[REGRAS COMUNS] [CONTEXTO]

OBJETIVO
CVT que preserve interfaces ortogonais (P34), respeite a graduação de tamanho, escale para 10⁵–10⁶ células
e convirja com critério realista; e um fluxo de colunas que preserve as colunas.

TAREFAS
P35.1 Restrições por dimensão (hierarquia já descrita ao João):
  - sítios fixos (lista em CvtOptions, campo novo);
  - nós de interface da P34: cantos e junções fixos; nós intermediários deslizam ao longo da polilinha por
    um passo de Lloyd 1D (alvo = centroide, com densidade, do arco de Voronoi restrito ao nó); a cada
    iteração os pares são regenerados a partir dos nós (P34.3–P34.5);
  - sítios interiores: alvo = centroide com densidade; se o alvo sair da região (região não convexa), projete
    no ponto mais próximo da região recuado de uma margem; se cair dentro de um círculo de interface,
    projete para fora.

P35.2 Densidade: ρ(x) = h(x)^-(D+2), a partir do SpacingField (para que o tamanho das células acompanhe h;
      cite a referência — Du, Faber & Gunzburger, SIAM Review 41 (1999) — para a relação ρ ↔ tamanho).
      Centroides e energia por quadratura: leque de triângulos (sítio, vₖ, vₖ₊₁) com regra de Dunavant de
      grau ≥ 5 em 2D; leque de tetraedros com regra de Keast em 3D. Com ρ constante, o resultado tem de
      bater com o atual em ≤ 10⁻¹² relativo.

P35.3 Passo por sítio: limite o deslocamento a 0,5·h local; um sítio inadmissível (validate_sites devolve
      quais) volta à posição anterior, e só ele; os demais seguem. O teste de energia global fica como
      salvaguarda opcional (energia não pode subir mais que 10⁻¹² relativo; se subir, reduza o passo).

P35.4 Custo por iteração:
  - fixed_support com índice espacial (CGAL AABB_tree com primitivas de segmento 2D / triângulo 3D, ou
    grade hash por par de regiões; cite a documentação e meça);
  - check_invariants completo só no fim e a cada k iterações (opção, padrão k = 10); no laço, só as
    verificações baratas;
  - reaproveite tudo o que não depende dos sítios (partição preparada, índices).
  Meta: custo de uma iteração ≤ 1,5 × o de uma geração, em 2D e 3D.

P35.5 Convergência:
  - critério em RMS do deslocamento relativo ao h local, com padrão 10⁻³, mais a variação relativa da
    energia (padrão 10⁻⁸); relate os dois;
  - aceleração opcional: Anderson (m = 5) com salvaguarda (volta ao Lloyd se a energia subir) ou L-BFGS
    sobre a energia, que é C² (Liu et al., ACM TOG 28(4), 2009, "On centroidal Voronoi tessellation —
    energy smoothness and fast computation"). Meça o número de iterações até a tolerância.

P35.6 Colunas: o vmm-mesh com horizons e cvt_* passa a fazer o CVT da base 2D e depois a extrusão (o padrão
      preserva as colunas). O CVT volumétrico fica opcional (cvt_mode = volume) e parte dos sítios da malha
      de entrada, não de sítios aleatórios. Documente a composição em C++ (optimize_cvt(Partition2D) →
      generate_layered_mesh).

TESTES
- Graduação: SpacingField com razão 10 entre extremos; depois de 30 iterações, a razão dos tamanhos médios
  das células se mantém dentro de ±20% da razão de h (medir por faixas).
- Pares: interface reta e curva da P34; depois do CVT, as faces de interface continuam ≤ 10⁻⁶ rad.
- Não convexo: rio em U (Anexo B.1) e meandro; sem estagnação global.
- Escala: 2D com 10⁴ e 10⁵ células; 3D com 10⁴; registrar tempo/iteração e iterações até a tolerância.
- Determinismo (R18): mesma semente → mesma malha; permutação dos sítios → mesma topologia.

CRITÉRIO DE CONCLUSÃO
Metas de P35.4 e dos testes atingidas; o exemplo cvt_horizons preserva as colunas; suíte completa.
```

---

## P36 — CI, padrões de build, embutimento e regressão

```
[REGRAS COMUNS] [CONTEXTO]

TAREFAS
P36.1 Matriz da CI (F02, I9; R12/DEC-022):
  - GCC 14 e o GCC estável mais recente; Clang 19 e o Clang estável mais recente (apt.llvm.org ou
    contêiner oficial; cite a fonte);
  - job com o CMake mínimo e o CGAL mínimo declarado (6.2.0, se for o mínimo; senão ajuste o README);
  - dependências Python da documentação fixadas em vmm/docs/requirements.txt;
  - suíte completa com ASan/UBSan, inclusive os testes slow, num workflow agendado (schedule) noturno;
  - logs de teste como artefatos de cada job.
P36.2 Padrão de build (D3): NATIVE_ARCH e LTO OFF por padrão; CMakePresets.json com presets "default"
      (portátil), "fast" (native + LTO), "coverage" e "sanitize"; install() emite FATAL_ERROR se native/LTO
      estiverem ligados, salvo VMM_ALLOW_NONPORTABLE_INSTALL=ON. Atualize README, DEC e diretrizes.
P36.3 Embutimento (M7): opções de projeto (BUILD_TYPE forçado, testes, exemplos, WERROR) condicionadas a
      PROJECT_IS_TOP_LEVEL; teste de consumidor por add_subdirectory e por FetchContent na CI.
P36.4 Regressão (F29, I15): golden files próprios do VMM, regeneráveis por ferramenta em vmm/tools/golden
      (2D multirregião, 3D multirregião, colunas nos dois modos), com cabeçalho de versões e comparação por
      tolerância relativa (DEC-020); ctest -L perf com limites de tempo para casos de 10⁴–10⁵.
P36.5 Estilo (M2, F31): .clang-format derivado do estilo dominante; verificação na CI só nos arquivos
      alterados; reformatar cvt.*, layered.*, column_overlay.* e acrescentar os blocos File/Description;
      trocar IDs crus std::size_t de LayeredData por Id<Tag> (permitido por D1).
P36.6 Tolerâncias em coordenadas grandes (M3): onde a tolerância compara pontos, use
      τ = c · ε · max(L, ‖x‖∞) e documente; teste com UTM (5e5, 7,5e6) e lados de 100 m, 1 km e 10 km;
      QualityLimits não pode acusar não ortogonalidade "de arredondamento" em UTM.
P36.7 DEC-028 × implementação (M4): a fusão em builder2d.cpp:455-490 é por proximidade (agrupamento
      guloso, não transitivo). Ou corrija o texto da DEC (nova DEC), ou torne a fusão transitiva (union-find
      com grade hash) e teste a cadeia a–b–c com distâncias 0,6τ.
P36.8 Pequenos: acessores sem .at() no caminho quente (assert em Debug) (M8); LayeredMesh{} não construível
      fora de from_data, ou documentado (M9); licença efetiva no README: quem liga o backend CGAL fica sob
      GPL-3.0-or-later (I16).

CRITÉRIO DE CONCLUSÃO
CI verde em todas as entradas da matriz; instalação portátil por padrão; consumidores por add_subdirectory e
FetchContent; golden files e perf no ctest.
```

---

## P37 — Fechamento da 1.1

```
[REGRAS COMUNS] [CONTEXTO]

TAREFAS
1. Suíte completa nas quatro configurações + matriz da CI verde; cobertura de uma execução limpa única.
2. Benchmark B1–B8 com checksums registrados; comparação com a linha de base do A02/A04.
3. Reexecute os experimentos do Anexo A e B e registre antes × depois.
4. CHANGELOG 1.1.0 (o que é estável, o que é experimental, mudanças de padrão de build); docs/api.rst;
   guia de colunas com os dois modos e o fluxo CVT → extrusão.
5. Relatório final reconciliando P21–P29, C01–C05 e P30–P36 com HEAD.
6. Instruções para o João: commit, tag v1.1.0, release no GitHub, Pages por tag.

CRITÉRIO
Checklist da §3 do A05 inteiramente marcado.
```

---

## Anexo A — Medida de não ortogonalidade a partir do `.vmesh`

Ângulo entre a normal da face e a reta entre os sítios do owner e do neighbour, separando as faces de mesma região das faces de interface. Script de referência usado no A02 (`tools/nonortho.py`):

```python
#!/usr/bin/env python3
"""Non-orthogonality of internal faces of a 2D/3D .vmesh, split into same-region and interface faces."""
import math, sys

def read(path):
    lines = [l.split() for l in open(path) if l.strip() and not l.startswith("#")]
    it = iter(lines); assert next(it)[0] == "vmm-mesh"
    dim = int(next(it)[1])
    for _ in range(int(next(it)[1])): next(it)            # media
    for _ in range(int(next(it)[1])): next(it)            # regions
    for _ in range(int(next(it)[1])): next(it)            # patches
    pts = [list(map(float, next(it))) for _ in range(int(next(it)[1]))]
    t = next(it); nf, ni = int(t[1]), int(t[2]); faces = []
    for f in range(nf):
        r = list(map(int, next(it))); k = r[0]
        faces.append((r[1:1 + k], r[1 + k], r[2 + k] if f < ni else None))
    cells = []
    for _ in range(int(next(it)[1])):
        r = next(it); cells.append((int(r[0]), list(map(float, r[2:2 + dim]))))
    return dim, pts, faces, cells

def normal(dim, pts, v):
    if dim == 2:
        a, b = pts[v[0]], pts[v[1]]; return [b[1] - a[1], -(b[0] - a[0])]
    n = [0.0, 0.0, 0.0]
    for i in range(len(v)):
        p, q = pts[v[i]], pts[v[(i + 1) % len(v)]]
        n[0] += (p[1]-q[1])*(p[2]+q[2]); n[1] += (p[2]-q[2])*(p[0]+q[0]); n[2] += (p[0]-q[0])*(p[1]+q[1])
    return n

dim, pts, faces, cells = read(sys.argv[1]); out = {"same": [], "interface": []}
for v, o, nb in faces:
    if nb is None: continue
    n = normal(dim, pts, v); d = [x - y for x, y in zip(cells[nb][1], cells[o][1])]
    nn, dd = math.hypot(*n), math.hypot(*d)
    if nn and dd:
        c = min(1.0, abs(sum(x*y for x, y in zip(n, d))) / (nn*dd))
        out["interface" if cells[o][0] != cells[nb][0] else "same"].append(math.acos(c))
for k, a in out.items():
    print(k, len(a), f"max {max(a):.3e}" if a else "", f"mean {sum(a)/len(a):.3e}" if a else "",
          f">0.01rad {sum(x > 0.01 for x in a)}")
```

**Linha de base (A02, `vmm-mesh` 1.1.0, nuvem):**

| Caso (2D, sítios uniformes 0,05, `interface_pairs = 0.05`) | Faces de interface > 0,01 rad |
|---|---|
| Interface reta, sem pares | 50 de 50 |
| Interface reta, com pares | 4 de 22 |
| Interface reta, com pares + CVT (10 it.) | 38 de 40 |
| Interface circular r = 0,3, com pares | igual a sem pares: 157 de 160 (nenhum par gerado) |

## Anexo B — Casos de reprodução

**B.1 Rio em U (células fragmentadas; CVT não convexo):** polígono `0 0, 10 0, 10 2.1, 0 2.1, 0 1.1, 9 1.1, 9 1.0, 0 1.0`; `sites = grid`, espaçamento 0,75 (origem em meio espaçamento). Na A04, 12 de 27 células saíram fragmentadas, com os invariantes aprovados.

**B.2 Obstáculo fino alinhado (VMM-402):**

```
# dike_a.cfg — falha
dimension = 2
formats = vmesh
[region river]
shape = polygon
xy = 0 0  10 0  10 2.1  0 2.1  0 1.1  9 1.1  9 1.0  0 1.0
sites = grid
sites.spacing = 0.5
sites.origin = 0.25 0.25
```

```
# dike2d_hole.cfg — falha; deslocar o buraco 0,01 ou mudar a origem da grade faz passar
dimension = 2
formats = vmesh
[region block]
shape = rectangle
lo = 0 0
hi = 4 2.1
sites = grid
sites.spacing = 0.5
sites.origin = 0.25 0.25
[hole wall]
shape = rectangle
lo = 0.6 1.0
hi = 3.4 1.1
```

```
# dike3d.cfg — falha ("face seen by one cell only")
dimension = 3
formats = vmesh
[region block]
shape = cuboid
lo = 0 0 0
hi = 2 2 2.1
sites = grid
sites.spacing = 0.5
sites.origin = 0.25 0.25 0.25
[hole slab]
shape = cuboid
lo = 0.3 0.3 1.0
hi = 1.7 1.7 1.1
```

**B.3 MDT alinhado (VMM-202):** base 2D no quadrado unitário com `CartesianGridSource(0.1, {0.05, 0.05})`; `HorizonGrid` com x = y = k·0,1 (k = 0…10), horizonte inferior 0 e superior 1 + 0,01·(i mod 7). Falha com "rounded overlay triangle is degenerate". Repetir com deslocamentos de 10⁻¹⁵, 10⁻¹³ e 10⁻⁹ nos nós internos.

**B.4 Escala das colunas:** base em grade de espaçamento 1/n no quadrado unitário, horizontes planos z = 0 e z = 1, fração `0 0.5 1`. Linha de base: n = 64 (3969 colunas) = 16,6 s; n = 128 (16 129 colunas) = 264 s.

**B.5 CVT:** quadrado unitário, `sites = uniform`, espaçamento 1/64 (7164 células), `cvt_iterations = 200` → não converge com a tolerância padrão (21 s). 3D: cubo unitário, espaçamento 0,07 (7565 células), 3 iterações = 34,7 s (geração 7,45 s).