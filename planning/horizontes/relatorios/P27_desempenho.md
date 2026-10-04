# P27 — desempenho sintético incremental

Ambiente Ubuntu-26.04-Test; Release da árvore ativa em 2026-10-04.
Comando: `python3 planning/horizontes/prototypes/P27/benchmark.py`.
O tempo inclui processo CLI, geração da base, extrusão e gravação vmesh/vlayers.
Uma amostra por caso; havia compilação e testes em outros diretórios de build.
Resultados exploratórios, não capacidade máxima nem benchmark isolado.

| Colunas | Camadas | Células | Faces | Pontos | s | Pico KiB | µs/célula |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 225 | 2 | 450 | 2310 | 768 | 0.0442 | 10044 | 98.32 |
| 225 | 8 | 1800 | 7890 | 2304 | 0.0607 | 11712 | 33.73 |
| 961 | 2 | 1922 | 9734 | 3072 | 0.5285 | 12688 | 274.98 |
| 961 | 8 | 7688 | 33170 | 9216 | 0.6213 | 22740 | 80.82 |
| 3969 | 2 | 7938 | 39942 | 12288 | 7.6726 | 25648 | 966.57 |
| 3969 | 8 | 31752 | 135954 | 36864 | 7.7878 | 65720 | 245.27 |

A classificação percorre todas as arestas para cada triângulo do refinamento.
A busca dos patches também percorre arestas de contorno. O crescimento observado
é aproximadamente quadrático com colunas nesses casos. Aumentar camadas reutiliza
o refinamento, portanto tem impacto menor. Antes de anunciar suporte a milhões de
células, implementar localização espacial ou propagação topológica e medir novamente.
Nenhuma meta de desempenho foi imposta pelo usuário; não se declara capacidade
máxima ou prazo de execução garantido.
