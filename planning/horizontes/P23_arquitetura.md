# P23 — Arquitetura e API aditiva

## Entradas e precondições

Depende de P22. Etapa documental.
Ler [protocolo comum](02_protocolo.md), [requisitos](01_requisitos.md),
[ordem e dependências](00_sequencia.md) e os relatórios aprovados das etapas anteriores.
Não iniciar implementação antes de fechar as decisões necessárias.

## Tarefas

1. Propor componentes para descrição dos horizontes, subdivisão vertical, identificação
   regional, construção e consulta. Nomes definitivos só após revisão.
2. Reutilizar Mesh<3>, CSR, IDs, Result, catálogo de erros, métricas e IO onde seus contratos
   forem válidos. Não criar dependência mesh → io para reconstruir os polígonos de base.
3. Definir mapa célula 3D → célula 2D/coluna, camada e região, e mapa face → horizonte
   ou face lateral. Distinguir ausência de metadado de índice zero.
4. Definir representação canônica das superfícies obrigatórias e sua proveniência.
   IDs de região não devem depender acidentalmente de índices de camadas.
5. Definir consulta acima/abaixo inclusive quando uma conexão envolver múltiplas faces
   ou quando Q03 permitir camadas ausentes. Não presumir vizinho único sem contrato.
6. Definir semântica de sites/referências celulares e das métricas em malhas não Voronoi 3D.
   Auditar hipóteses de ortogonalidade/orientação no verificador existente.
7. Planejar renumeração: remapear metadados junto à malha. Transformações que destruam
   colunas devem invalidar ou substituir explicitamente esse metadado.
8. Definir compatibilidade .vmesh: nunca inserir silenciosamente dados incompatíveis em v1.
   Comparar extensão versionada e arquivo complementar, incluindo leitores antigos e novos.
9. Definir registro aberto para CLI se aprovado, sem ampliar despacho fechado.
10. Planejar overflow de IDs/contagens, entradas não finitas, tolerâncias e erros.
11. Apresentar iterações pequenas para P24–P26 e impacto no grafo de dependências.

## Entregáveis

relatorios/P23_arquitetura.md com API proposta, modelos de dados, grafo, contrato de IO,
plano de testes por função pública e decisões de estabilidade.

## Critério de conclusão

Arquitetura revisada; nenhuma API estável muda de significado. Não expõe CGAL.
Há solução explícita para faces não planas, proveniência regional e metadados após renumeração.

Ao terminar, registrar resultado e pendências e submeter à revisão prevista no protocolo.

## Restrição de CVT e tesselação futura

A partição de referência não pode ser recalculada a partir dos sítios atualizados.
Prever acesso aos suportes e aos pares regionais sem implementar agora CVT.
O construtor deve distinguir suporte geométrico fixo de faces de malha variáveis,
em ambas as dimensões. Reutilizar esse contrato em vez de congelar IDs de faces.
