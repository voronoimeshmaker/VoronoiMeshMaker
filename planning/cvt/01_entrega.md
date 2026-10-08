# CVT — conclusão técnica

Data: 2026-10-08. Ambiente exclusivo: WSL `Ubuntu-26.04-Test`,
`/home/jflavio/Programas/VMM`. C01–C05 concluídos no escopo de
[00_sequencia.md](00_sequencia.md). Sem commit, tag ou push pelo agente.
A retomada encontrou as alterações anteriores já registradas pelo usuário em
`42750c8`; as verificações finais abaixo usam essa árvore e os testes novos.

## Resultado

- `optimize_cvt`: Lloyd de densidade uniforme em 2D/3D, partição regional fixa,
  busca de passo, energia, resíduo e estados distintos de convergência,
  estagnação e limite de iterações.
- `cvt_domain`: extração das regiões presentes de `LayeredMesh`, preservando
  interfaces e patches e fornecendo o mapa `source_region`.
- API C++ e configuração: o CVT de horizontes reinicia sítios volumétricos
  reproduzíveis por `seed`, mantendo a contagem por região. Devolve `Mesh3D`
  em vmesh/VTU, sem metadados de colunas nem novo arquivo vlayers.
- Testes da API verificam controles inválidos, energia analítica, resíduo
  inicial/final, limite de iterações e estagnação por redução não resolvida.
  A integração verifica determinismo, interfaces oblíquas, não convexidade,
  horizontes com desaparecimento parcial, formatos e configuração.
- Guias pt/en, README e changelog reconciliados com o comportamento executado.
  Exemplos `cvt` e `cvt_horizons` executados pela galeria após os testes.

## Evidências de validação

Todos os caminhos abaixo são relativos à raiz ativa. Logs ficam em `build/`.

| Verificação | Resultado | Evidência |
|---|---|---|
| Release GCC 15.2, build completo | aprovado | `build/cvt_final_build.log` |
| Suíte Release final | 324/324; 25,17 s | `build/cvt_final_tests.log` |
| Suíte instrumentada completa GCC 14 | 322/322; 822,19 s | `build/cvt_completion_coverage_tests.log` |
| CVT instrumentado após os testes novos | 13/13, incluindo os 2 testes novos da API | `build/cvt_final_coverage_tests.log` |
| R25 por arquivo | aprovado, nenhuma nova exceção | `build/cvt_final_coverage_report.log` |
| Debug ASan/UBSan GCC 14, CVT | 13/13 | `build/cvt_completion_debug_tests.log` |
| Build completo Clang 19 | aprovado | `build/cvt_completion_clang_build.log` |
| CVT Clang 19 | 13/13 | `build/cvt_final_clang_tests.log` |
| Doxygen/Sphinx pt_BR e en com `-E -W` | ambos sem avisos | `build/cvt_completion_docs.log` |
| Instalação isolada e consumidor CMake 1.1 | compilação e execução aprovadas | `build/cvt_completion_install.log`, `build/cvt_completion_consumer.log` |
| Requisitos estruturais e espaços | zero problemas | `check_requirements.py`, `git diff --check` |

Cobertura agregada: **97,3% linhas / 90,0% ramos / 98,3% funções**.
`vmm/src/facade/cvt.cpp`: **94,9% linhas / 81,5% ramos**.
A exceção já aprovada de `facade/facade.cpp` permanece; não foram adicionadas
exceções nem alterados os limites de 90%/80%. As cinco falhas fora de CVT do
relatório anterior desapareceram com a execução da suíte instrumentada completa.
O perfil do arquivo de testes modificado foi atualizado pelo gcov; os perfis
das fontes da biblioteca foram preservados, sem mudança de fonte entre a suíte
completa e a execução adicional de CVT.

O escopo de sanitizadores e Clang nesta entrega é explicitamente CVT; os
324 casos da suíte completa foram executados em Release GCC. A cobertura usa
a suíte completa anterior mais a execução atualizada dos 13 casos de CVT.

Comandos de reprodução (executar dentro da distribuição indicada):

```bash
cd /home/jflavio/Programas/VMM
cmake --build build -j 3
ctest --test-dir build -L vmm -j 4 --output-on-failure
cmake --build build/horizons-coverage -j 3
ctest --test-dir build/horizons-coverage -L vmm -j 3 --timeout 1800 --output-on-failure
cmake --build build/horizons-coverage --target vmm_coverage
ctest --test-dir build/horizons-debug -R Cvt --output-on-failure
ctest --test-dir build/horizons-clang -R Cvt --output-on-failure
python3 vmm/tools/ci/check_requirements.py --root vmm
vmm/docs/build_docs.sh build build/horizons-docs-venv
python3 planning/cvt/benchmark.py --build build --repeats 3
```

Nesta execução, a documentação usou `build/build_cvt_docs.py`, que reproduz as
etapas Doxygen e Sphinx após os testes já aprovados, sem repetir a suíte.
Saída: `build/cvt_docs/html/index.html` e `build/cvt_docs/html/en/index.html`.
O consumidor em `build/cvt-consumer` usa apenas o pacote instalado em
`build/cvt-install`, chama CVT e verifica convergência e energia analítica.

## Desempenho

Script: [benchmark.py](benchmark.py). Dados brutos:
[benchmark_results.csv](benchmark_results.csv). São nove casos, três processos
independentes por caso, semente 7 e cinco iterações. Medição de ponta a ponta:
partição, geração, otimização, validação e escrita de vmesh. GNU time fornece
tempo com resolução de centésimos de segundo e RSS máximo em KiB. Execução
após as suítes e builds; sem outros jobs VMM concorrentes.

| Caso | Células finais | Tempo mediano (s) | RSS máximo entre repetições (KiB) |
|---|---:|---:|---:|
| 2D | 16 | <0,01 | 8212 |
| 2D | 64 | 0,01 | 8336 |
| 2D | 256 | 0,02 | 8620 |
| 3D | 8 | 0,05 | 9264 |
| 3D | 27 | 0,18 | 9404 |
| 3D | 64 | 0,42 | 9588 |
| Horizontes, 4 sítios na base | 16 | 0,16 | 10360 |
| Horizontes, 9 sítios na base | 36 | 0,41 | 10672 |
| Horizontes, 16 sítios na base | 64 | 0,67 | 11036 |

Todos os casos aceitaram cinco passos e reduziram a energia; nenhum convergiu
nesse orçamento e nenhum estagnou. As energias impressas coincidiram entre
repetições. Esses casos pequenos caracterizam o custo local e não demonstram
escalabilidade para milhões de células nem tempo até convergência.

## Limitações mantidas

- Densidade uniforme; sem pesos de potência, inserção/remoção de sítios ou física.
- Mínimo global, ortogonalidade em interfaces e eliminação de células finas
  não são garantidos. Sucesso da função não implica convergência.
- A geometria curva permanece facetada; precisão de saída é `double`.
- Cada tentativa reconstrói e valida a malha. Não há atualização incremental
  das estruturas geométricas nem promessa de desempenho para grandes malhas.
- CVT volumétrico perde a organização em colunas. Subdivisões internas da mesma
  região não são interfaces obrigatórias. Arquivos vlayers de execuções antigas
  não são removidos automaticamente; usar nomes de saída distintos.
- Partições e sítios devem ser aceitos pelo gerador. Não há suporte irrestrito
  a malhas poliedrais externas sem referência regional válida.

Nenhuma pendência obrigatória de C01–C05 permanece. Publicação e versionamento
no Git ficam a cargo do usuário.
