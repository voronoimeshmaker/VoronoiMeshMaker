# P29 — Revisão e preparação da entrega

## Entradas e precondições

Depende de P28. Publicação e commits pertencem ao João.
Ler [protocolo comum](02_protocolo.md), [requisitos](01_requisitos.md),
[ordem e dependências](00_sequencia.md) e os relatórios aprovados das etapas anteriores.
Não iniciar implementação antes de fechar as decisões necessárias.

## Tarefas

1. Revisar rastreabilidade H01–H10 e respostas Q01–Q10 contra implementação e evidência.
2. Auditar assinatura/semântica de APIs estáveis, formatos, erros e compatibilidade.
3. Conferir instalação e consumo externo via find_package, headers públicos e executável
   quando incluído, usando a biblioteca construída na árvore ativa.
4. Escolher versão de produto conforme DEC-041; manter versões CMake/header sincronizadas.
   Não declarar versão publicada, criar tag ou fazer push.
5. Atualizar decisões e relatórios sem reescrever aprovações históricas.
6. Separar realizado, limitações aceitas e futuro T01. Não marcar tesselação posterior
   como implementada em virtude da existência do refinamento comum das interfaces.
7. Preparar diff revisável, lista de arquivos e comandos de reprodução.
8. Registrar a validação final realmente executada e eventuais lacunas.

## Entregáveis

relatorios/P29_entrega.md com status por requisito, compatibilidade, evidência,
riscos e instruções de reprodução; arquivos prontos para revisão do João.

## Critério de conclusão

Entrega atende ao contrato aprovado; nenhum pendente obrigatório escondido.
Commit, tag e publicação são ações posteriores do João, não evidência de conclusão técnica.

Ao terminar, registrar resultado e pendências e submeter à revisão prevista no protocolo.
