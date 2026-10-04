# Validação incremental — 2026-10-04

Estado parcial; não encerra P27 nem P21–P29.

## Árvore e ambiente

WSL Ubuntu-26.04-Test, /home/jflavio/Programas/VMM. Alterações sobre
8191e262301a0692f9485259a7d313f789d05626, sem commit/push.

## Implementação e regressões

- HorizonGrid::elevation avalia a referência triangular fixa, inclusive limites e diagonal.
- LayeredMesh::from_data verifica relação região/intervalo, incidência de colunas/camadas,
  posições nos horizontes e contenção vertical por amostras de vértices, arestas e centro.
  Essa amostragem não é uma prova geral para faces arbitrárias atravessando múltiplas
  facetas; as faces produzidas pelo gerador respeitam a triangulação de referência.
- Integração cobre base não convexa com buraco e ilha, horizonte oblíquo com diferentes
  sítios, persistência, renumeração e composição CLI.
- Regressão negativa desloca a geometria sem mudar volumes; o contrato rejeita o deslocamento.
- Caso com escala 0.001 e translação 100 revelou cancelamento no centro das faces.
  Corrigido face_geometry 3D para calcular o momento relativo ao primeiro vértice.
  Teste unitário específico e integração com seis combinações de escala/translação passam.
- Geração repetida produz serialização idêntica nos casos de escala/translação.

## Evidência

Comandos executados somente na árvore ativa:

```sh
cmake --build build -j3
ctest --test-dir build -R 'HorizonGrid|Layered' --output-on-failure
ctest --test-dir build -R FaceCentroidSmallTranslatedTriangle --output-on-failure
ctest --test-dir build --output-on-failure -j3
python3 vmm/tools/ci/check_requirements.py --root vmm
```

14 testes da extensão aprovados; regressão unitária de centro aprovada.
Suíte completa final: 304/304 aprovados em 31.50 s.
Log: build/horizons_full_tests.log. Checker: zero problemas.

Uma execução intermediária teve quatro BAD_COMMAND porque o executável Mesh foi
relinkado enquanto CTest o utilizava. A repetição final ocorreu após concluir a
compilação e passou integralmente. Não repetir compilação e testes na mesma árvore
simultaneamente.

## Próximos passos

Compilação Debug GCC14 com VMM_SANITIZE=ON em build/horizons-debug;
consultar build/horizons_debug_build.log e build/horizons_debug_tests.log antes de
iniciar outra execução. A execução dirigida terminou: 20/20 testes aprovados em 2.12 s (HorizonGrid, Layered e Mesh), com ASan/UBSan habilitados.
Não há ainda execução completa Debug/sanitizadores, cobertura, Clang ou
desempenho. Acrescentar teste de suporte finito 2D, demais negativos da matriz P27,
relatórios das etapas, Sphinx pt/en e relatório final de riscos.
Automação deve continuar ativa.
