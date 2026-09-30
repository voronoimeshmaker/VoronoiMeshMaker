.. SPDX-License-Identifier: BSD-3-Clause

=====================
Diretrizes do projeto
=====================

Regras vigentes do VoronoiMeshMaker, verificáveis pela integração contínua ou pela revisão de cada iteração. Cada regra cita o requisito (``planning/P05_BASELINE.md``) e a decisão (``planning/DECISIONS.md``) de origem.

Arquitetura
-----------

- Sem funções virtuais, sem herança e sem enum de despacho: a extensão é feita por templates, concepts, traits, policies, composição e registros abertos (R3, DEC-024).
- Enum de dado é permitido (severidade, idioma, lado de uma face); enum cujo valor escolhe entre implementações é proibido (DEC-024).
- Dados em arrays contíguos, com identificadores fortes e conectividade compacta (CSR); dados base separados dos derivados (R4).
- O grafo de dependências entre módulos não tem ciclos, e o núcleo não depende de IO nem do backend (R4). A CI extrai os includes entre módulos e falha em ciclo ou dependência proibida.
- API pública pequena e só com tipos do VMM: nenhum tipo do CGAL ou de outra biblioteca externa aparece nela (R8, R19, R20).

Domínio, regiões e sítios
-------------------------

- O domínio tem uma ou mais regiões de fronteira fixa; cada região pertence a um meio de um registro aberto (R5).
- As regiões são declaradas por precedência e convertidas numa partição explícita, verificada por um validador que rejeita vazios e informa regiões anuladas e lascas (R15, DEC-018).
- Os contornos das formas são simples: uma aresta não pode tocar nem cruzar outra aresta da mesma forma, e os buracos não tocam o anel externo nem uns aos outros.
- Interfaces entre regiões são conformes e rotuladas pelo par de regiões, inclusive entre regiões do mesmo meio (R6).
- A célula é a de Voronoi recortada pelo domínio e pelas interfaces; a malha entregue é imutável (R7).

Invariantes, tolerâncias e determinismo
---------------------------------------

- Toda malha entregue satisfaz os invariantes: medidas por região e total, fechamento de cada célula, owner e neighbour consistentes, faixas de patch completas, interfaces entre regiões diferentes e adjacência simétrica (R16, DEC-011).
- A ortogonalidade é garantida pela construção de Voronoi; a não ortogonalidade é medida e relatada, não usada como critério de rejeição (DEC-032).
- Tolerâncias são relativas à diagonal *L* da caixa envolvente, com padrão 1e-12 *L*; constantes absolutas são proibidas no código geométrico (R17, DEC-020).
- Predicados de topologia são exatos (backend CGAL).
- Mesma entrada, semente e build produzem malha idêntica bit a bit; outra ordem dos sítios produz a mesma topologia e a mesma numeração canônica. Os geradores aleatórios são próprios e portáveis (R18, DEC-020).

Backend geométrico
------------------

- O CGAL é o único backend e responde a perguntas geométricas; o VMM constrói e possui a malha (DEC-002, DEC-007).
- Os headers do CGAL só entram nos fontes de ``vmm_backend_cgal``; ``vmm_core`` não depende do CGAL (R19). A CI compila cada header público sem o CGAL.

Erros e mensagens
-----------------

- Falhas previsíveis voltam como ``vmm::Result<T>`` (``std::expected``); condições excepcionais usam ``vmm::Exception``, que não deriva de ``std::exception`` (R11, DEC-016).
- Cada erro tem código estável, categoria, contexto, origem e, quando existir, a entidade relacionada; os textos vêm de um catálogo em português e inglês (R10).

Linguagem, dependências e licença
---------------------------------

- C++23; compiladores mínimos GCC 14 e Clang 18 (R12, DEC-014). Nome ``VoronoiMeshMaker``, namespace ``vmm`` (R13).
- Includes agrupados em biblioteca padrão, bibliotecas externas e VMM, cada grupo em ordem alfabética, com os comentários separadores do projeto.
- Dependências: CGAL e Boost; GoogleTest só nos testes; qualquer outra exige uma nova decisão (R21). Só versões estáveis (R22).
- ``vmm_core`` sob BSD-3-Clause; ``vmm_backend_cgal`` sob GPL-3.0-or-later; todo arquivo tem cabeçalho SPDX (R23).
- Sem ``-ffast-math``; arquitetura nativa e LTO desligadas por padrão; ``-Werror`` nos alvos do VMM; nenhum binário versionado; fins de linha LF (R24, DEC-012).

Testes
------

- Toda classe não trivial tem o seu ``ut_<Classe>.cpp``, em ``vmm/tests/<módulo>/<Classe>/`` (R1, R2); as classes triviais ficam listadas em ``vmm/tools/ci/trivial_classes.txt`` (DEC-023).
- Cobertura por arquivo: pelo menos 90 % das linhas e 80 % dos ramos, com exceções justificadas (R25).
- Além dos testes por classe: invariantes, arquivos de referência (oráculo O1–O4) e casos patológicos (R26).
- Primeiro os testes das funções públicas de cada classe, depois os de integração; só então um exemplo entra na galeria (R30).

Qualidade, saída e documentação
-------------------------------

- Métricas de qualidade (não ortogonalidade, distorção, razão de aspecto, faces curtas) com limites configuráveis (R14); benchmark B1–B3 a cada entrega (R27).
- Views de faces internas, de contorno por patch e de interface por par de regiões, e adjacência compacta, suficientes para montar matrizes esparsas (R9, DEC-015).
- A reordenação guarda a permutação e a sua inversa.
- Persistência no formato nativo ``.vmesh``; visualização em VTK XML ``.vtu`` (R28, DEC-019, DEC-030).
- Referência da API com as seções Parameters, Notes, Level, See Also, Location e Examples; um exemplo que falha quebra o build da documentação (R29).
