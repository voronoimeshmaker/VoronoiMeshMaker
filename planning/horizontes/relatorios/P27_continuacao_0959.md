# Continuação P27/P28 — 2026-10-04, execução das 09:59 UTC

Sem commit/push. Somente Ubuntu-26.04-Test e árvore /home/jflavio/Programas/VMM.

## Correção geométrica e testes

Novo caso FixedFiniteObliqueInterfaceIn2DAndExtrusion verifica interface x+y=2,
extremos (0,2)/(2,0), intervalos sem lacunas/sobreposição em 2D e extrusão 3D.
Dois espaçamentos mudam a subdivisão preservando suporte e extensão.
Revelou dois pontos exatos arredondando a (1,1); corrigida a identificação global
dos IDs de saída no overlay. Coluna exata que desaparecer na conversão é rejeitada.
15 testes dirigidos passaram após essa correção; suíte Release 305/305.

Ampliados negativos: truncamentos da persistência, falhas de streams e arquivos,
entradas inválidas do backend e da extrusão, faces duplicadas e lacunas.
LayeredMesh::from_data agora verifica duplicação, fechamento, orientação e
positividade, além de suporte amostrado e metadados. Essa última mudança passou
nos 19 testes de HorizonGrid/Layered em 0.10 s. Suíte completa atual: 309/309 aprovados em 48.59 s,
no log build/horizons_full_tests.log.

## Cobertura

Árvore instrumentada GCC14 Debug em build/horizons-coverage.
Última medição dirigida ANTERIOR à checagem adicional de duplicação/fechamento:

| Arquivo fonte | Linhas | Ramos |
|---|---:|---:|
| backend/cgal/column_overlay.cpp | 96.9% | 80.0% |
| facade/layered.cpp | 94.3% | 81.3% |
| geometry/horizons.cpp | 97.8% | 86.2% |
| io/layered.cpp | 97.4% | 85.2% |
| mesh/layered.cpp | 90.3% | 80.7% |
| reorder/layered.cpp | 100% | 90.0% |

Nenhuma exceção nova adicionada. O alvo global retornou falha porque executamos
apenas testes dirigidos. Uma execução completa instrumentada está em andamento;
ao terminar, o comando já encadeia o alvo vmm_coverage. Precisa recompilar e
executar os testes Layered depois, pois from_data mudou após o início da execução.
Não citar esses números como cobertura final da versão atual de mesh/layered.cpp.

## Desempenho e documentação

Ver P27_desempenho.md e script reproduzível prototypes/P27/benchmark.py.
Amostras com 225/961/3969 colunas e 2/8 camadas. Máximo observado 31752 células;
tempo inclui IO, há concorrência com outros processos, não é limite de capacidade.
Custo aproximadamente quadrático em colunas é risco conhecido da classificação.

Atualizado guia Sphinx e catálogo inglês sem adicionar exemplos antes do gate P27.
Compilação isolada com -E -W passou pt_BR/en; HTML inglês inspecionado.
Logs build/horizons_docs_pt.log e build/horizons_docs_en.log.
Ainda falta construção completa Doxygen/galeria/Sphinx e exemplos após validação.

## Processos e retomada

Antes de iniciar novas execuções, usar ps e ler logs para evitar duplicação:

- Debug ASan/UBSan completo: build/horizons_debug_full.log; iniciado com 304 testes,
  código anterior às correções desta execução. Ainda havia testes lentos ativos.
  Ao terminar, recompilar e rodar os testes dirigidos afetados.
- Cobertura completa: build/horizons_coverage_full.log, seguida pelo alvo
  vmm_coverage em build/horizons_coverage_report.log.
- Clang19 Release: build/horizons_clang_build.log, seguida por testes dirigidos
  em build/horizons_clang_tests.log. Recompilar ao terminar para garantir que
  inclua todas as alterações feitas durante a compilação.
- Release atual: build/horizons_full_tests.log.

Não recompilar uma árvore enquanto CTest usa seus executáveis.
P27/P28/P29 não concluídos. Automação ativa.
