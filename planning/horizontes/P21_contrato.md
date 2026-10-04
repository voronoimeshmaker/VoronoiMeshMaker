# P21 — Contrato geométrico e fechamento dos requisitos

## Entradas e precondições

Nenhum código de produção.
Ler [protocolo comum](02_protocolo.md), [requisitos](01_requisitos.md),
[ordem e dependências](00_sequencia.md) e os relatórios aprovados das etapas anteriores.
Não iniciar implementação antes de fechar as decisões necessárias.

## Tarefas

1. Ler H01–H10 e manter seu caráter confirmado.
2. Obter respostas a Q01–Q10; agrupar perguntas dependentes e não repetir respostas existentes.
3. Definir domínios/regiões, camadas internas, horizontes obrigatórios, patches, colunas e IDs.
4. Incorporar o suporte fixo confirmado: plano em 3D, reta por trecho em 2D, com subdivisão livre dentro da fronteira finita, inclusive em CVT. Definir unidades, orientação vertical, ordenação dos horizontes e referência geométrica
   que deve ser preservada. Não aceitar mudança posterior da fronteira sob pretexto de tolerância.
5. Definir suporte/rejeição de encontros, cruzamentos, espessura nula, regiões ausentes,
   buracos e fragmentos da base. Pinch-out suportado e entrada inválida são casos distintos.
6. Definir se fronteiras laterais também têm que ser preservadas (H06 se aplica a todas as interfaces).
7. Escolher contratos de entrada/saída e dois casos geométricos de aceitação, sem dados físicos.
8. Definir a matriz de rastreabilidade requisito → API proposta → teste → etapa.

## Entregáveis

Contrato em relatorios/P21_contrato.md, questionário respondido e matriz de aceitação.
Registrar decisões técnicas propostas no registro central quando necessário.
Não considerar o silêncio uma escolha.

## Critério de conclusão

Todas as questões que alteram geometria, topologia ou formato da primeira entrega têm resposta.
Cada limitação é explícita e aceita; não há implementação dependente de hipótese não confirmada.

Ao terminar, registrar resultado e pendências e submeter à revisão prevista no protocolo.
