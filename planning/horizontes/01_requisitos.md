# Requisitos — domínios, colunas e horizontes

Data: 2026-10-03. Este registro distingue instruções do João de recomendações técnicas.

## Confirmado pelo João

| ID | Requisito | Evidência na conversa | Aceitação |
|---|---|---|---|
| H01 | O VMM gera malhas; o mohid-ng é consumidor | “o mohid-ng usará o vmm, e nao o contrário” | nenhuma dependência ou regra física do solver |
| H02 | Propriedades são associadas no mohid-ng | “associado a malha, incluiremos a propriedade” | saída contém geometria e identificadores, não propriedades físicas |
| H03 | Separar os domínios geometricamente | “é importante separar um dominio do outro” | célula pertence a uma região e não atravessa sua interface |
| H04 | Geração inicial em colunas e horizontes | “Colunas e horizontes” | mapa explícito entre base 2D, coluna e células 3D |
| H05 | Mesma malha horizontal nas faixas iniciais | resposta “1 - sim” | base compartilhada na geração inicial |
| H06 | Fronteiras entre domínios não podem mudar | “o que nao pode mudar é a fronteira entre dois dominios” | suporte geométrico e incidência regional preservados |
| H07 | Uma transformação futura pode desfazer colunas | observação sobre tesselação posterior | estrutura em colunas não é restrição de toda malha VMM |
| H09 | CVT deve manter fixo o suporte da interface em 2D e 3D | instruções posteriores: plano fixo em 3D, mesmo princípio em 2D | faces/arestas podem mudar dentro do suporte, nunca mover a interface |
| H10 | Atualizar documentação Sphinx | pedido explícito do João | contrato publicado no fonte do manual; recurso futuro identificado como planejado |
| H08 | Planejar antes de implementar | pedido dos questionamentos e da sequência de MD | pendências resolvidas antes de código dependente delas |

## Vocabulário proposto, sem mudança da API existente

- Domínio global: conjunto geométrico total ocupado pela malha.
- Região: parte identificada do domínio global; corresponde ao domínio mencionado na conversa.
- Interface: fronteira compartilhada entre duas regiões identificadas.
- Horizonte: superfície usada para delimitar regiões ou subdividir a discretização.
- Camada de discretização: intervalo vertical de células; não implica região nova.
- Coluna: conjunto inicial de células derivadas de uma célula da base 2D.
- Patch: identificação geométrica de contorno, sem impor condição física.
- Meio existente na API: manter compatibilidade; rótulo não é propriedade física.

Um horizonte que separa regiões é restrição geométrica obrigatória. Uma subdivisão interna
de uma região não ganha automaticamente essa obrigação. Formalizar essa distinção em P21.

## Questões do contrato — respondidas em P21 (2026-10-03)

| ID | Pergunta | Recomendação para avaliação | Bloqueia |
|---|---|---|---|
| Q01 | Horizontes planos e irregulares z(x,y)? Há superfícies com mais de uma cota para o mesmo x,y? | planos e gráficos z(x,y); superfícies dobradas exigem outro modelo | P22/P23 |
| Q02 | Entrada por função, cotas nos vértices, grade ou arquivo triangulado? Altitudes ou profundidades relativas? | funções e cotas; convenção z positiva para cima explícita; conversões geométricas documentadas | P22/P23/P26 |
| Q03 | Dois horizontes podem se encontrar e uma região desaparecer em parte da área? | decidir explicitamente; não colapsar ou remover regiões silenciosamente | P22/P25 |
| Q04 | Quantas subdivisões por intervalo? Uniformes, graduadas, variáveis entre colunas? | subdivisões uniformes/graduadas; mesmo número por intervalo inicialmente | P23/P24 |
| Q05 | Faces trianguladas nos horizontes são aceitáveis? | permitir para obter faces planas em superfícies irregulares | P22/P23 |
| Q06 | API C++, vmm-mesh ou ambos? Quais identificações precisam persistir? | API e persistência explícita dos metadados; CLI conforme uso desejado | P23/P26 |
| Q07 | Toda região horizontal é atravessada por todos os horizontes? Como combinar fronteiras laterais e verticais? | explicitar mapa de região horizontal × intervalo para região 3D | P21/P23 |
| Q08 | Base com células não convexas, buracos ou componentes desconectados precisa ser suportada já? | avaliar casos reais e custo no protótipo; rejeição explícita de casos não suportados | P22/P23 |
| Q09 | Qual referência define a fronteira imutável: função original ou superfície discretizada aceita? | congelar representação discreta acordada; separar erro de aproximação inicial de alteração posterior | P21/P22 |
| Q10 | Quais dimensões, escalas e tamanhos de malha representarão a aplicação? | dois casos puramente geométricos: profundidades e horizontes acompanhando terreno | P22/P27 |

A tabela acima preserva o questionário original; suas respostas estão registradas abaixo e no relatório P21.

As sete perguntas da conversa estão preservadas: Q01–Q06 detalham as questões de horizontes,
entrada, desaparecimento, subdivisões, faces e entrega; H05 registra a única resposta numérica
já recebida. Q07–Q10 refinam condições necessárias para uma implementação verificável.

## Critérios geométricos transversais

- Região e camada são identificações distintas. Regiões diferentes continuam separadas mesmo
  se um consumidor lhes atribuir a mesma propriedade.
- Faces de interface têm dois lados consistentes; nenhum vazio, sobreposição ou face solta.
- Novos vértices numa interface permanecem em seu suporte geométrico. Partir uma face não
  autoriza mover a fronteira.
- Preservar curvas/arestas de junção e interfaces triplas quando presentes no escopo aprovado.
- Medidas positivas, fechamento vetorial e conectividade recíproca; medidas por região.
- Usar tolerâncias relativas e aritmética robusta conforme as decisões vigentes.
- Não reduzir a verificação à área total: uma fronteira deslocada pode conservar área.
- Sem promessa de ortogonalidade global das colunas com horizontes inclinados.
- A malha gerada permanece imutável. Regeneração futura produz novo objeto; não implica física temporal.

## Esclarecimento confirmado — suporte fixo, subdivisão variável

Em 3D, o plano de cada interface plana permanece fixo. As faces dos volumes apoiadas nesse
plano podem mudar de tamanho, forma, quantidade e conectividade, sem sair dele.
Em 2D, a reta de cada trecho de interface permanece fixa; as arestas podem mudar seus
extremos e sua subdivisão dentro do trecho permitido.

Preservar o suporte não é congelar a tesselação da interface. Além de coplanaridade ou
colinearidade, preservar a extensão da fronteira, seus limites e o par de regiões:
não basta permanecer em algum ponto do mesmo plano/reta infinito.
Nas junções, os pontos devem respeitar simultaneamente os suportes incidentes.
Para interfaces não planas, o contrato de Q09 deve especificar os planos dos trechos
discretizados ou outra superfície de referência; não substituir toda a interface curva
por um plano único.

No CVT, deslocar sítios e reconstruir células não autoriza deslocar essa fronteira.
Este é requisito obrigatório de qualquer CVT futuro; a existência de uma implementação
CVT pública atual deve ser verificada antes de propor sua alteração.


## Respostas confirmadas ao iniciar P21

- Q01/Q02/Q05/Q09: aceita a representação por planos e gráficos z(x,y), funções ou cotas,
  convertidos para superfície facetada fixa; subdivisão das faces pode mudar dentro dela.
- Q03: suportar encontros e desaparecimento parcial de domínio.
- Q04: subdivisões uniformes/graduadas, mesma quantidade nominal por intervalo.
- Q06: API C++ e vmm-mesh, com identificadores persistidos.
- Q07: combinação explícita região horizontal × intervalo.
- Q08: incluir bases não convexas, buracos e componentes separados.
- Q10: usar casos sintéticos; metas quantitativas a propor após medir o protótipo.

Origem: respostas explícitas do João às três perguntas agrupadas de P21.
Ver [contrato consolidado](relatorios/P21_contrato.md).
As recomendações aceitas acima deixam de ser pendências; detalhes de algoritmo e API
continuam pertencendo a P22/P23, sem necessidade de repetir esse questionário.
