# P26 — Integração, identificação e saída

## Entradas e precondições

Depende dos testes por classe de P24/P25.
Ler [protocolo comum](02_protocolo.md), [requisitos](01_requisitos.md),
[ordem e dependências](00_sequencia.md) e os relatórios aprovados das etapas anteriores.
Não iniciar implementação antes de fechar as decisões necessárias.

## Tarefas

1. Integrar a geração à API pública como opção aditiva; manter geração Voronoi 3D existente.
2. Testar pipeline base Voronoi 2D real → horizontes → malha 3D → invariantes e métricas.
3. Expor consultas por coluna, camada, região e horizonte conforme arquitetura, sem
   reconstrução geométrica pelo consumidor.
4. Integrar renumeração aos metadados e testar permutação/inversa, vizinhos e identificadores.
5. Implementar persistência aprovada, preservando leitura de .vmesh v1 e diagnosticando
   versões desconhecidas. Não anunciar round-trip completo se perder metadados.
6. Integrar VTU com células poliédricas válidas e identificação de células; definir como
   visualizar interfaces sem inventar semântica física. Testar leitor apropriado disponível.
7. Se Q06 incluir CLI, adicionar configuração validada com erros por linha/seção e
   ponto de extensão aberto. Se excluída, documentar explicitamente o acesso apenas pela API.
8. Testar round-trip de geometria, conectividade, domínio, coluna e camada; erro de arquivos
   incompatíveis/corrompidos e consistência entre arquivos complementares se utilizados.
9. Manter consumidor de teste mínimo em C++, sem depender do mohid-ng.

## Entregáveis

Integração testada, IO/configuração conforme escopo e relatório com matriz
API × metadados × formatos.

## Critério de conclusão

Consumidor identifica domínios e interfaces sem inferir propriedades.
Renumeração e persistência não corrompem identificações; regressões de IO antigo verificadas.

Ao terminar, registrar resultado e pendências e submeter à revisão prevista no protocolo.
