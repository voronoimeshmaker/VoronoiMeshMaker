# P22–P26 — implementação e integração

Registro em 2026-10-04, árvore ativa Ubuntu-26.04-Test.
Execução autônoma autorizada pelo João; sem commit/push.

## P22: prova e construção geométrica

O protótipo racional em prototypes/P22a/proof.py verifica integrais afins,
subdivisões e desaparecimento. Não prova sozinho a conformidade da malha.
A implementação usa triangulação restrita com construções exatas no backend
CGAL: interseção comum da malha base com a grade de referência, incluindo
diagonais SW–NE. Triângulos recebem a coluna por número de enrolamento.
Faces interiores de uma mesma coluna/camada são canceladas; as demais são
compartilhadas por IDs globais.

O teste de interface lateral oblíqua revelou interseções exatas distintas
arredondando ao mesmo par de doubles. A saída agora aplica uma identificação
global desses vértices antes de montar as faces; triângulos com IDs repetidos
não formam faces. Se uma coluna presente na representação exata desaparece
inteiramente, a geração falha explicitamente. Esse tratamento não equivale
a garantir precisão arbitrária: persistem limites de representabilidade.

## P23: arquitetura implementada

- HorizonGrid: cotas nodais ou funções amostradas uma única vez; z(x,y)
  afim por triângulo; igualdade permitida, inversão rejeitada.
- Backend column_overlay: CGAL permanece no backend; entrada/saída sem
  tipos externos, conforme firewall.
- generate_layered_mesh: composição com Mesh2D existente; não substitui
  a geração Voronoi euclidiana 3D.
- LayeredMesh/LayeredData: Mesh3D imutável, grade, frações, coluna/camada
  por célula e nível por face. Não há propriedades físicas.
- Identificação regional: região da base × intervalo entre horizontes.
  Subdivisões dentro do mesmo intervalo compartilham a região.
- Referências das células são centroides volumétricos; não são geradores
  de um Voronoi euclidiano 3D.

## P24/P25: geração

Horizontes planos, inclinados, facetados e encontros parciais implementados.
Frações estritamente crescentes de 0 a 1 representam subdivisão uniforme ou
graduada. O número nominal é comum por intervalo. Volume zero não materializa
célula. Base com concavidades, buracos e ilha separada tem teste de integração.

Volumes de referência são integrados pelas espessuras afins, independentemente
das métricas calculadas pelas faces. A fábrica valida metadados e amostras de
suporte/contenção. Não anunciar essa amostragem como prova geral de qualquer
malha externa modificada ou de futuras transformações CVT.

## P26: integração

- Consultas cells_in_column e vertical_neighbours.
- Renumeração preserva proveniência; teste RCM e inversa.
- Formato VMM_LAYERS 1 incorpora grade e metadados mais vmesh.
- Formato vmesh existente preservado. VTU contém geometria volumétrica.
- Configuração dimension=2 com horizons=arquivo.hgrid compõe a extrusão;
  relatório final possui dimensão 3 e arquivo .vlayers acompanha as saídas.
- Testes cobrem persistência, truncamentos, streams inválidos, configuração,
  mudança de sítios e suporte finito 2D e 3D. Não executam CVT.

## Validação e pendências

Suíte Release anterior à última ampliação de negativos: 305/305, 37.70 s,
build/horizons_full_tests.log. Recompilar para incluir testes posteriores.
Logs dirigidos de cobertura: build/horizons_coverage_tests.log.
Matriz completa de sanitizadores ainda em andamento; Clang em compilação.
P27 permanece aberto, assim como documentação P28 e entrega P29.

Não considerar todos os requisitos de aceitação satisfeitos por este relatório.
Consolidar cobertura, negativos topológicos, desempenho e limites da representação.
