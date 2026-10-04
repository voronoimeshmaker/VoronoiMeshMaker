# P22/P23 — execução autônoma
Data: 2026-10-04. O João autorizou executar todas as etapas sem aprovação intermediária.
Essa instrução substitui as pausas de revisão do plano anterior; não muda o escopo geométrico.
Preservar mudanças existentes, não fazer commit/push.

Iterações: (1) prova analítica isolada; (2) representação fixa de horizontes em grade
retangular triangulada, com cotas ou amostragem de funções; (3) sobreposição exata da
base com essa triangulação; (4) montagem de poliedros por coluna/camada e contatos de
espessura zero; (5) IO, renumeração e CLI; (6) testes, documentação e relatório.
Não se supõe que um horizonte irregular possa ser reconstruído apenas nos vértices
da base: a grade de referência é independente da malha Voronoi e fica congelada.
A mesma subdivisão plana serve a todos os níveis. Encontros interiores são representados
pelas arestas da referência. Cruzamentos de horizontes são erros, não pinch-outs.
A união de prismas triangulares da mesma coluna/camada elimina faces interiores;
células podem ter partes desconectadas como no contrato existente.
