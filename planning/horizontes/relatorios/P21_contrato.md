# P21 — Contrato geométrico (minuta em execução)

- Data: 2026-10-03.
- Estado: CONCLUÍDO como contrato de escopo — respostas incorporadas; implementação e validação geométrica ainda não executadas.
- Ambiente: WSL Ubuntu-26.04-Test, /home/jflavio/Programas/VMM.
- HEAD consultado: da5c03eab5865e14fe7ab7ed1ff29d529cd80d79.
- Alterações documentais anteriores preservadas; nenhum código de produção alterado.
- Compilação, testes, cobertura e benchmark: não executados nesta etapa documental.
- Autorização: “pode começar” inicia a sequência; não responde implicitamente Q01–Q10.

## 1. Contrato confirmado

O VMM gera geometria e conectividade, sem depender do mohid-ng. Propriedades físicas são
associadas pelo consumidor. Usar região para a parte identificada do domínio global.

Todas as faixas iniciais compartilham uma base Voronoi 2D. Uma região pode conter várias
camadas de discretização. Uma camada não é sinônimo de região nem de material.

A fronteira regional é fixa, tanto lateral quanto vertical. Em 3D, o plano de um trecho
plano é fixo; em 2D, a reta de um trecho é fixa. Faces/arestas podem mudar dentro desses
suportes. Seus limites, extensão e incidência regional permanecem preservados.
O mesmo requisito vale para CVT e retesselação futuros. Não congelar IDs ou quantidade
de faces como substituto de preservar a geometria.

Colunas são uma organização inicial, não uma obrigação para toda transformação futura.
A malha entregue é imutável; uma transformação produz outro objeto e metadados coerentes.

## 2. Modelo geométrico para avaliação

Proposta aceita pelo João nesta execução (detalhes numéricos e de API serão definidos em P22/P23):
- coordenadas cartesianas em unidade de comprimento consistente, z positivo para cima;
- horizontes como gráficos z(x,y), sem dobras verticais;
- superfícies recebidas por funções ou cotas e convertidas para representação facetada;
- referência discretizada aceita separada da discretização variável das células;
- frações verticais ordenadas por intervalo, de 0 a 1, para subdivisão uniforme/graduada;
- mapa explícito (região horizontal, intervalo) → região 3D, admitindo que vários
  intervalos de discretização pertençam à mesma região;
- proveniência por célula e por face, sem embutir propriedades físicas;
- nenhuma troca silenciosa de domínio, colapso ou descarte de uma região fina.

No caso de fronteira curva, interpolar novamente apenas a partir de vértices móveis pode
alterar seu suporte. A representação fixa e sua aproximação inicial precisam ser acordadas.
Encontros e desaparecimento parcial estão incluídos. Nas regiões ausentes não criar células de volume zero. A quantidade nominal de subdivisões é a mesma por intervalo; isso não obriga a materializar células onde o intervalo não existe.

## 3. Matriz de aceitação

Os contratos de API abaixo são responsabilidades propostas, não nomes públicos aprovados.

| Requisitos | Responsabilidade de API | Verificação planejada | Etapa |
|---|---|---|---|
| H01/H02 | entrada/saída puramente geométrica | grafo sem solver e sem campos físicos | P23/P26 |
| H03/H06 | partição regional de referência | pertença, cobertura e suporte, não só área | P22/P27 |
| H04/H05 | gerador de colunas e proveniência | mesma base em todas as faixas iniciais | P24 |
| H06/H09 | restrições de suporte 2D/3D | reta/plano oblíquos, limites e junções fixos | P22/P27 |
| H07 | metadados explícitos | renumeração preserva; transformação futura não deixa IDs obsoletos | P23/P26/T01 |
| H08 | contrato e decisões | nenhuma recomendação tratada como resposta | P21 |
| H10 | guia/API/galeria Sphinx | atualização pt/en, exemplos depois de GTest e integração | P28 |
| Q03 | topologia de encontros | suportar fechamento ou rejeitar explicitamente | P22/P25 |
| Q04 | subdivisão vertical | espessuras/volumes analíticos e frações válidas | P24/P25 |
| Q05/Q09 | representação fixa de horizonte | suporte invariante sob mudança das faces | P22/P25 |
| Q06 | IO e acesso | round-trip de geometria e identificações | P26 |
| Q07/Q08 | combinação regional e base geral | lateral × vertical, buracos e fragmentos conforme escopo | P22/P26 |
| Q10 | escala de aplicação | tempo/memória por colunas × camadas | P27 |

## 4. Casos geométricos candidatos

Não são exemplos adicionados ao manual nem resultados executados.

- C1: base retangular, três horizontes planos e duas regiões, com mais de uma camada
  numa região; volumes por área × espessura.
- C2: horizontes planos oblíquos paralelos e terreno facetado; verificar suporte fixo
  após mudar a distribuição das faces, não igualdade dos vértices.
- C3: duas regiões laterais cruzadas por horizontes; encontro das interfaces vertical
  e horizontal com conformidade nos quatro setores.
- C4: análogo 2D com interface oblíqua; subdivisão das arestas variável na mesma reta
  e extensão, com extremos fixos.
- C5: região que termina onde horizontes se encontram, se aprovada; caso contrário
  rejeição explícita, sem células de espessura zero.
- C6: base não convexa, buraco e fragmentos, conforme contrato a fechar.
- C7: dados inválidos, NaN/Inf, inversão de horizontes, IDs inválidos e overflow.
- C8: três tamanhos sintéticos, variando separadamente colunas e camadas; metas
  quantitativas após protótipo, sem inventar capacidade ainda não medida.

Em C2/C4, acrescentar testes negativos com deslocamento da interface que mantém sua medida.
O conjunto de verificações deve detectá-lo. CVT real requer algoritmo futuro implementado;
reconstruções com sítios diferentes não serão apresentadas como execução de CVT.

## 5. Verificação da implementação atual

Leitura de código nesta árvore:
- vmm/src/voronoi/builder2d.cpp: geração por região e emit_interface_faces.
- vmm/src/voronoi/builder3d.cpp: refinamento comum das peças de interface.
- vmm/src/facade/facade.cpp: partição, sítios, construção e check_invariants.
- Busca textual por cvt/lloyd em vmm/include, vmm/src e vmm/tests não retornou ocorrências.
  Isso sustenta não anunciar um otimizador público existente; não é prova de execução.

O suporte a colunas não deve reutilizar automaticamente promessas de Voronoi euclidiano 3D.
Manter a modalidade atual e seu contrato intactos.

## 6. Perguntas enviadas nesta execução

1. Encontros de horizontes e desaparecimento parcial de domínio: suportar já ou exigir
   espessura positiva? (Q03)
2. Planos e gráficos z(x,y) recebidos por funções/cotas, com referência facetada fixa,
   ou apenas planos? (Q01/Q02/Q05/Q09)
3. Proposta de API e CLI, persistência, subdivisões, mapa regional e bases gerais; caso
   geométrico/tamanho desejado? (Q04/Q06/Q07/Q08/Q10)

Respostas recebidas em 2026-10-03:
1. “Sim, suportar encontros e desaparecimento”.
2. “Sim, usar essa representação”.
3. “Aceito a proposta; use casos sintéticos”.

Q01–Q10 estão resolvidas no nível de escopo: planos e gráficos z(x,y); funções/cotas;
referência facetada fixa; encontros e desaparecimento; subdivisões uniformes/graduadas
com mesma quantidade nominal por intervalo; API C++ e vmm-mesh; identificadores persistidos;
combinação região horizontal × intervalo; bases não convexas, buracos e componentes
separados; casos sintéticos para validação. Não foram fixadas metas de tempo/memória
pelo usuário: serão propostas com evidência no protótipo.

## 7. Consequências obrigatórias para P22/P23

- O desaparecimento pode ocorrer dentro da projeção de uma célula horizontal.
  Avaliar refinamento comum da base nas linhas de término, compartilhado entre faixas,
  mantendo proveniência da base original. Não reduzir suporte a encontros em vértices
  previamente existentes.
- Igualdade de horizontes em parte da área não significa inversão permitida.
  Definir detecção robusta no interior das facetas; testar cruzamentos indevidos.
- A resolução horizontal comum deve coexistir com regiões verticalmente ausentes.
- Bases com buracos e partes desconectadas exigem triangulação válida; leque ingênuo
  pelo sítio não satisfaz esse contrato.
- Planos/retas e extensão das interfaces são restrições; as faces geradas são variáveis.
- A discretização inicial da superfície deve ficar identificada e reutilizável,
  sem reamostrar sua geometria durante mudanças da tesselação.
- CLI recebe dados serializáveis; funções C++ são entrada da API, não executáveis
  arbitrários embutidos em arquivos de configuração.
- O contrato não amplia T01 para implementação de CVT: define sua obrigação geométrica.

## 8. Resultado

Escopo fechado com as respostas do João. P21 entrega contrato, matriz e casos candidatos.
Próxima etapa: P22, prova geométrica isolada antes de fixar a API.
Não há evidência de geração nova, testes ou cobertura nesta etapa documental.
