# P04 — Problemas-âncora, consumidores da malha e metas

- **Data:** 2026-09-28
- **Sequência:** v5.1, prompt P04
- **Destino no repositório:** `planning/P04_ancora_metas.md`
- **Convenção:** **[F]** fato verificado (com fonte) · **[I]** inferência · **[R]** recomendação · **NÃO VERIFICADO** quando não foi possível obter.
- **Natureza:** documental. Nenhum código, CMake ou teste foi alterado. As metas numéricas são **propostas** para o João validar (§9); nenhuma foi medida.

---

## 1. Ambiente

| Item | Valor |
|---|---|
| Onde rodou | sessão do Claude na nuvem, fora do WSL oficial |
| Repositório lido | `planning/` do GitHub, commit `ceec0dc` |
| Build, testes, cobertura | não executados (prompt documental) |
| Fontes externas | documentação oficial consultada em 28/09/2026, com link em cada linha da §4 |

---

## 2. Problemas-âncora

Três problemas, em ordem crescente de dificuldade. A1 e A2 são 2D (entrega (a)); A3 é 3D (Fase 3). Os três usam só geometria sintética e reprodutível, gerada por parâmetros, para que virem testes de integração sem depender de dados externos. Dados reais (GIS, DEM) ficam como variante opcional.

### 2.1 A1 — Seção transversal de rio (2D, plano x–z)

**a) Geometria**
- Retângulo de 200 m (x) × 15 m (z), de z = −15 m até z = 0, que é o nível d'água e o topo do terreno.
- Canal trapezoidal centrado: largura no topo 40 m, profundidade 5 m, taludes 1V:2H (fundo com 20 m).
- Contato entre duas camadas de solo em z = −8 m, abaixo do canal (não o intercepta).
- Variante A1-ar: o retângulo sobe até z = +5 m, com uma faixa de ar acima do terreno e da lâmina d'água.
- **Não convexidade:** o solo superior tem forma de "U" em volta do canal.
- **Arestas vivas:** os 4 vértices do trapézio; os cantos do retângulo.
- **Junções triplas:** só na variante A1-ar, nos dois pontos onde água, solo e ar se encontram (bordas do canal no nível d'água).
- Sem buracos.

**b) Meios, regiões e interfaces**

| Região | Meio |
|---|---|
| `canal` | água |
| `solo_sup` | sólido |
| `solo_inf` | sólido |
| `atmosfera` (só A1-ar) | gás |

- Interfaces: `canal|solo_sup` (leito e taludes); `solo_sup|solo_inf` (**duas regiões do mesmo meio**, R6); na variante, `canal|atmosfera` e `solo_sup|atmosfera`.

**c) Patches de contorno**
- `base` (z = −15 m), `lateral_esq`, `lateral_dir` (cortam as duas camadas de solo e, na variante, o ar);
- sem a variante: `superficie_agua` (topo do canal) e `terreno` (topo do solo fora do canal);
- com a variante: `topo_ar`.

**d) Declaração das regiões** (ver §3): `solo_inf` = retângulo inteiro; `solo_sup` = semiplano z > −8 m, por cima; `canal` = trapézio, por cima; `atmosfera` = z > 0, por cima.

**e) Tamanho**
- 10³ a 10⁵ células.
- Densidade variável: espaçamento de 0,1 m junto ao leito e às interfaces, crescendo até 5 m longe delas (razão ~50).

**f) Dados de entrada:** cinco parâmetros geométricos e a lei de espaçamento dos sítios. Nada externo.

**g) Invariantes e oráculo**
- soma das áreas das células = área do retângulo; soma por região = área analítica de cada região;
- fechamento de cada célula; consistência owner/neighbour;
- comprimento total das faces de cada interface = comprimento analítico da interface (leito + taludes; contato entre camadas);
- conformidade: toda face de interface tem exatamente uma célula de cada lado.
- **Oráculo da VMMLib:** não se aplica (domínio não convexo por região e multirregião). Só os invariantes.

### 2.2 A2 — Trecho de rio em planta, com meandro e ilha (2D, plano x–y)

**a) Geometria**
- Retângulo de planície de 2 km × 1 km.
- Canal meandrante de largura 30 m, com eixo dado por uma curva gerada por seno (*sine-generated curve*), amplitude e comprimento de onda paramétricos.
- Uma ilha de solo dentro do canal (elipse de 60 m × 15 m, no trecho mais largo, onde o canal alarga para 80 m).
- **Não convexidade forte** (meandros); **buraco** na região de água (a ilha); **múltiplos componentes** do meio sólido (planície e ilha não se tocam).
- **Arestas vivas:** as extremidades do canal nas fronteiras de entrada e saída.
- **Junções triplas:** não há (a ilha só toca a água).

**b) Meios, regiões e interfaces**

| Região | Meio |
|---|---|
| `canal` | água |
| `planicie` | sólido |
| `ilha` | sólido |

- Interfaces: `canal|planicie` (as duas margens); `canal|ilha`.
- `planicie` e `ilha` são duas regiões do mesmo meio que **não** se tocam (testa rótulos por região, não só por meio).

**c) Patches de contorno**
- `montante` e `jusante` (as duas seções de canal nas bordas do retângulo);
- `limite_planicie` (resto do contorno do retângulo, no solo).

**d) Declaração:** `planicie` = retângulo; `canal` = polígono do canal, por cima; `ilha` = elipse, por cima.

**e) Tamanho**
- 10⁴ a 10⁶ células.
- Espaçamento de ~2 m no canal, crescendo até 50 m na planície (razão ~25); refinamento extra de 0,5 m em volta da ilha.

**f) Dados de entrada**
- Sintético: eixo do canal por parâmetros (amplitude, comprimento de onda, número de meandros).
- Variante real (opcional): margens em polilinha lidas de arquivo (CSV ou WKT). Leitura de Shapefile/GeoJSON fica fora do núcleo (DEC-001, DEC-004).

**g) Invariantes e oráculo**
- os mesmos de A1, com a área analítica do canal calculada pelo polígono de entrada;
- a região `ilha` tem exatamente as células cujos sítios estão dentro dela, e nenhuma célula atravessa a interface;
- a adjacência entre `planicie` e `ilha` é vazia.
- **Oráculo da VMMLib:** não se aplica.

### 2.3 A3 — Bloco de solo com rio (3D, Fase 3)

**a) Geometria**
- Caixa de 500 m (x) × 200 m (y) × 30 m (z).
- Canal com seção trapezoidal cuja profundidade varia ao longo de x (de 3 m a 6 m) e cujo eixo tem uma curva suave em planta: **não é uma extrusão** da A1.
- Duas camadas de solo com contato inclinado; ar acima do terreno.
- **Arestas vivas:** as arestas do trapézio ao longo do canal; as arestas da caixa.
- **Junções triplas:** curvas onde água, solo e ar se encontram (as bordas do canal no nível d'água).

**b) Meios e regiões:** `canal` (água), `solo_sup` e `solo_inf` (sólido), `atmosfera` (gás). Interfaces como na variante A1-ar, agora superfícies.

**c) Patches:** `base`, quatro laterais (`montante`, `jusante`, `margem_esq`, `margem_dir`) e `topo_ar`.

**d) Declaração:** a mesma ordem de A1-ar. Variante da entrega (c): o terreno vem de uma superfície triangulada (STL).

**e) Tamanho:** 10⁵ a 10⁷ células; espaçamento de 0,5 m junto ao leito a 10 m no ar e no solo profundo.

**f) Dados de entrada:** parâmetros analíticos; na variante (c), um STL do terreno gerado a partir dos mesmos parâmetros (para ter a resposta analítica).

**g) Invariantes:** soma dos volumes; soma dos vetores de área de cada célula = 0; área de cada interface = área analítica; conformidade das faces de interface; comprimento das junções triplas. Sem oráculo da VMMLib (ela é só 2D).

---

## 3. Declaração de regiões

| Opção | Como o usuário declara | A favor | Contra |
|---|---|---|---|
| **P — Precedência** (CSG por diferença, na ordem declarada) | lista de regiões; cada nova região "pinta por cima" das anteriores | simples de escrever; sem vazios se a primeira região cobre o domínio; cobre A1, A2 e A3 sem esforço | depende da ordem; uma sobreposição não intencional fica escondida; fronteiras quase coincidentes geram lascas |
| **E — Partição explícita** | cada região com o seu contorno completo; as fronteiras compartilhadas precisam coincidir | explícito; nada depende da ordem | o usuário tem de construir as fronteiras comuns exatamente iguais, o que é difícil em 2D e impraticável em 3D |
| **L — Função rótulo** | uma função x → id da região | geral; natural para STL e para dados em grade | a interface fica implícita; arestas vivas e junções triplas se perdem sem tratamento especial |
| **A — Arranjo exato** | curvas (2D) ou superfícies (3D) que dividem o domínio; regiões por face do arranjo | conformidade exata por construção | pesado para o usuário e para o núcleo; em 3D depende de Nef/PMP |

**[R]** API por **precedência (P)**, convertida internamente numa **partição explícita** das regiões, com um **validador** que:
1. garante que todo ponto do domínio pertence a exatamente uma região (sem vazios, exceto se houver uma região de fundo declarada);
2. informa as regiões que ficaram vazias ou fragmentadas pela ordem (por exemplo, uma região inteiramente coberta por outra);
3. informa lascas: partes de região com medida abaixo de uma fração da escala local, típicas de fronteiras quase coincidentes.

A função rótulo (L) fica como mecanismo interno de consulta e, no 3D com STL, como alternativa a avaliar no P15a. A representação interna da partição é decidida no P06.

---

## 4. Consumidores e formatos

| Formato | Consumidores | O que representa | Biblioteca externa (DEC-004) | Esforço de um escritor | Fonte |
|---|---|---|---|---|---|
| **VTK XML (.vtu)** e legado | ParaView, VisIt, pós-processamento em geral | células poligonais (2D) e poliédricas (3D, via lista de faces); dados por célula (região, métricas) | nenhuma (texto ou binário em base64) | baixo; já existe na VMMLib para o 2D (`VTK_XML_ClippedVoronoi2D`, `VTK_Legacy_ClippedVoronoi2D`) | [F] [VTK XML file format](https://docs.vtk.org/en/latest/vtk_file_formats/vtkxml_file_format.html) |
| **OpenFOAM polyMesh** | OpenFOAM e derivados | `points`, `faces`, `owner`, `neighbour`, `boundary` (patches com `nFaces`/`startFace`); células poliédricas sem limite de faces; normal da face interna aponta para a célula de maior índice | nenhuma (texto) | médio; o 2D exige extrudar uma camada de células com patches `empty` na frente e atrás | [F] [OpenFOAM user guide 4.1](https://www.openfoam.com/documentation/user-guide/4-mesh-generation-and-conversion/4.1-mesh-description); [células poliédricas](https://doc.cfd.direct/openfoam/user-guide-v13/mesh-description); [`empty`](https://www.openfoam.com/documentation/guides/latest/doc/guide-bcs-constraint-empty.html) |
| **MODFLOW 6 DISV** | MODFLOW 6 (água subterrânea), FloPy | 2D por camadas: vértices e células (`CELL2D`: centro e lista de vértices); **vértices em sentido horário**; células conectadas precisam compartilhar vértices | nenhuma (texto) | baixo para o 2D em planta (A2) | [F] [GWF-DISV](https://modflow6.readthedocs.io/en/latest/_mf6io/gwf-disv.html) |
| **MODFLOW 6 DISU** | MODFLOW 6 | conectividade CSR (`IAC`, `JA`) e, por conexão, `IHC`, `CL12` (distância do centro à face), `HWVA` (largura ou área), ângulo da normal; vértices opcionais | nenhuma (texto) | baixo; mapeia direto na adjacência do VMM (DEC-015) | [F] [GWF-DISU](https://modflow6.readthedocs.io/en/latest/_mf6io/gwf-disu.html) |
| **PFLOTRAN UNSTRUCTURED_EXPLICIT** | PFLOTRAN | `CELLS`: id, centro e volume; `CONNECTIONS`: par de células, centro da face e área. A documentação observa que, em células de Voronoi, os centros "não são necessariamente o centroide" | nenhuma (texto) | baixo | [F] [UNSTRUCTURED_EXPLICIT](https://www.pflotran.org/documentation/user_guide/cards/subsurface/grids/unstructured_explicit_grid.html) |
| **TOUGH MESH** | TOUGH2/TOUGH3 | `ELEME`: elemento, material, volume, coordenadas; `CONNE`: par de elementos, orientação, distâncias D1 e D2 até a interface, área, componente da gravidade | nenhuma (texto de colunas fixas) | baixo; nomes de elemento de 5 caracteres limitam a numeração | [F] [TOUGH3 User's Guide](https://escholarship.org/content/qt0dh2w4w6/qt0dh2w4w6.pdf) |
| **CGNS** | solvers CFD de uso geral | poliedros por `NGON_n` (faces) e `NFACE_n` (células); `ParentElements` dá as duas células de cada face | **biblioteca CGNS (licença do tipo zlib) + HDF5** | médio a alto; nova dependência | [F] [CGNS SIDS §7](https://cgns.org/standard/SIDS/grid.html); [README e licença](https://github.com/CGNS/CGNS/blob/develop/README.md) |
| **Gmsh (.msh)** | Gmsh, solvers de elementos finitos | elementos de forma fixa (linhas, triângulos, quadriláteros, tetraedros, prismas, hexaedros, pirâmides) | nenhuma | — | [F] [Manual do Gmsh](https://gmsh.info/doc/texinfo/gmsh.html) |

**[I]**
- Os formatos de subsuperfície (DISU, PFLOTRAN, TOUGH) descrevem a malha como **lista de conexões com distâncias e áreas**, exatamente o que o modelo de volumes finitos do VMM já calcula. São escritores baratos e mostram o valor do VMM para o público ambiental.
- O Gmsh não representa células poliédricas gerais: exportar Voronoi para ele exigiria triangular as células, o que descaracteriza a malha. Fica fora.
- O CGNS é o formato neutro mais completo, mas traz HDF5 como dependência. Pela DEC-004, só entra como opcional, e mais tarde.
- A ordem das faces que o OpenFOAM espera além da orientação (faces internas antes das de contorno, agrupadas por célula dona): **NÃO VERIFICADO** na documentação oficial consultada; confirmar no código-fonte antes do escritor.

**[R] Formatos**
1. **Primeira versão (0.1):**
   - **VTK XML** (verificação visual e galeria; parte já existe);
   - **OpenFOAM polyMesh** (principal consumidor de volumes finitos poliédricos);
   - **formato nativo do VMM**, simples e versionado, com o modelo de volumes finitos completo. Serve para os golden files (DEC-011) e para quem constrói as próprias estruturas (DEC-015). O formato em si é decidido no P06.
2. **Segunda versão (0.2):** MODFLOW 6 (DISV para o 2D em planta; DISU para o 3D), PFLOTRAN e TOUGH.
3. **Opcional, sem prazo:** CGNS, atrás de uma opção de build.
4. **Fora:** Gmsh.

Todos como escritores fora do núcleo (DEC-001), sem tipos de solver na API (DEC-015).

---

## 5. Metas mensuráveis

Valores propostos para validação (§9). A linha de base é medida no P07, no ambiente oficial, e as metas podem ser recalibradas uma vez com essa medida.

| Meta | 2D | 3D | Método de medição |
|---|---|---|---|
| **Tempo de geração** | 10⁶ células em até 30 s, Release, 1 thread | 10⁶ células em até 10 min, 1 thread (revisar depois do P15a) | tempo de parede por fase (triangulação, células, recorte, topologia, métricas), com `std::chrono`, mediana de 5 execuções |
| **Crescimento** | aproximadamente N log N | idem | regressão log-log em N = 10⁴, 10⁵, 10⁶ |
| **Memória** | até 1 KB por célula no pico | até 4 KB por célula no pico | pico de RSS (`/usr/bin/time -v`) dividido por N |
| **Não ortogonalidade** | faces internas de Voronoi: ângulo entre d_ij e a normal abaixo de 10⁻⁸ rad | idem | métrica calculada pelo próprio VMM, máximo por região |
| **Faces e células mínimas** | nenhuma face com comprimento abaixo de 10⁻³ do espaçamento local depois do colapso de arestas curtas | nenhuma face com área abaixo de 10⁻⁶ do espaçamento local ao quadrado | histograma relatado pelo VMM; células abaixo do limite listadas |
| **Razão de aspecto** | relatada, não limitada (depende dos sítios do usuário) | idem | máximo e percentil 99 por região |

**Determinismo**
- **Mesma entrada, configuração, semente e build** (compilador, flags e versões registradas): malha idêntica **bit a bit**, incluindo a numeração de células e faces.
- **Número de threads diferente** (quando houver paralelismo): mesmo resultado bit a bit.
- **Outra plataforma ou compilador:** mesma topologia e numeração; geometria dentro da tolerância relativa.
- **Outra ordem de inserção dos sítios:** mesma topologia depois da numeração canônica (por identidade do sítio); geometria dentro da tolerância. O P03 mediu diferenças de até 1,1e-16 nas áreas.

**Tolerâncias (DEC-011)**, relativas à escala L = diagonal da caixa envolvente do domínio:
- igualdade geométrica de pontos: 10⁻¹² L;
- soma das medidas das células contra a medida do domínio e de cada região: erro relativo até 10⁻¹²;
- fechamento de cada célula: |Σ S_f| até 10⁻¹² L^(d−1);
- comparação com golden files: topologia exata; grandezas geométricas com erro relativo até 10⁻¹².

**[I]** As tolerâncias relativas eliminam o problema do `kEpsilon = 1e-6` absoluto da VMMLib (P03 §9) com domínios em UTM (~10⁶ m) ou em milímetros.

---

## 6. Casos do benchmark leve (DEC-013)

| Caso | Base | Tamanho | Oráculo da VMMLib | O que se mede |
|---|---|---|---|---|
| **B1** | quadrado unitário, sítios uniformes com semente fixa | 10⁶ células | **sim** (domínio convexo de um anel) | tempo por fase, pico de memória, invariantes, soma de verificação da topologia; comparação com a VMMLib |
| **B2** | A1 sem a variante de ar | ~5·10⁴ células | não | tempo, memória, invariantes por região e por interface |
| **B3** | A2 | ~5·10⁵ células | não | tempo, memória, invariantes, conformidade da interface da ilha |

- Os três rodam em Release, com a mesma semente, e o resultado vai para o relatório de cada entrega.
- Tamanhos pequenos o bastante para rodar em poucos minutos no WSL; o B1 é o único grande, porque é o que permite comparar com a VMMLib.

---

## 7. Riscos

| # | Risco | Mitigação |
|---|---|---|
| 1 | As metas de tempo e memória são estimativas, sem medida | recalibração única no P07, com a linha de base do ambiente oficial |
| 2 | Fronteiras quase coincidentes na precedência geram lascas | validador da §3 e tolerância relativa |
| 3 | O 2D no OpenFOAM exige extrusão e patches `empty`; erro de orientação invalida a malha | teste de integração que roda `checkMesh` quando o OpenFOAM estiver disponível (opcional na CI) |
| 4 | MODFLOW 6 exige vértices em sentido horário; o VMM usa anti-horário | conversão no escritor, com teste |
| 5 | Nomes de 5 caracteres do TOUGH limitam malhas grandes | codificação documentada; aviso acima do limite |
| 6 | Dados reais (GIS, DEM) não reproduzíveis na CI | casos sintéticos por parâmetros; variantes reais só na galeria |
| 7 | O A3 depende do resultado do P15a | A3 é alvo, não compromisso; revisado depois do P15a |

---

## 8. Entradas propostas para `planning/DECISIONS.md`

- **DEC-017 — Problemas-âncora A1, A2 e A3** (PROPOSTA).
- **DEC-018 — Declaração de regiões por precedência, com partição interna e validador** (PROPOSTA).
- **DEC-019 — Formatos de saída por versão** (PROPOSTA).
- **DEC-020 — Metas, determinismo e tolerâncias relativas** (PROPOSTA).
- **DEC-021 — Casos do benchmark leve B1, B2 e B3** (PROPOSTA).

O texto completo está em `planning/DECISIONS.md`.

---

## 9. Questionário

1. **Problemas-âncora:** A1 (seção transversal) e A2 (planta com meandro e ilha) para o 2D, e A3 para o 3D?
   - (a) sim, como estão **[recomendado]**; (b) trocar um deles; (c) acrescentar outro.
2. **Variante com ar no A1:** entra na entrega (a)?
   - (a) sim, porque é o único caso 2D com junções triplas **[recomendado]**; (b) não, fica para depois.
3. **Dados reais:** algum trecho de rio com dados que você queira como variante da galeria?
   - (a) não por ora, só sintéticos **[recomendado]**; (b) sim (indicar qual).
4. **Declaração de regiões:** precedência na API, com partição interna e validador?
   - (a) sim **[recomendado]**; (b) partição explícita; (c) função rótulo.
5. **Formatos da 0.1:** VTK XML, OpenFOAM e formato nativo?
   - (a) sim **[recomendado]**; (b) incluir MODFLOW 6 já na 0.1; (c) outro conjunto.
6. **CGNS:** opcional sem prazo, ou fora?
   - (a) opcional sem prazo **[recomendado]**; (b) fora.
7. **Solver que você usa:** qual solver de volumes finitos vai consumir a malha nos seus trabalhos (o seu próprio, OpenFOAM, outro)?
   - resposta livre. Define a prioridade entre os escritores.
8. **Metas de tempo:** 10⁶ células em 30 s (2D) e 10 min (3D), recalibráveis uma vez no P07?
   - (a) sim **[recomendado]**; (b) mais rígidas; (c) só relatar, sem meta.
9. **Determinismo:** bit a bit no mesmo build; topologia idêntica entre plataformas?
   - (a) sim **[recomendado]**; (b) só topologia idêntica, sempre.
10. **Tolerância relativa:** 10⁻¹² L como padrão?
    - (a) sim **[recomendado]**; (b) outro valor.

---

## 10. Resumo e decisões pendentes

**Resumo**
- Três problemas-âncora sintéticos e reprodutíveis: A1 (seção transversal com duas camadas de solo e ar opcional), A2 (planta com meandro e ilha), A3 (bloco 3D com canal de profundidade variável).
- Juntos cobrem: não convexidade, buraco, múltiplos componentes, duas regiões do mesmo meio, arestas vivas, junções triplas e forte variação de densidade.
- Nenhum deles tem oráculo na VMMLib; a verificação é por invariantes. O benchmark B1 é o único comparável com a VMMLib.
- Regiões: precedência na API, partição interna e validador.
- Formatos: 0.1 com VTK XML, OpenFOAM e formato nativo; 0.2 com MODFLOW 6, PFLOTRAN e TOUGH; CGNS opcional; Gmsh fora.
- Metas propostas, com método de medição; recalibração única no P07.

**Decisões pendentes para o João**
1. Aprovar DEC-017 (problemas-âncora)? Recomendação: aprovar.
2. Aprovar DEC-018 (precedência + partição + validador)? Recomendação: aprovar.
3. Aprovar DEC-019 (formatos por versão)? Recomendação: aprovar, com a resposta da pergunta 7 definindo a ordem dos escritores da 0.2.
4. Aprovar DEC-020 (metas, determinismo, tolerâncias)? Recomendação: aprovar, com recalibração no P07.
5. Aprovar DEC-021 (benchmark B1–B3)? Recomendação: aprovar.
6. Responder ao questionário da §9.