# A03 — Matriz GUT do VoronoiMeshMaker

- **Data:** 2026-10-16
- **Objeto:** `main` em `fb02bb7` (1.1.0 preparada, não publicada).
- **Entradas:** achados do SIPOC (A01, S1–S8) e modos de falha da FMEA (A02, F01–F31), agrupados em problemas. A revisão adversarial (A04) é independente e entra só no consolidado (A05).
- **Convenção:** [F] fato · [I] inferência · [R] recomendação. Os itens herdam a classificação da origem.

## 1. Escalas

| Nota | G — Gravidade | U — Urgência | T — Tendência |
|---|---|---|---|
| 5 | inviabiliza o uso pretendido (MohidNG, malhas reais) ou dá resultado errado sem aviso | precisa ser resolvido **antes da tag da 1.1**, porque a DEC-041 congela API e formatos | piora rápido; fica muito mais caro depois do congelamento ou da integração |
| 4 | degrada muito o resultado ou bloqueia um grupo de usuários | antes da integração com o MohidNG | piora no médio prazo |
| 3 | retrabalho relevante; contornável | no próximo ciclo de desenvolvimento | piora devagar |
| 2 | incômodo | pode esperar | estável |
| 1 | cosmético | sem prazo | tende a desaparecer |

**GUT = G × U × T.** Faixas: **≥ 64 ação imediata** · 27–63 próximo ciclo · < 27 backlog.

**Critério de urgência:** a referência é a tag da 1.1. Tudo o que muda API, formato ou chave de configuração custa uma versão maior depois do congelamento (DEC-041). Isso puxa para cima a urgência de G02, G05 e G06, mesmo onde a gravidade técnica é a mesma de outros itens.

## 2. Matriz

| # | ID | Problema | Origem | G | U | T | GUT | Momento |
|---|---|---|---|---:|---:|---:|---:|---|
| 1 | G01 | Contrato de leitura do MohidNG inexistente (formato, convenções, leitor de referência) | S1, F27 | 5 | 5 | 4 | **100** | antes da integração |
| 2 | G02 | Ortogonalidade na interface não entregue: pares ausentes em interfaces curvas, falhas em quinas e pontas, CVT desfaz os pares | F13, F14, F17 | 5 | 4 | 4 | **80** | antes da tag 1.1 |
| 3 | G03 | Versões sem tag nem release; o site mostra 1.1 antes da publicação | F04, S5 | 4 | 5 | 4 | **80** | imediato |
| 4 | G04 | Custo quadrático da geração em colunas | F23 | 5 | 4 | 4 | **80** | antes da integração |
| 5 | G05 | Decisões da 1.1 fora do `DECISIONS.md` | F07, S6 | 3 | 5 | 5 | **75** | antes da tag 1.1 |
| 6 | G06 | CVT e colunas não se compõem (o CVT perde as colunas) | F22, S2 | 4 | 4 | 4 | **64** | antes da tag 1.1 |
| 7 | G07 | CMake mínimo declarado (3.28) não configura | F01 | 4 | 5 | 3 | **60** | imediato |
| 8 | G08 | Commits no `main` com a CI falhando | F05, S7 | 3 | 4 | 4 | **48** | imediato |
| 9 | G09 | Evidência dos relatórios não verificável por terceiros | F06 | 3 | 3 | 4 | **36** | contínuo |
| 10 | G10 | CVT com densidade uniforme desfaz a graduação [I] | F18, S8 | 4 | 3 | 3 | **36** | depois da 1.1 |
| 11 | G11 | CVT caro e frágil em escala (reconstrução completa, varredura linear, passo global, tolerância inalcançável) | F19, F20, F21 | 4 | 3 | 3 | **36** | depois da 1.1 |
| 12 | G12 | Células finas e inclinadas nas colunas, sem opção de espessura mínima | F24 | 4 | 3 | 3 | **36** | antes da integração |
| 13 | G13 | Matriz da CI aquém da R12 (sem compiladores recentes e outras plataformas) | F02 | 3 | 3 | 3 | **27** | próximo ciclo |
| 14 | G14 | Pacote padrão não portátil (`-march=native`, LTO) | F03 | 3 | 3 | 3 | **27** | próximo ciclo |
| 15 | G15 | Células fragmentadas sem tratamento configurável | F16 | 3 | 3 | 3 | **27** | próximo ciclo |
| 16 | G16 | Oráculo de regressão só para o 2D de uma região | F29 | 3 | 2 | 4 | **24** | próximo ciclo |
| 17 | G17 | `vmm-mesh` sem espaçamento variável e sem polígono de arquivo | F10, F11, S3 | 3 | 3 | 2 | **18** | próximo ciclo |
| 18 | G18 | Verificador R1 não cobre os headers do topo | F08 | 2 | 3 | 3 | **18** | imediato (barato) |
| 19 | G19 | Degenerescência maciça em 3D | F15 | 3 | 2 | 2 | **12** | backlog |
| 20 | G20 | Desempenho de domínios STL grandes, uma thread | F30 | 2 | 2 | 3 | **12** | backlog |
| 21 | G21 | Estilo inconsistente; sem `.clang-format` | F31 | 1 | 3 | 3 | **9** | próximo ciclo |
| 22 | G22 | Dependência da versão do `gcovr` na medida de cobertura | F09 | 2 | 2 | 2 | **8** | backlog |
| 23 | G23 | Sem entradas geoespaciais | F12, S4 | 2 | 2 | 2 | **8** | backlog |
| 24 | G24 | `.vlayers` antigos convivem com `.vmesh` novos | F28 | 2 | 2 | 2 | **8** | backlog |
| 25 | G25 | Precisão de saída em double; horizontes só z(x,y) | F25, F26 | 2 | 1 | 2 | **4** | backlog |

## 3. Ações por problema

| ID | Ação | Momento |
|---|---|---|
| G01 | especificar `.vmesh`/`.vlayers` para consumidores (índices, sinais, z, profundidade, unidades); leitor de referência mínimo e teste de ida e volta; validar com a equipe do MohidNG | antes da integração |
| G02 | pares ao longo da polilinha (não por segmento) e aviso quando faltarem; tratamento de quinas e pontas; pares como entidade fixa ou acoplada no CVT; relatório por interface | antes da tag 1.1 |
| G03 | identificar e marcar `v1.0.0`; `main` como `1.1.0-dev`; publicar o site por tag | imediato |
| G04 | classificação por propagação topológica ou índice espacial; caso de 10⁵ colunas no benchmark com meta | antes da integração |
| G05 | DECs para horizontes, `.vlayers`, CVT, chaves `cvt_*` e execução sem aprovação intermediária | antes da tag 1.1 |
| G06 | fluxo recomendado: CVT da base 2D e depois extrusão, no C++ e no `vmm-mesh`; documentar | antes da tag 1.1 |
| G07 | mínimo 3.29 ou `try_compile` sem alvo importado; job de CI com o mínimo | imediato |
| G08 | `main` protegido; PR com `vmm-ci` obrigatório | imediato |
| G09 | logs-chave versionados ou artefatos da CI; suíte completa antes de declarar entrega | contínuo |
| G10 | densidade a partir do `SpacingField`; teste de preservação da graduação | depois da 1.1 |
| G11 | índice espacial no suporte; passo por sítio com projeção; tolerância realista e aceleração; benchmark | depois da 1.1 |
| G12 | espessura mínima, remoção ou fusão de células finas, ajuste do fundo; relatório de qualidade por coluna | antes da integração |
| G13 | GCC e Clang mais recentes; declarar plataformas suportadas | próximo ciclo |
| G14 | aviso ou recusa no `install`; preset de pacote | próximo ciclo |
| G15 | contador no `vmm-mesh`; opção de falhar | próximo ciclo |
| G16 | golden files próprios para multirregião, 3D e colunas | próximo ciclo |
| G17 | fonte `adaptive` e `polygon file` no `.cfg` | próximo ciclo |
| G18 | incluir `cvt.hpp`, `layered.hpp` e `vmm.hpp` no verificador | imediato (barato) |
| G19 | recorte convexo exato por célula | backlog |
| G20 | paralelizar o recorte | backlog |
| G21 | `.clang-format` e verificação na CI | próximo ciclo |
| G22 | documentar; recalibrar ao migrar | backlog |
| G23 | manter fora do núcleo (DEC-001); documentar o caminho externo | backlog |
| G24 | identificador comum nos dois arquivos | backlog |
| G25 | manter as rejeições; documentar | backlog |

## 4. Leitura da matriz

**Ação imediata (6 itens, GUT ≥ 64):** G01, G02, G03, G04, G05, G06. Três blocos:
1. **Integração com o MohidNG:** G01 (contrato) e G04 (escala das colunas). Sem eles, o consumidor principal não usa a 1.1 em casos reais.
2. **Interfaces e CVT antes do congelamento:** G02, G05 e G06. São API, chaves e comportamento que a tag congela.
3. **Publicação:** G03. Versão rastreável e site coerente com o que foi publicado.

**Exceção de custo:** G07 (CMake mínimo, 60), G08 (proteção do `main`, 48) e G18 (verificador R1, 18) ficam abaixo da faixa, mas custam horas e protegem as demais correções. Convém fazê-los junto com G03.

**Próximo ciclo (9 itens, 27–63):** G07, G08, G09, G10, G11, G12, G13, G14, G15.

**Backlog (10 itens, < 27):** G16, G17, G18, G19, G20, G21, G22, G23, G24, G25.

**Ordem de execução sugerida [R]:**
1. G07, G08, G18 e G03: ajustes de build e processo (um dia).
2. G05: registrar as DECs da 1.1, que destrava a discussão de G02 e G06.
3. G01 e G06, com a equipe do MohidNG: o contrato define o que precisa ser congelado.
4. G02 e G04: trabalho técnico de maior porte.
5. Só então a tag da 1.1. O resto entra nos ciclos seguintes.
