# P27 — validação consolidada

2026-10-04. Exclusivamente Ubuntu-26.04-Test, /home/jflavio/Programas/VMM.
Sem commit, tag ou push. Logs em build/ pertencem a esta árvore.

## Matriz executada

- GCC Release: 309/309 antes dos exemplos; pipeline documental com exemplo CLI:
  310/310. O log final é build/horizons_docs_full.log.
- GCC14 Debug ASan/UBSan: 304/304 em 3791.77 s na versão inicial; depois das
  correções do overlay e validação de importação, recompilação e 25/25 testes
  dirigidos afetados em 3.53 s. Não se afirma uma segunda suíte completa de 310
  testes com sanitizadores. Logs horizons_debug_full.log e horizons_debug_tests.log.
- Clang19 Release: recompilação e 25/25 testes dirigidos em 0.10 s.
  A primeira execução estava com biblioteca anterior à checagem de faces
  duplicadas; a recompilação eliminou essa falha. Log horizons_clang_tests.log.
- Cobertura GCC14: 308/308 testes instrumentados completos, depois recompilação
  e 25/25 dirigidos para incorporar o último teste e a validação de fechamento.
  gcovr: 97.4% linhas, 90.3% ramos, 98.4% funções; R25 zero arquivos reprovados
  considerando as exceções históricas do projeto. Nenhuma nova exceção adicionada.
- check_requirements: zero problemas. Firewall integra a suíte. git diff --check
  sem erros no momento de revisão.

## Cobertura da extensão

| Fonte | Linhas | Ramos |
|---|---:|---:|
| backend/cgal/column_overlay.cpp | 96.9% | 80.0% |
| facade/layered.cpp | 94.3% | 81.3% |
| geometry/horizons.cpp | 97.8% | 86.2% |
| io/layered.cpp | 97.4% | 85.2% |
| mesh/layered.cpp | 90.9% | 80.9% |
| reorder/layered.cpp | 100% | 90.0% |

## Aceitação geométrica

Planos, oblíquos, superfícies facetadas, frações graduadas, desaparecimento dentro
da célula base, intervalo totalmente ausente, concavidade, buraco e ilha.
Regiões laterais cruzadas por horizontes, reta oblíqua 2D com extremos e cobertura
por intervalos verificados; plano extrudado 3D; horizontes com sítios diferentes.
Não é teste de CVT, que não está implementado.

Negativos: inversão, NaN/Inf, cotas/listas/IDs inválidos, referência fora da base,
translação que preserva volumes mas viola suporte, região incorreta, face duplicada,
lacuna e persistência truncada. Repetição determinística e escalas 0.001/1/1000
com translação 0/100; renumeração RCM e inversa; equivalência C++/CLI por cmp
dos arquivos .vlayers do exemplo plano.

A correção do centro de faces usa momentos relativos ao primeiro vértice.
A correção do overlay identifica globalmente pontos exatos que convertem para a
mesma coordenada double; rejeita perda completa de coluna representada.

## Desempenho

Ver P27_desempenho.md. Três tamanhos de base e duas quantidades independentes de
camadas. Até 31752 células, 135954 faces, 36864 pontos e 65720 KiB observados.
Medição inclui IO e teve concorrência; não é garantia de capacidade ou latência.
O custo aproximadamente quadrático da classificação é a principal limitação
de escalabilidade, sem meta imposta pelo usuário.

## Limites da evidência

Cobertura não prova correção para todas as geometrias. Testes de suporte e
extensão cobrem casos sintéticos, não uma prova de conformidade de malhas
arbitrariamente editadas. Validação da fábrica combina amostragem, metadados,
duplicações e invariantes. Grades curvas são aproximações afins fixadas na entrada.
