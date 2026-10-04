# P29 — entrega e relatório final

Data: 2026-10-04. Versão 1.1.0 preparada na árvore de trabalho, não publicada.
Ambiente: WSL Ubuntu-26.04-Test, /home/jflavio/Programas/VMM.
Sem commit, tag ou push; alterações anteriores preservadas.

## Resultado

P21–P29 implementados e validados dentro do escopo geométrico acordado.
O VMM gera a malha; mohid-ng associa propriedades. Não foi implementado CVT,
retesselação geral, física, transporte ou movimentação temporal de horizontes.

A entrada é uma base 2D e uma grade independente de horizontes z(x,y), com
triangulação fixa. Colunas compartilham a mesma base; intervalos podem desaparecer
parcialmente. Subdivisões uniformes/graduadas permanecem na mesma região do
intervalo. Saída oferece consultas, proveniência, renumeração e persistência.
C++ e vmm-mesh usam a mesma construção; exemplo plano gera .vlayers idêntico.

## Rastreabilidade

| Requisito | Resultado e evidência |
|---|---|
| H01/H02 | Geometria e identificadores; nenhuma dependência do mohid-ng ou propriedade física nova |
| H03 | Regiões = região horizontal × intervalo; testes de contenção, rótulos, fechamento e integração |
| H04/H05 | Coluna/camada explícitas, refinamento comum da base; testes de consultas e desaparecimento |
| H06 | Referência fixa, interfaces oblíquas, extremos e cobertura 2D, planos 3D e reconstrução com sítios diferentes |
| H07 | Mesh3D separado de LayeredMesh; transformações futuras devem atualizar ou remover proveniência inválida |
| H08 | Questionário e sequência anteriores preservados; autorização posterior permitiu execução sem aprovações intermediárias |
| H09 | Contrato de CVT documentado, sem alegar implementação ou execução de CVT |
| H10 | Sphinx pt/en, Doxygen, galeria, exemplos C++/CLI, README e changelog |

Q01–Q10: gráficos z(x,y), funções amostradas/cotas, igualdade e desaparecimento,
frações por intervalo, faces planas, API/CLI, combinação regional, bases gerais,
referência congelada e casos sintéticos entregues. Coordenada z cresce de baixo
para cima; profundidades positivas para baixo exigem conversão pelo consumidor.

## Validação

Ver P27_validacao.md e P28_documentacao.md para comandos e limites:
- 310 testes Release aprovados no pipeline documental.
- Suíte Debug ASan/UBSan inicial 304/304 e 25/25 dirigidos após as correções.
- Clang19: 25/25 dirigidos após recompilação.
- Cobertura: suíte instrumentada mais testes dirigidos atualizados; 97.4% linhas,
  90.3% ramos, sem novos desvios R25.
- Instalação em build/horizons-install e consumidor externo com
  find_package(VoronoiMeshMaker 1.0), compilação e execução aprovadas.
- vmesh versão 1 preservada; vlayers possui cabeçalho/versionamento separado.
- CMake, header e Sphinx preparados como 1.1.0; compatibilidade binária não é
  garantida pelo projeto, requer recompilação dos consumidores.

## Problemas, limitações e riscos

1. **Escalabilidade:** classificação por enrolamento percorre arestas para cada
   triângulo. O custo observado cresce aproximadamente de forma quadrática com
   colunas. Antes de malhas muito grandes, priorizar índice espacial ou propagação
   topológica; não extrapolar as medições até 31752 células para milhões.
2. **Precisão de saída:** backend exato termina em double. Pontos que arredondam
   à mesma posição são identificados globalmente. Perda completa de coluna é
   rejeitada; feições próximas do limite de representabilidade podem falhar.
   Não prometer preservação geométrica com precisão arbitrária.
3. **Superfícies:** são gráficos z(x,y) em grade retangular triangulada fixa.
   Dobras, sobreposições verticais e superfícies multivaloradas ficam fora.
   A aproximação inicial depende da resolução; ela não melhora ao mudar sítios.
4. **Validação externa:** from_data verifica amostras do suporte, metadados,
   duplicações, volumes positivos e fechamento/orientação. Não é prova completa
   para qualquer poliedro arbitrariamente editado, especialmente faces cruzando
   múltiplas facetas de referência. O gerador é validado adicionalmente por
   construção e volumes independentes.
5. **Qualidade:** horizontes inclinados e pinch-outs podem gerar células muito
   finas/não ortogonais. Identidade regional não garante qualidade para qualquer
   esquema numérico; solver deve avaliar métricas antes de usar.
6. **Semântica:** índices são proveniência da geração, preservados pela renumeração.
   Não são identificação persistente de entidades entre retesselações arbitrárias.
   Faces coincidentes de pinch-out guardam o menor nível incidente.
7. **Documentação:** 16 avisos Sphinx por idioma na interpretação de extern template
   da referência API. Páginas e galeria foram geradas; avisos estão registrados,
   sem supressão. Não representam testes C++ falhando.
8. **Benchmark:** uma amostra por caso, inclui IO e concorrência com outros processos.
   Usar os números como exploração, não compromisso de desempenho.
9. **CVT futuro:** deverá preservar plano/reta, extensão, junções e regiões a cada
   iteração. Os testes atuais demonstram reconstrução com referências fixas,
   não um otimizador CVT.

## Revisão pelo João

Diferenças ficam disponíveis por git diff e arquivos novos por git status.
Principais arquivos: horizons.hpp/cpp, layered.hpp/cpp nos módulos facade, mesh,
io e reorder, backend column_overlay, testes geometry/mesh/integration,
exemplos horizons/config e guia horizons.rst. Relatórios históricos mostram a
sequência real, inclusive falhas encontradas e corrigidas.
Commit, tag e publicação continuam exclusivamente com o João.
