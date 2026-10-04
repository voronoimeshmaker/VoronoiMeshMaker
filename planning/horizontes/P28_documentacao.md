# P28 — Manual e exemplos após os testes

## Entradas e precondições

Só iniciar exemplos depois dos testes de classes e integração de P27.
Ler [protocolo comum](02_protocolo.md), [requisitos](01_requisitos.md),
[ordem e dependências](00_sequencia.md) e os relatórios aprovados das etapas anteriores.
Não iniciar implementação antes de fechar as decisões necessárias.

## Tarefas

1. Explicar domínio/região, horizonte obrigatório, camada de discretização e coluna.
2. Documentar coordenadas, aproximação de superfícies, degenerações e limites suportados.
3. Explicar que colunas de Voronoi 2D não são em geral Voronoi euclidiano 3D;
   não prometer ortogonalidade de horizontes inclinados.
4. Documentar todas as funções novas com seções Doxygen do projeto e exemplos.
5. Adicionar exemplos puramente geométricos: faixas de profundidade e horizontes de terreno,
   com várias subdivisões dentro da mesma região. Nenhuma propriedade física.
6. Mostrar identificações e interfaces, arquivos exportados e uso do contrato pelo consumidor.
7. Se houver CLI, oferecer exemplo equivalente à API e verificar sua equivalência.
8. Atualizar manual pt/en, galeria, README e changelog com limites explícitos.
9. Executar build de documentação e exemplos conforme R30; registrar falhas e avisos.
10. Separar documentação do recurso entregue da tesselação futura ainda indisponível.

## Entregáveis

Documentação e exemplos executados, imagens da galeria geradas pelo pipeline existente,
relatorios/P28_documentacao.md.

## Critério de conclusão

Exemplos reproduzíveis passam; documentação não afirma suporte ausente.
Guia permite gerar, inspecionar e exportar malha em camadas sem conhecer o mohid-ng.

Ao terminar, registrar resultado e pendências e submeter à revisão prevista no protocolo.

## Conteúdo obrigatório do Sphinx

Explicar com precisão: plano fixo em 3D, reta por trecho em 2D; faces/arestas podem mudar
sem sair do suporte ou alterar a extensão regional. A regra também vale para CVT.
Distinguir contrato planejado, geração disponível e otimizador realmente implementado.
Atualizar a página inicial de planejamento criada nesta entrega quando o recurso for
implementado; não manter afirmações contraditórias entre guia, API, galeria e changelog.
