# P05a — Plano de iterações (provas de conceito)

- **Data:** 2026-09-28
- **Sequência:** v5.2, prompt P05a, iteração P05a.0 (plano, sem código)
- **Destino no repositório:** `planning/P05a_plano.md`
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação · **NÃO VERIFICADO**.
- **Estado:** aguarda aprovação do João. Nenhum código foi escrito.

---

## 1. Ambiente

| Item | Valor |
|---|---|
| Onde o código é escrito | sessão do Claude na nuvem |
| Onde vale a evidência | WSL `Ubuntu-26.04-Test` (AGENTS.md): o João roda um roteiro por iteração e cola a saída |
| Versões esperadas no WSL | g++ 15.2, CMake 4.2.3, CGAL 6.2.1 (release), Boost 1.92 (P03 v2.2) |
| Versão do CGAL na nuvem | 5.6 **[F]**; serve só para compilar antes de entregar, não como evidência |
| Repositório lido | GitHub, commit `cd0302e` |

---

## 2. Objetivo e limites

**Objetivo:** reduzir o risco do P06 respondendo, com código curto e descartável, a cinco perguntas:
1. Uma malha 2D com duas regiões, não convexa e com buraco, satisfaz os invariantes da DEC-011?
2. Qual das duas estratégias de interface conforme funciona melhor (§3.2)?
3. O firewall de compilação da DEC-007 funciona na prática com CMake?
4. A adjacência compacta (tipo CSR) basta para montar o padrão de uma matriz esparsa (DEC-015)?
5. O modelo de dados aguenta 3D sem mudança de forma (prova 3D mínima)?

**Limites**
- Tudo em `prototypes/P05a/`, com CMake próprio, **fora** do build principal. Nenhum arquivo de `VMMLib/`, `tests/`, `cmake/` ou do `CMakeLists.txt` raiz é alterado.
- Protótipo não é código de produção: não entra na biblioteca, não tem API estável e não precisa de teste por classe (R1, R2, R25). Ainda assim respeita o R3 (sem virtual, sem herança, sem enum de despacho) e usa tolerância relativa (R17), para não validar uma forma que o P06 teria de proibir. Regra proposta na DEC-027.
- Cada iteração termina com um executável que imprime os invariantes e sai com código ≠ 0 se algum falhar.
- Limite de três tentativas por iteração (protocolo, item 5).

---

## 3. Iterações

### P05a.1 — Firewall de compilação e adjacência compacta (tarefas 3 e 4)

**Arquivos (~300 linhas)**
- `prototypes/P05a/CMakeLists.txt`: dois alvos, `p05a_core` (sem CGAL) e `p05a_backend_cgal`; um alvo de verificação que compila cada header público **sem** o CGAL no caminho de includes.
- `prototypes/P05a/include/p05a/mesh.hpp`: `Real = double`, IDs fortes, adjacência em CSR (`offsets`, `neighbours`).
- `prototypes/P05a/src/backend_cgal/delaunay_pairs.cpp`: único arquivo que inclui o CGAL; devolve os pares vizinhos da triangulação de Delaunay 2D.
- `prototypes/P05a/apps/p05a_1_csr.cpp`: monta a CSR a partir dos pares e, dela, o padrão de uma matriz esparsa (número de não nulos por linha), sem geometria.

**Critério:** o alvo de verificação compila sem o CGAL; o padrão da matriz é simétrico e igual ao calculado direto dos pares.

### P05a.2 — Prova 2D multirregião (tarefa 1)

**Caso:** quadrado de 1 × 1 com um furo quadrado central de 0,2 × 0,2 e duas regiões separadas por uma interface em "L" (não convexa). Sítios pseudoaleatórios com semente fixa, estritamente no interior de cada região.

**Duas estratégias de interface, lado a lado:**

| | E1 — Voronoi por região + refinamento comum | E2 — Sítios espelhados |
|---|---|---|
| Como | cada região gera o seu Voronoi só com os seus sítios e o recorta pelo próprio polígono; as faces da interface são subdivididas nos pontos de quebra dos dois lados | os sítios próximos da interface são refletidos para o outro lado; a interface cai sobre bissetrizes |
| Conformidade | por construção (cada subsegmento tem uma célula de cada lado) | depende da vizinhança dos espelhos; pode exigir corte como garantia |
| Ortogonalidade na interface | perdida (d_ij não é normal à face) | preservada onde o espelho funciona |

**Arquivos (~450 linhas):** recorte convexo por semiplanos (célula de Voronoi), recorte pelo polígono da região com o CGAL (`Boolean_set_operations_2`, só no alvo do backend), montagem da topologia e verificador de invariantes.

**Critério:** para as duas estratégias, o executável imprime e verifica:
- soma das áreas = área do domínio e de cada região (erro relativo ≤ 10⁻¹²);
- fechamento de cada célula (|Σ S_f| ≤ 10⁻¹² L);
- owner/neighbour consistentes;
- cada face de interface com uma célula de cada região;
- não ortogonalidade máxima nas faces internas e nas de interface.

O mesmo caso roda com o domínio escalado por 10⁻³ e por 10⁶, e com os sítios em 5 ordens diferentes; a topologia (numeração canônica) tem de ser a mesma.

### P05a.3 — Prova 3D mínima (tarefa 2)

**Caso:** caixa 1 × 1 × 1, uma região, sítios pseudoaleatórios. Células por interseção de semiespaços (vizinhos pela Delaunay 3D do CGAL; recorte convexo próprio), recortadas pelas seis faces da caixa.

**Arquivos (~300 linhas):** reusam os tipos de `mesh.hpp` com `Dim = 3`.

**Critério:** soma dos volumes = 1 (erro relativo ≤ 10⁻¹²); fechamento de cada célula; owner/neighbour consistentes; o mesmo `mesh.hpp` serve a 2D e 3D sem ramos por dimensão fora da geometria. Não testa domínio não convexo nem multirregião: isso é do P15a.

### P05a.4 — Relatório (tarefa 5)

`planning/P05a_provas_conceito.md`: ambiente e versões; saída de cada executável; comparação E1 × E2; o que muda no P06; recomendação sobre a estratégia de interface da 0.1 (proposta como DEC).

---

## 4. Roteiro de evidência no WSL

Em cada iteração o João roda um só bloco, que configura, compila e executa `prototypes/P05a/` num diretório de build em `/tmp` e imprime as versões usadas (DEC-012). Nenhum arquivo versionado fora de `prototypes/P05a/` é tocado.

---

## 5. Prazo

**[R]** Quatro iterações. Se a P05a.2 não fechar os invariantes em três tentativas, a iteração para e o relatório registra a falha como resultado; isso já é informação suficiente para o P06.

---

## 6. Decisões pendentes para o João

1. Aprovar este plano (P05a.1 a P05a.4)? Recomendação: aprovar.
2. Aprovar a DEC-027 (regras para `prototypes/`)? Recomendação: aprovar.
3. Prazo de quatro iterações? Recomendação: aprovar.