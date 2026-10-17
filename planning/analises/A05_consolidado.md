# A05 — Consolidado das análises e plano de ação

- **Data:** 2026-10-16
- **Objeto:** VMM no `main`, commit `fb02bb7` (08/10/2026); 1.0.0 declarada em 30/09, sem tag; 1.1.0 preparada, não publicada.
- **Fontes:** A01 SIPOC · A02 FMEA · A03 GUT · A04 revisão adversarial (agente independente, sem acesso a A01–A03).
- **Ambiente:** nuvem, fora do WSL oficial (AGENTS.md). Vale como evidência do commit publicado, não da árvore local do João.
- **Convenção:** [F] fato verificado · [I] inferência · [R] recomendação.

## Resumo

1. **O núcleo 1.0 está sólido.** [F] Build Release sem avisos; 324/324 testes; `check_requirements` sem problemas; CI verde em `fb02bb7`. Os invariantes ficam em 10⁻¹⁵ e as faces internas a uma região são ortogonais em ~10⁻⁸ rad. B1: 10⁶ células 2D em 16 s e 673 B/célula, dentro da meta da DEC-020. 3D, UTM e grades cartesianas 3D passaram nos ataques.
2. **A 1.1 não está pronta para a tag.** Colunas e CVT, os recursos pensados para o MohidNG, funcionam em casos pequenos, mas:
   - não escalam: custo quadrático nas colunas; CVT com reconstrução completa a cada tentativa;
   - perdem a ortogonalidade: nas faces laterais das colunas (até 0,27 rad), nas interfaces e depois do CVT;
   - entram na API estável sem nenhuma DEC.
3. **Falhas duras reproduzíveis** em entradas comuns no uso ambiental: dique fino alinhado com o bissetor de sítios em grade; MDT alinhado à base com espaçamento 0,1.
4. **Build e publicação:** o CMake mínimo declarado (3.28) não configura; não há tags nem releases; LTO e `-march=native` estão ligados por padrão; a matriz da CI fica aquém do prometido.
5. **Governança:** as decisões da 1.1 estão fora do `DECISIONS.md`, os relatórios contradizem o código, e a evidência citada (logs em `build/`) não é verificável por terceiros.
6. **O que falta para o objetivo:**
   - contrato de leitura com o MohidNG;
   - colunas em escala de 10⁵–10⁶;
   - espessura mínima de camada;
   - CVT que preserve a graduação e as colunas;
   - robustez em entradas degeneradas;
   - publicação versionada.

**Veredito:** não marcar a 1.1 como está. As duas análises independentes (A02 e A04) chegaram à mesma conclusão. Primeiro as fases 0 e 1 abaixo; a fase 2 decide se a 1.1 sai com colunas e CVT estáveis ou como experimentais.

---

## 1. Convergência das análises

Os problemas encontrados por métodos diferentes, de forma independente, têm confiança maior.

| Problema | SIPOC | FMEA | GUT | A04 | Confiança |
|---|---|---|---|---|---|
| Colunas com custo quadrático | — | F23 (288) | G04 (80) | B1, I1 | **alta** (medido nas duas: expoente 1,97 e 1,9) |
| Decisões da 1.1 fora do registro; API congelada sem DEC | S6 | F07 (240) | G05 (75) | B2 | **alta** |
| Contrato e lacunas para o MohidNG | S1 | F27 (392) | G01 (100) | I17, perguntas 1–4 | **alta** |
| CVT e colunas não se compõem | S2 | F22 (126) | G06 (64) | I6 | **alta** |
| CVT não escala | — | F19, F21 (192, 144) | G11 (36) | I5 | **alta** (medido nas duas) |
| Critério de convergência do CVT inalcançável | — | F20 (108) | G11 | M10 | **alta** |
| CMake 3.28 não configura | — | F01 (392) | G07 (60) | I8 | **alta** |
| Sem tags e releases; `main` como "1.1.0 não publicada" | S5 | F04 (162) | G03 (80) | I11 | **alta** |
| Evidência não verificável | — | F06 (315) | G09 (36) | I13 | **alta** |
| CI aquém da R12 | — | F02 (150) | G13 (27) | I9 | **alta** |
| LTO e `-march=native` por padrão | — | F03 (150) | G14 (27) | I10 | **alta** |
| Células fragmentadas sem tratamento | — | F16 (96) | G15 (27) | I7 (aviso descartado; 12/27 num rio em U) | **alta** |
| Verificador R1/R4 pula os headers do topo | — | F08 (192) | G18 (18) | M1 | **alta** |
| Oráculo de regressão só 2D | — | F29 (120) | G16 (24) | I15 | **alta** |
| Células finas nas colunas, sem espessura mínima | — | F24 (210) | G12 (36) | I17 | **alta** |
| Estilo inconsistente na 1.1 | — | F31 (48) | G21 (9) | M2 | **alta** |
| Pares espelhados ausentes em interfaces curvas, sem aviso | — | F13 (448) | G02 (80) | — | média (só A02; medido) |
| CVT desfaz a ortogonalidade dos pares | — | F17 (392) | G02 | — | média (só A02; medido) |
| CVT de densidade uniforme desfaz a graduação | S8 | F18 (336) | G10 (36) | — | média ([I], não medido) |
| Commits no `main` com a CI falhando | S7 | F05 (140) | G08 (48) | parcial (I13) | média |
| **Nas colunas, `site` é o centroide; faces laterais até 0,27 rad** | — | — | — | **B3** | média (só A04; conferido no código: `facade/layered.cpp:105,109`) |
| **`column_overlay.hpp` "interno" fica estável pela DEC-041** | — | — | — | **B2** | média (conferido: `column_overlay.hpp:17,34`; `docs/api.rst`) |
| **Obstáculo fino alinhado derruba 2D e 3D (VMM-402)** | — | — | — | **I4** | média (reproduzido aqui com `dike_a.cfg`) |
| **MDT alinhado a base de espaçamento 0,1 derruba as colunas (VMM-202)** | — | — | — | **I3** | média (só A04) |
| **Explosão de faces nas colunas (até 110 faces/célula)** | — | — | — | **I2** | média |
| **`read_native` deixa escapar `std::bad_alloc`** | — | — | — | **I14** | média (conferido: `io/native.cpp:200`) |
| **Licença efetiva GPL para quem gera malha** | — | — | — | **I16** | média |
| Relatórios contraditórios ("CVT não implementado", "sem commit") | — | — | — | I12 | média |
| Tolerâncias e UTM; DEC-028 vs. fusão por proximidade; embutir com `add_subdirectory`; `.at()` no caminho quente; `LayeredMesh{}` | — | — | — | M3–M9 | baixa a média |

---

## 2. Plano de ação

Cada ação tem origem, critério de pronto e momento. Para os itens novos da A04, a prioridade foi atribuída com as escalas da A03.

### Fase 0 — Higiene de build, publicação e processo (horas a 2 dias; sem mudar a geometria)

| # | Ação | Origem | Pronto quando |
|---|---|---|---|
| 0.1 | Subir `cmake_minimum_required` para 3.29 **ou** trocar o `try_compile` de `VerifyCGALHeaders.cmake:46-50` por includes sem alvo importado | F01, I8 | configura com o CMake mínimo; job de CI com o mínimo |
| 0.2 | Identificar o commit da 1.0.0, marcar `v1.0.0` e criar a release; `main` passa a `1.1.0-dev`; site publicado por tag, ou com aviso "em desenvolvimento" | F04, I11 | `git ls-remote --tags` lista `v1.0.0`; site coerente |
| 0.3 | Proteger o `main`: PR com `vmm-ci` obrigatório | F05 | nenhum push direto |
| 0.4 | Verificador R1/R4 cobrindo `cvt.hpp`, `layered.hpp` e `vmm.hpp` (módulo `facade`) | F08, M1 | o verificador acusa um header de teste sem `ut_` |
| 0.5 | `read_native` limitando as contagens ao tamanho do arquivo; teste negativo | I14 | o teste com `points 4·10¹²` devolve `Result` com erro |
| 0.6 | `vmm-mesh` instala um sink de log e imprime `fragmented_cells`; opção de falhar ao fragmentar | F16, I7 | aviso visível no CLI; teste |
| 0.7 | Reconciliar `planning/horizontes` e `planning/cvt` com o código; seção "Estado em <commit>" | I12 | nenhum relatório contradiz `HEAD` |
| 0.8 | Registrar os checksums de B1–B7 por plataforma num arquivo versionado | F06, I13 | arquivo em `planning/` ou `tests/data/` |

### Fase 1 — Decisões antes da tag da 1.1

| # | Decisão a registrar (DEC-043 em diante) | Origem |
|---|---|---|
| 1.1 | Horizontes e colunas: `HorizonGrid`, região = região da base × intervalo, `.hgrid`, `.vlayers` (e se ele é estável na 1.x) | B2, F07 |
| 1.2 | Estado de colunas e CVT na 1.1: estáveis, ou `vmm::experimental` / "sem garantia na 1.1" | B2 |
| 1.3 | `column_overlay.hpp` na lista de internos, ou movido para `src/` ou `vmm::detail` | B2 |
| 1.4 | Semântica de `site` nas colunas: guardar o gerador da base (recomendado) ou declarar o centroide; teste de orientação de Voronoi só onde se aplica | B3 |
| 1.5 | Fluxo recomendado para colunas + CVT: CVT da base 2D e depois extrusão (padrão do CLI com `horizons`); CVT volumétrico opcional, partindo da malha de entrada | F22, I6 |
| 1.6 | Execuções autônomas "sem aprovação intermediária": quando são permitidas e como são revisadas | F07, I13 |
| 1.7 | LTO e `-march=native` por padrão (revisar a DEC-034, porque o ganho medido no P07 ficou dentro da variação) e a R24 da linha de base | F03, I10 |
| 1.8 | Contrato com o MohidNG (com a equipe): formato e leitor, tamanho-alvo (colunas × camadas, tempo aceitável), uma face entre camadas por coluna, z-level com corte pelo fundo, espessura mínima, licença GPL do backend | F27, I16, I17 |

### Fase 2 — Correções técnicas que decidem a 1.1

| # | Ação | Origem | Pronto quando |
|---|---|---|---|
| 2.1 | Colunas: classificação por propagação na CDT ou índice espacial; patch levado pela restrição; CDT só com as células do MDT de cada coluna | F23, B1 | 10⁵ colunas abaixo de uma meta registrada (ex.: < 2 min); caso no `vmm_benchmark` |
| 2.2 | Consultas de colunas em CSR (`cells_in_column`, `vertical_neighbours` em O(1) por célula) | I1 | `std::span`; teste de custo |
| 2.3 | Fusão de faces coplanares com o mesmo par de células; modo "um plano por coluna" | I2 | horizonte plano gera 1 face por coluna e por interface de camada |
| 2.4 | Pares espelhados ao longo da polilinha da interface (não por segmento), com aviso quando faltarem; quinas e pontas | F13, F14 | círculo e isóbata sintética com faces de interface < 10⁻² rad |
| 2.5 | Robustez: empate bissetor × segmento (obstáculo fino) e snap rounding no overlay das colunas; mensagens de erro próprias | I3, I4, M11 | testes de propriedade com grades, diques finos e MDT alinhado (h = 0,1; 0,3; 1/3) |
| 2.6 | Espessura mínima e tratamento das células finas nas colunas (opções declaradas) | F24, I17 | relatório de espessura e inclinação; teste com pinch-out |

### Fase 3 — Próximo ciclo (1.2)

| # | Ação | Origem |
|---|---|---|
| 3.1 | CVT com densidade do `SpacingField`; pares fixos ou acoplados; sítios fixos | F17, F18 |
| 3.2 | CVT escalável: índice espacial no suporte, passo por sítio com projeção, critério RMS ou por energia, aceleração (Anderson ou L-BFGS) | F19–F21, I5, M10 |
| 3.3 | CI: GCC e Clang mais recentes; CMake e CGAL mínimos; dependências Python fixadas; sanitizadores com a suíte completa em agendamento | F02, I9 |
| 3.4 | Embutimento amigável: padrões condicionados a `PROJECT_IS_TOP_LEVEL`; aviso ou recusa no `install` com native/LTO | M7, F03 |
| 3.5 | Golden files próprios (multirregião, 3D, colunas) e `ctest -L perf` com limites | F29, I15 |
| 3.6 | `.clang-format`, `Id<Tag>` no lugar de `std::size_t`, cabeçalhos `File/Description` nos arquivos da 1.1 | F31, M2 |
| 3.7 | Tolerâncias com max(L, \|x\|·ε) para coordenadas UTM; corrigir o texto da DEC-028 ou a fusão por proximidade | M3, M4 |
| 3.8 | `vmm-mesh`: fonte `adaptive` e polígono por arquivo | F10, F11 |
| 3.9 | Metadado de CRS e saída binária (se o contrato do MohidNG pedir) | I17 |

### Backlog

Degenerescência maciça 3D (F15) · paralelização do recorte (F30) · versão do `gcovr` (F09) · identificador comum `.vmesh`/`.vlayers` (F28) · `.at()` no caminho quente (M8) · `LayeredMesh{}` (M9) · determinismo entre plataformas (M6) · entradas geoespaciais fora do núcleo (F12).

---

## 3. Critério para marcar a 1.1

- [ ] Fase 0 completa.
- [ ] DECs 1.1 a 1.6 registradas e aprovadas; 1.7 e 1.8 decididas ou explicitamente adiadas.
- [ ] Se colunas e CVT forem **estáveis**: 2.1–2.6 feitos, com metas de escala registradas. Se forem **experimentais**: só 2.4 e 2.5, porque falham também fora das colunas.
- [ ] Suíte completa (não dirigida) com GCC, Clang e sanitizadores, com logs como artefatos da CI.
- [ ] Relatório final da 1.1 reconciliando P21–P29 e C01–C05 com `HEAD`.

---

## 4. Decisões pendentes para o João

1. **Colunas e CVT na 1.1:** (a) estáveis, depois da fase 2 inteira; (b) experimentais, publicando a 1.1 depois das fases 0–1 e dos itens 2.4–2.5. **Recomendação: (b)**, porque deixa a API aberta até o contrato do MohidNG fechar.
2. **`site` nas colunas:** (a) guardar o gerador da base; (b) manter o centroide e documentar. **Recomendação: (a)**, porque preserva o fluxo de dois pontos ortogonal que justifica o Voronoi.
3. **Padrão de build:** (a) manter LTO e native (DEC-034); (b) padrão portátil, com as opções rápidas opt-in. **Recomendação: (b)**: o ganho medido não justifica o custo para o MohidNG e para pacotes.
4. **CMake mínimo:** (a) subir para 3.29; (b) corrigir a verificação e manter 3.28. **Recomendação: (b) se for simples, senão (a)**, com job de CI nos dois casos.
5. **Commit da 1.0.0 para a tag:** [F] `75d42ae` a `8191e26` declaram `VERSION 1.0.0`, e o `8191e26` já traz o planejamento dos horizontes. **Recomendação:** `da5c03e` (01/10), o último commit 1.0.0 com a CI verde e antes do trabalho da 1.1. Precisa da sua confirmação.
6. **Execuções autônomas:** voltar a exigir aprovação por etapa, ou manter a autonomia com revisão no fim e evidência versionada. **Recomendação:** autonomia só com logs como artefato e a suíte completa no fim.
7. **Reunião com a equipe do MohidNG** para o item 1.8 (perguntas 1–4 e 13 da A04): marcar antes da fase 2.
