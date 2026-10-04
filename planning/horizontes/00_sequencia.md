# Sequência — colunas e horizontes no VMM

- Versão do plano: 1.0 — 2026-10-03.
- Estado: P21–P29 concluídos em 2026-10-04; versão 1.1.0 preparada para revisão, não publicada.
- Repositório: /home/jflavio/Programas/VMM, WSL Ubuntu-26.04-Test.
- Continuação de [sequencia_prompts.md](../sequencia_prompts.md), após P20.
- Base observada: API 1.0.0; commit da5c03e. Reconfirmar a árvore antes de executar.
- Natureza: geometria, topologia, identificadores e geração de malhas. O mohid-ng consome o VMM.

## Como usar

Ler [requisitos e pendências](01_requisitos.md) e [protocolo](02_protocolo.md).
Executar uma etapa por vez. Cada etapa começa lendo os resultados aprovados das anteriores.
Os arquivos P21–P29 são instruções de trabalho, não relatórios de resultados.
As decisões já expressas pelo João estão registradas como confirmadas em 01_requisitos.md.
As recomendações ainda não respondidas não são autorização de implementação.

O João autorizou posteriormente executar todas as etapas sem aprovação intermediária.
Registrar resultados e evidência a cada etapa e prosseguir autonomamente; esta autorização
substitui a espera por revisão mencionada nos documentos iniciais.

## Ordem de execução

| Etapa | Objetivo | Depende de | Entregável para revisão |
|---|---|---|---|
| [P21](P21_contrato.md) | Fechar contrato geométrico e decisões pendentes | respostas do João | contrato e matriz de aceitação |
| [P22](P22_prova_geometrica.md) | Provar extrusão conforme e horizontes | P21 | protótipo isolado e evidência |
| [P23](P23_arquitetura.md) | Fixar API, representação e compatibilidade | P22 | arquitetura e plano de iterações |
| [P24](P24_colunas_planas.md) | Gerar colunas com horizontes planos | P23 | código e GTest por classe |
| [P25](P25_horizontes_irregulares.md) | Generalizar para superfícies e casos aprovados | P24 | código e GTest por classe |
| [P26](P26_integracao_saida.md) | Integrar consultas, IO e, se aprovado, configuração | P25 | testes de integração e persistência |
| [P27](P27_validacao.md) | Validar geometria, regressões, cobertura e desempenho | P26 | evidência consolidada |
| [P28](P28_documentacao.md) | Documentar e adicionar exemplos após os testes | P27 | manual pt/en e galeria verificada |
| [P29](P29_entrega.md) | Revisar e preparar versão compatível | P28 | relatório final e entrega revisável |

Não fixar número de versão de produto nem datas de entrega antes de P21/P23.
A hipótese é uma versão menor compatível da série 1.x; a decisão final depende do contrato.

## Tesselação posterior

[T01 — transformação futura](T01_tesselacao_futura.md) descreve o contrato que uma futura
tesselação deverá satisfazer. Não é dependência de P21–P29 e não autoriza implementá-la.
A nova geração não deve impedir transformações futuras nem fingir que uma transformação
preserva colunas quando deixa de preservá-las.

## Limites da entrega

Não implementar propriedades físicas, equações, condições físicas de contorno, transporte,
acoplamento a solver, interpolação de campos físicos, ALE ou evolução temporal.
Não alterar a geração Voronoi 2D/3D existente sem necessidade demonstrada.
Não chamar a malha de colunas de Voronoi euclidiano 3D: ela deriva de uma base Voronoi 2D.

## Histórico

| Versão | Data | Alteração |
|---|---|---|
| 1.0 | 2026-10-03 | Plano inicial P21–P29, requisitos confirmados, perguntas abertas e contrato futuro T01 |

## Atualização durante a elaboração

O João explicitou que CVT deve preservar o plano da interface em 3D e a reta de cada
trecho em 2D, permitindo que as faces/arestas se reorganizem dentro desse suporte.
A documentação Sphinx deve acompanhar esse contrato. H09/H10 e T01 registram essas
instruções; não se pressupõe autorização para implementar agora um otimizador CVT.


## Andamento — 2026-10-03

P21 iniciado e contrato consolidado com respostas explícitas do João.
Ver [relatório P21](relatorios/P21_contrato.md). Encontros/desaparecimento de domínios,
superfícies facetadas, bases gerais, API e CLI, persistência e casos sintéticos estão
incluídos. P22 ainda não executado; não há implementação nova nem validação numérica.

## Andamento — 2026-10-04

P21 fechado. Implementados grade de referência, refinamento comum, colunas com
horizontes facetados e desaparecimento, consultas, persistência e composição CLI.
Ver [validação incremental](relatorios/P27_incremental_20261004.md).
P27 continua: cobertura, matriz completa, desempenho e casos patológicos.
P28/P29 ainda não concluídos. Não interpretar o estado histórico abaixo como
ausência da implementação atual. Sem commit ou push.

## Conclusão — 2026-10-04

A autorização posterior do João permitiu completar todas as etapas sem revisão
intermediária. Registros anteriores são históricos; o estado final é este.

- P21: [contrato](relatorios/P21_contrato.md).
- P22–P26: [implementação e arquitetura](relatorios/P22_P26_implementacao.md).
- P27: [validação consolidada](relatorios/P27_validacao.md) e [desempenho](relatorios/P27_desempenho.md).
- P28: [documentação executada](relatorios/P28_documentacao.md).
- P29: [entrega, problemas, limitações e riscos](relatorios/P29_entrega.md).

310 testes Release; matriz complementar e cobertura descritas nos relatórios.
Sphinx pt/en e consumo instalado verificados. Os 16 avisos por idioma da
referência API estão documentados. Não houve commit, tag ou push.
T01/CVT permanece futuro; a conclusão não amplia esse escopo.
