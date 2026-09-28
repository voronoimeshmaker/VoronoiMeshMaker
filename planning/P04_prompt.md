```
REGRAS COMUNS
- Repositório: /home/jflavio/Programas/VMM, na distribuição WSL Ubuntu-26.04-Test (siga o AGENTS.md).
  Remoto: https://github.com/voronoimeshmaker/VoronoiMeshMaker
- Declare no início do entregável o ambiente em que rodou e as versões usadas (compilador, CMake, CGAL,
  Boost, GoogleTest e as demais que o prompt usar). Só versões estáveis (DEC-012). Padrão C++23 (DEC-014).
- Não faça commit nem push. Preserve as alterações não commitadas.
- Leia antes de começar: AGENTS.md, planning/00_requisitos_iniciais.md, planning/DECISIONS.md e
  todos os planning/P*.md já concluídos. As decisões APROVADAS são obrigatórias; não as reabra.
- Novas decisões: acrescente-as ao planning/DECISIONS.md com status PROPOSTA (numeração
  sequencial). Nunca altere o status de uma entrada existente.
- Nada de afirmar que um teste passou, que a cobertura foi atingida ou que uma dependência funciona
  sem evidência do ambiente ativo. Afirmações sobre software de terceiros (licenças, APIs,
  limitações) precisam de fonte citada (documentação oficial ou código-fonte), com link.
- Distinga sempre: FATO VERIFICADO / INFERÊNCIA / RECOMENDAÇÃO.
- Entregáveis em português; código, identificadores, comentários técnicos e Doxygen em inglês.
- Ao terminar: (1) resumo de até 15 linhas; (2) lista numerada de DECISÕES PENDENTES para o João,
  cada uma com opções e a sua recomendação; (3) PARE. Não inicie a etapa seguinte.

P04 — PROBLEMA-ÂNCORA, CONSUMIDOR DA MALHA E METAS

CONTEXTO
A fase 1 (P01–P03) está concluída e registrada em planning/. Pontos que este prompt deve respeitar:
- DEC-001: o produto é domínio + sítios → modelo de malha de volumes finitos; formatos de solver
  entram como escritores (plugins), não como núcleo.
- DEC-011: a verificação principal é por invariantes geométricos; a VMMLib é oráculo só no que ela
  cobre (2D, domínio convexo de um anel).
- DEC-013: a partir do P07, um benchmark leve (2 ou 3 casos fixos) acompanha cada entrega.
- DEC-015: o VMM não depende do PETSc nem de nenhum solver; entrega topologia e adjacência próprias.
- 00_requisitos §7: a definição de regiões por precedência (CSG por diferença) é decidida no P04/P06.
Este prompt é documental: não altera código, CMake nem testes. Declare o ambiente mesmo assim.

OBJETIVO
Tirar o projeto do abstrato: definir os casos reais que guiam as prioridades, os formatos de saída
da primeira versão e metas mensuráveis.

TAREFAS
1. Problemas-âncora. Propor 2 ou 3 problemas de escoamento ambiental com regiões fixas (água,
   sólido, gás), em ordem crescente de dificuldade. Os dois primeiros são 2D (entrega (a)); o
   terceiro pode ser 3D e servir de alvo para a Fase 3. Exemplo inicial: trecho de rio 2D com margem
   de solo e leito, 2 a 3 regiões. Para cada problema:
   a) geometria (dimensões, escala física, forma das fronteiras, presença de não convexidade,
      buracos, arestas vivas e junções triplas);
   b) meios, regiões e interfaces (R5, R6), incluindo ao menos um caso com duas regiões do mesmo
      meio;
   c) patches de contorno e os seus rótulos;
   d) como as regiões seriam declaradas: avaliar a proposta de precedência (CSG por diferença, na
      ordem declarada) contra alternativas, com o validador de sobreposições e vazios;
   e) ordem de grandeza do número de células e a variação de densidade de sítios;
   f) dados de entrada (polígonos, arquivos, parâmetros) e onde obtê-los ou como gerá-los;
   g) invariantes que o caso verifica (DEC-011) e se o caso pode ter referência na VMMLib.
2. Consumidores da malha. Levantar, com fonte, os solvers de volumes finitos usados pelo
   público-alvo e os formatos que eles leem. No mínimo: OpenFOAM (polyMesh), VTK (legado e XML,
   já presentes na VMMLib), CGNS, Gmsh (.msh), MODFLOW 6 (DISV/DISU), PFLOTRAN (unstructured
   explicit/.uge), TOUGH (MESH). Para cada formato: o que ele representa (células poliédricas,
   faces, owner/neighbour, regiões, patches, coeficientes geométricos), se exige biblioteca externa
   (DEC-004), licença dessa biblioteca e esforço relativo de um escritor. Recomendar os formatos da
   primeira versão, com justificativa.
3. Metas mensuráveis, para 2D e para 3D, cada uma com o método de medição:
   a) tempo de geração em função do número de células;
   b) memória por célula;
   c) limites de qualidade (não ortogonalidade, razão de aspecto, faces e células mínimas);
   d) determinismo (o que exatamente deve ser reproduzível, e em que condições);
   e) tolerâncias relativas à escala do domínio (DEC-011).
4. Casos do benchmark leve (DEC-013): escolher 2 ou 3 casos fixos, derivados dos problemas-âncora,
   com tamanho definido e o que se mede.
5. QUESTIONÁRIO curto (no máximo 10 perguntas, cada uma com opções e a sua recomendação) para o
   João validar os casos, os formatos e as metas.

ENTREGÁVEL
planning/P04_ancora_metas.md, com esta ordem de seções:
1. ambiente;
2. problemas-âncora (uma subseção por problema, com os itens a–g);
3. declaração de regiões (comparação e recomendação);
4. consumidores e formatos (tabela com fonte para cada linha) e recomendação;
5. metas mensuráveis;
6. casos do benchmark leve;
7. riscos;
8. entradas propostas para planning/DECISIONS.md (status PROPOSTA);
9. questionário;
10. resumo e decisões pendentes.

CRITÉRIO DE CONCLUSÃO
Casos descritos o suficiente para virar testes de integração e casos de benchmark; formatos com
fonte e recomendação; metas com método de medição; questionário pronto. Nenhum arquivo além do
entregável e do planning/DECISIONS.md foi alterado.
```
