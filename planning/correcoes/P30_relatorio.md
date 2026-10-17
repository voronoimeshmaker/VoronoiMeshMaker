# P30 — Higiene de build, leitores e rastreabilidade

## Ambiente e versões

2026-10-17; WSL Ubuntu-26.04-Test, /home/jflavio/Programas/VMM.
GCC 15.2.0 e 14.3.0; Clang 19.1.7; CMake 4.2.3; CGAL 6.2.1
em /usr/local (NR 1060211000); Boost 1.92.0 em /usr/local;
GoogleTest 1.17.0; Python 3.14.4; Ninja 1.13.2; GMP 6.3.0; MPFR 4.2.2.
CMake 3.28.3 e 4.4.4 instalados em venvs isolados em build/ e verificados.

## Estado e P30.0 — FATO VERIFICADO

HEAD = fb02bb7, sem commits posteriores à análise e sem diff rastreado inicial.
Somente planning/analises/ estava não rastreado: cinco documentos fornecidos
pelo João, copiados e conferidos por SHA-256. Nenhuma correção da P30 já aplicada.
AGENTS.md, requisitos, baseline, DEC-001–042 e A01–A05 consultados.
Não existe etapa anterior desta sequência; o relatório CVT 01_entrega.md foi
consultado para conhecer o estado anterior. Resultados antigos não validam esta entrega.

## Iteração 0 — plano, sem alteração de código

1. P30.1: reproduzir a configuração com 3.28.3; remover a ligação ao alias CGAL
   do teste de header, preservando os requisitos e o overlay; verificar mínimo
   e estável atual e compilar vmm_firewall.
2. P30.2–3: preservar versão numérica e API; acrescentar pré-release; documentação
   de desenvolvimento e Pages restrito a tags; instruções de proteção do main
   e evidência do candidato da 1.0, sem criar tags ou mudar configurações remotas.
3. P30.4: mapear headers da raiz para facade nos verificadores R1/R2/R4;
   permitir app -> facade conforme DEC-040; teste negativo isolado.
4. P30.5: auditar os cinco leitores e suas sobrecargas públicas; validar contagens,
   valores finitos, IDs e final de arquivo; evitar alocações a partir de contagens
   não verificadas; converter exceções em Result. Testes negativos e ida e volta.
5. P30.6: lista estável de fragmentação e rejeição opt-in, sink no CLI e --quiet;
   manter semântica padrão/API 1.0. Testar rio em U e integração .cfg.
6. P30.7–8: notas de estado aditivas nos relatórios; checksums B1–B7 medidos no WSL
   e teste B1 reduzido com identificação de plataforma.
7. Fechamento: suíte completa Release GCC, Debug GCC ASan/UBSan, Release Clang 19
   e cobertura limpa por arquivo; evidências resumidas versionáveis nesta pasta.

Cada iteração técnica deve ter aproximadamente até 500 linhas de produção e
até três tentativas. Não avançar para P31 ao concluir P30 sem aprovação do João.
Decisões D1–D7 não estão automaticamente aprovadas pela autorização de executar.

## Estado de execução

EM ANDAMENTO. Planejamento inicial concluído. Nenhum teste ou benchmark desta
etapa executado ainda; nenhuma alegação de cobertura ou correção concluída.

## Decisões pendentes

Nenhuma necessária para iniciar P30. As decisões da P31 continuam reservadas
à aprovação do João. Commits, tags, push e proteção remota são ações do João.

## Progresso técnico — FATO VERIFICADO

P30.1: falha original reproduzida no CMake 3.28.3. O alias de alvo importado
não é suportado por LINK_LIBRARIES de try_compile antes de 3.29; isso é mudança
de funcionalidade, não uma política que possa ser ativada no 3.28. CMP0137 trata
propagação de variáveis na assinatura de projeto e não resolve este alias.
Fontes: [try_compile 3.29](https://cmake.org/cmake/help/v3.29/command/try_compile.html),
[CGAL 6.2.1](https://github.com/CGAL/cgal/blob/v6.2.1/Installation/cmake/modules/CGAL_SetupCGALDependencies.cmake),
[CMake estável 4.4.4](https://cmake.org/download/).
A primeira tentativa de correção encontrou expressão COMPILE_LANGUAGE nas flags;
a segunda resolveu a guarda CXX. Configuração e vmm_firewall passaram com
3.28.3 e 4.4.4. Mínimo 3.28 preservado; sem ligação ao alvo CGAL no probe.

P30.2–3: sufixo dev, teste de versão numérica/sufixo, READMEs e banner Sphinx
alterados. Workflow gera artefato em main/PR e restringe deploy a push de tag v*.
Guia de proteção de main em CONTRIBUTING; configuração remota não modificada.
Candidato da 1.0 da5c03e confirmado pelo CMake e header; CI histórica ainda
não verificada. Não criada tag.

P30.4: headers da raiz são facade; app -> facade já é autorizado pela DEC-040
(aplicação sobre a fachada), não muda arquitetura. Agregados MeshRequest3D e
MeshResult3D incluídos na lista de triviais. Controles negativos demonstram
R1 para header raiz, aceitação do teste espelhado e R4 core -> facade proibido.
CTest dirigido: 4/4, 0,18 s (Version e verificadores). Não substitui suíte completa.

P30.7: notas de estado aditivas nas sequências e relatórios P27/P29 disponíveis.
P30.5, P30.6, P30.8 e validação integral ainda pendentes. P30 não concluída.

### Iteração de leitores

FATO VERIFICADO: 33/33 testes dirigidos (Native, Stl, MeshConfig, Layered), 0,09 s.
Contagens nativas deixam de produzir resize(n); limites de IDs/bytes e leitura
incremental. NaN/Inf, lixo final, overflow de contagem de vértices e site ID
inválido rejeitados. Leitores públicos convertem exceções padrão; catálogo 504
proposto na DEC-043. STL exige a estrutura completa de facetas e valores finitos.
A garantia contra falta de memória cobre a execução interna dos leitores; não
é uma garantia sobre alocações feitas pelo chamador antes da chamada.

### Iteração de fragmentação e checksums — em implementação

Lista aditiva de IDs canônicos nos resultados, reject_fragmented opt-in,
código 404 e chave de configuração; sink de log e --quiet no executável.
A lista acompanha o último iterado aceito do CVT. Teste de rio em U adicionado.
Build Release completo em andamento; nenhuma conclusão desta iteração ainda.
Checksums B1–B7 e quatro suítes continuam pendentes.

### Candidato 1.0.0

FATO VERIFICADO: a API pública do GitHub retornou oito checks com sucesso para
`da5c03eab5865e14fe7ab7ed1ff29d529cd80d79`, incluindo GCC, Clang, sanitizadores,
cobertura, static-checks, docs build/deploy. Evidência: `evidencias/P30/ci_da5c03e.json`.
Isso valida somente esse commit publicado, não a árvore da P30.
Após confirmar D5, comandos reservados ao João:

```bash
git tag -a v1.0.0 da5c03eab5865e14fe7ab7ed1ff29d529cd80d79 -m "VoronoiMeshMaker 1.0.0"
git push origin v1.0.0
```

### Validação integral em curso

Build Release GCC concluído antes dos testes adicionais de injeção de falha.
A iteração desses testes teve duas falhas de compilação: inlining de delete
produziu -Wmismatched-new-delete; macro GTest sem chaves produziu -Wdangling-else.
Corrigidos no suporte de testes (noinline e chaves), sem relaxar -Werror.
Terceira tentativa: build passou, suíte completa Release iniciada.
Builds Debug ASan/UBSan GCC14, cobertura GCC14 e Release Clang19 em andamento.
Os .gcda antigos serão removidos somente da pasta de cobertura antes da execução
limpa integral; avisos de perfis antigos durante descoberta não são cobertura final.
