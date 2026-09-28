# VMM — Requisitos iniciais

* **Origem:** discussão com o João em 28/09/2026, antes do P01, atualizada com as decisões tomadas durante P01–P03 e discussões subsequentes.

* **Papel deste arquivo:** registrar os requisitos declarados pelo João e manter uma visão consolidada do escopo do projeto. As decisões formais ficam em `planning/DECISIONS.md`. Quando um requisito já estiver coberto por uma decisão formal, a entrada DEC correspondente deve ser citada.

* **Repositório:** `https://github.com/voronoimeshmaker/VoronoiMeshMaker`
  Branch de referência na discussão inicial: `main`, commit `7402178`.

* **Estado inicial do repositório:**

  * `VMMLib/` contém a implementação funcional existente, com aproximadamente 13 mil linhas de C++;
  * `VoronoiGridMaker/` contém uma tentativa anterior de reorganização arquitetônica, composta predominantemente por placeholders e comentários, além de `docs/architecture/project_guidelines.tex`;
  * `VoronoiGridMaker/` deve ser tratado apenas como material histórico e fonte eventual de ideias. **Não constitui a base obrigatória da nova arquitetura.**

---

## 1. Objetivo

Concluir o **VoronoiMeshMaker (VMM)** como uma biblioteca pública de geração e representação de malhas de Voronoi, destinada a usuários e aplicações além dos programas desenvolvidos pelo João.

A biblioteca deverá fornecer:

* suporte 2D completo;
* uma **versão 3D real**, baseada em células poliédricas de Voronoi, e não em simples extrusão de uma malha 2D;
* domínio multirregião;
* interfaces conformes;
* topologia, geometria e métricas suficientes para uso direto por métodos de volumes finitos.

### Produto principal

O VMM recebe:

1. um domínio;
2. a definição de suas regiões e fronteiras;
3. um conjunto de sítios geradores;

e produz um **modelo de malha pronto para ser consumido por discretizações de volumes finitos** (DEC-001).

Esse modelo deve fornecer, no mínimo:

* células/volumes;
* faces;
* `owner` e `neighbour` de cada face;
* centros geométricos;
* vetores de área;
* áreas ou medidas das faces;
* volumes ou áreas das células;
* distâncias entre centros adjacentes \(d_{ij}\);
* medidas de não ortogonalidade;
* conectividade entre entidades;
* adjacência entre volumes;
* rótulos de região;
* rótulos de patch;
* informação suficiente para que aplicações externas construam suas próprias estruturas algébricas.

O VMM **não é um solver de equações diferenciais** e não deve incorporar álgebra linear ou estruturas específicas de um solver externo.

---

## 2. Requisitos

| #   | Requisito                                                                                                                                                                                                                                                                                                                                                                |
| --- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| R1  | Cada classe não trivial deverá possuir seu próprio GTest. Sempre que uma classe for modificada, seu teste correspondente deverá ser atualizado. O significado preciso de “não trivial” e os critérios quantitativos de cobertura serão definidos antes da implementação da nova arquitetura.                                                                             |
| R2  | `tests/` deverá espelhar a organização da biblioteca, usando arquivos `ut_<Classe>.cpp` e mecanismo automático de descoberta dos testes.                                                                                                                                                                                                                                 |
| R3  | Não utilizar funções virtuais nem hierarquias de herança na arquitetura do VMM. Polimorfismo deverá ser estático quando necessário, utilizando templates, concepts, traits, policies, composição e factories/registros abertos.                                                                                                                                          |
| R4  | A arquitetura deverá seguir princípios de DOD, DDD, SOLID adaptado ao polimorfismo estático e SoC. O trecho “SOLID clássico não será adotado” de `project_guidelines.tex` deverá ser revisto para refletir essa diretriz.                                                                                                                                                |
| R5  | O domínio possui \(N \geq 1\) regiões, com fronteiras fixas e definidas. Existem dois níveis distintos: **meio**, por exemplo água, sólido ou gás, definido por mecanismo extensível; e **região**, que é uma instância espacial desse meio. Um mesmo meio pode possuir de zero a várias regiões.                                                                        |
| R6  | Interfaces entre regiões devem ser conformes e identificadas pelo par de regiões que se encontram naquela interface, inclusive quando as duas regiões pertencem ao mesmo meio.                                                                                                                                                                                           |
| R7  | A malha é estática. Fronteiras móveis, adaptação dinâmica e remalhamento durante a solução não fazem parte do escopo atual.                                                                                                                                                                                                                                              |
| R8  | O VMM deve ser completamente independente do PETSc. Nenhum header, tipo, objeto ou chamada da API PETSc deverá aparecer no núcleo, backend geométrico ou interface pública do VMM.                                                                                                                                                                                       |
| R9  | O VMM deverá expor conectividade e adjacência da malha usando tipos próprios ou tipos padrão do C++. Essas informações deverão ser suficientes para que um programa consumidor construa, por exemplo, matrizes esparsas, grafos, pré-alocação, reordenação ou particionamento em PETSc ou em qualquer outra biblioteca externa, sem reconstruir geometricamente a malha. |
| R10 | O VMM possuirá seu próprio subsistema de tratamento de erros e exceções. Os tipos de erro e exceção serão próprios do VMM e não utilizarão herança nem funções virtuais. Implementações já existentes em outros projetos poderão servir como referência conceitual, mas não constituem automaticamente a implementação do VMM.                                           |
| R11 | Falhas previsíveis e recuperáveis poderão ser representadas por mecanismos como `std::expected<T,E>`. Exceções próprias do VMM serão reservadas para condições que a arquitetura definir como excepcionais. A política detalhada será estabelecida na arquitetura do subsistema de erros.                                                                                |
| R12 | O padrão oficial da biblioteca será **C++23**. Recursos de C++23 deverão ser utilizados quando trouxerem benefício concreto à arquitetura, com destaque para `std::expected`. A adoção de um recurso específico dependerá também de sua disponibilidade adequada nos compiladores suportados.                                                                            |
| R13 | O nome do projeto e da biblioteca permanece **VoronoiMeshMaker**. O termo `VoronoiGridMaker` não deve ser utilizado como novo nome do projeto; refere-se apenas a uma estrutura histórica existente no repositório.                                                                                                                                                      |

---

## 3. Aplicação-alvo

As aplicações de referência são problemas ambientais envolvendo diferentes meios e regiões, incluindo:

* rios;
* mares;
* solo;
* ar;
* configurações do tipo “terra + rio”;
* domínios com várias regiões do mesmo ou de diferentes meios.

Essas aplicações servem para orientar requisitos geométricos e topológicos, mas o VMM deverá permanecer uma biblioteca de malhas de propósito geral dentro de seu escopo.

### Fora do escopo atual

Por enquanto, não são requisitos:

* escoamento com superfície livre móvel;
* fronteiras móveis;
* remalhamento dinâmico;
* modelagem explícita de meios porosos em escala de poros.

### Domínios 3D

O VMM deverá futuramente aceitar:

* primitivas geométricas analíticas;
* geometrias arbitrárias importadas;
* superfícies trianguladas, como STL;
* geometrias provenientes de fluxos CAD, quando tecnicamente viável.

Recursos do CGAL, incluindo PMP, Mesh_3, AABB trees e triangulações 3D, poderão ser utilizados pelo backend geométrico, respeitando DEC-002, DEC-006 e DEC-007.

A arquitetura detalhada do 3D somente deverá ser consolidada depois da validação experimental prevista para o P15a.

---

## 4. Dependências e interoperabilidade

### 4.1 Backend geométrico

O **CGAL é o backend geométrico do VMM**, encapsulado atrás da abstração interna definida pelas decisões DEC-002, DEC-006 e DEC-007.

Tipos CGAL não deverão vazar para a interface pública normal da biblioteca.

O modelo de dados da malha pertence ao VMM.

### 4.2 Dependências externas

A biblioteca deverá manter o número de dependências externas tão pequeno quanto razoavelmente possível (DEC-004).

A relação de dependências aceitas e opcionais é controlada por DEC-009.

### 4.3 PETSc

**PETSc não é dependência do VMM.**

Programas que utilizam o VMM poderão utilizar PETSc normalmente.

O VMM deverá fornecer uma representação eficiente da topologia e da adjacência da malha, possivelmente por estruturas compactas equivalentes a CSR ou por views adequadas, permitindo que a aplicação consumidora determine diretamente:

* vizinhos de cada volume;
* número de vizinhos;
* `owner` e `neighbour`;
* padrão de conectividade;
* estrutura necessária à pré-alocação de matrizes esparsas;
* grafos necessários à reordenação ou particionamento.

A transformação dessas informações em:

* `Mat`;
* `IS`;
* estruturas de particionamento;
* estruturas de renumeração;
* ou qualquer outro objeto PETSc

é responsabilidade da **aplicação consumidora ou de uma camada de integração externa**, e não do VMM.

Consequentemente, tipos como `Mat`, `IS`, `AO`, `PetscInt` ou equivalentes não devem fazer parte da API do VMM.

### 4.4 Versões das dependências

A política do projeto é acompanhar versões estáveis recentes das dependências, especialmente CGAL.

O projeto não dependerá, entretanto, da expressão abstrata “última versão disponível” para garantir reprodutibilidade.

Deverão ser distinguidos:

1. **versão mínima suportada**, declarada pelo sistema de build;
2. **versões testadas**, mantidas pela infraestrutura de integração contínua;
3. **versões efetivamente utilizadas**, registradas durante builds, testes e execuções relevantes.

Essa política deverá ser consolidada em DEC-012.

---

## 5. Tratamento de erros

O VMM terá um subsistema próprio para representar, transportar e reportar erros.

Esse subsistema deverá ser independente de:

* CGAL;
* PETSc;
* solvers externos;
* aplicações consumidoras.

A arquitetura deverá permitir representar, quando aplicável:

* código estável do erro;
* categoria;
* mensagem;
* origem;
* contexto;
* `std::source_location`;
* entidade geométrica relacionada ao erro, quando existir.

O contexto poderá envolver, por exemplo:

* `SiteId`;
* `CellId`;
* `FaceId`;
* `RegionId`;
* `PatchId`.

A biblioteca deverá distinguir conceitualmente:

* **falhas recuperáveis**, que podem ser devolvidas ao chamador por um tipo de resultado, preferencialmente baseado em `std::expected`;
* **condições excepcionais**, representadas por um tipo próprio de exceção do VMM.

O tipo de exceção do VMM **não deverá derivar de `std::exception`**, em conformidade com R3.

Subsistemas de tratamento de erros desenvolvidos anteriormente poderão servir de referência para nomenclatura, organização, catálogos e fluxo de erros, mas a implementação do VMM deverá ser própria.

---

## 6. Documentação

### 6.1 Referência da API

A documentação deverá seguir como referência conceitual a organização adotada pelo PETSc, com informações como:

* Synopsis;
* Parameters;
* Notes;
* Level;
* See Also;
* Location;
* Examples.

Isso é apenas uma **referência de organização documental** e não implica qualquer dependência do PETSc.

Ainda será decidido no P13 se a unidade principal da documentação será:

* uma página por função;
* uma página por classe;
* ou um modelo híbrido adequado à API C++ do VMM.

### 6.2 Galeria

A documentação deverá possuir galeria baseada em `sphinx-gallery`, com:

* texto explicativo;
* código executável;
* figuras;
* arquivos para download;
* exemplos reproduzíveis.

O estilo de apresentação poderá utilizar como referência galerias científicas consolidadas, incluindo a organização adotada pelo NESTLE.

### 6.3 Tema

O tema utilizado será **PyData**, já empregado em `docs_sphinx/`.

### 6.4 Paleta “Estuário”

#### Site claro

* fundo: `#FBFAF7`;
* superfície: `#F1EEE7`;
* texto: `#1E2528`;
* texto secundário: `#56636A`;
* primária: `#0F6E7A`;
* secundária: `#9A521D`.

#### Site escuro

* fundo: `#10171A`;
* superfície: `#18222A`;
* texto: `#E4E9EA`;
* texto secundário: `#9AA8AD`;
* primária: `#5CC2CC`;
* secundária: `#E39A5C`.

#### Figuras

* água: `#0072B2`;
* sólido: `#A0703A`;
* gás: `#D6E4EC`, com transparência de aproximadamente 40%;
* interfaces: `#D55E00`;
* sítios: `#1E2528`;
* patches: `#009E73`, `#E69F00`, `#CC79A7` e `#56B4E9`;
* colormap sequencial: `cividis`;
* colormap divergente: `RdBu`.

---

## 7. Propostas ainda não decididas

As questões abaixo permanecem propostas e deverão ser decididas nas etapas indicadas.

| Proposta                                                                                                                                                                                                                                  | Etapa prevista   |
| ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------- |
| Tornar dimension-independent apenas as estruturas para as quais a generalização 2D/3D seja natural e não imponha abstrações prematuras; storage, identificadores, views, conectividade e validação deverão ser avaliados individualmente. | P06              |
| Definir regiões por **precedência**, usando composição espacial equivalente a CSG por diferença na ordem declarada, acompanhada de validação de sobreposições e vazios.                                                                   | P04/P06          |
| Usar sítios espelhados como técnica preferencial em determinadas interfaces, mantendo clipping geométrico como garantia de conformidade quando necessário. A estratégia deverá ser validada antes de se tornar policy padrão.             | P06/P10          |
| O VMM armazena geometria, topologia, classificação e rótulos; propriedades constitutivas e físicas pertencem às aplicações consumidoras.                                                                                                  | P06              |
| Utilizar factories e polimorfismo estático como mecanismos principais de configuração; eventual registro em runtime deverá aceitar callables sem introduzir hierarquias virtuais.                                                         | P06              |
| Avaliar suporte futuro a pesos para diagramas de potência, incluindo a possibilidade de um sítio não possuir célula.                                                                                                                      | P06              |
| Periodicidade permanece de baixa prioridade. Caso implementada, avaliar armazenamento de offset periódico nas faces e limitações das triangulações periódicas do backend geométrico.                                                      | P06 ou posterior |
| Build e execução automática dos exemplos, geração de figuras via PyVista e publicação com GitHub Actions → GitHub Pages.                                                                                                                  | P05/P13          |
| Internacionalização da documentação em português e inglês usando `sphinx-intl`.                                                                                                                                                           | P13              |

---

## 8. Questões e decisões da discussão inicial

| Questão                                                                                                                 | Situação atual                                                                                                                                                                                                                                 |
| ----------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Licença dos componentes dependentes do CGAL                                                                             | **Decidida.** Separação entre `vmm_core`, sem dependência direta do CGAL, e `vmm_backend_cgal`, sujeito às obrigações correspondentes às dependências utilizadas (DEC-008).                                                                    |
| Base de código                                                                                                          | **Decidida.** Será criada uma estrutura nova para o VoronoiMeshMaker, com migração seletiva de componentes aproveitáveis da `VMMLib`. A `VMMLib` será utilizada como oráculo de regressão para comportamentos previamente validados (DEC-011). |
| Uso de `VoronoiGridMaker/` como nova arquitetura                                                                        | **Descartado como requisito.** Essa árvore permanece como referência histórica e poderá fornecer ideias ou documentação aproveitável, mas a nova arquitetura não será obrigada a reproduzi-la.                                                 |
| Nome do projeto                                                                                                         | **Decidido:** `VoronoiMeshMaker`.                                                                                                                                                                                                              |
| Namespace público                                                                                                       | **Em aberto.** `vmm` é o candidato atual, mas ainda deve ser formalizado.                                                                                                                                                                      |
| Padrão C++                                                                                                              | **Decidido:** C++23.                                                                                                                                                                                                                           |
| Tratamento de erros                                                                                                     | **Decidido em princípio:** sistema próprio do VMM, sem herança e sem funções virtuais. `std::expected` poderá representar falhas recuperáveis; exceção própria será usada quando apropriado. A API detalhada ainda será projetada.             |
| Dependência do PETSc                                                                                                    | **Decidido:** o VMM não depende de PETSc. Aplicações consumidoras podem utilizar PETSc.                                                                                                                                                        |
| Informação para matrizes PETSc                                                                                          | **Decidido:** o VMM fornece topologia e adjacência próprias; a aplicação consumidora converte essas informações para estruturas PETSc.                                                                                                         |
| Referência da API: uma página por função ou uma por classe                                                              | **Em aberto.** Será decidido no P13.                                                                                                                                                                                                           |
| Critério de GTest “completo”                                                                                            | **Em aberto.** É necessário definir tratamento de classes triviais e critérios mínimos de cobertura.                                                                                                                                           |
| Violações atuais de R3 na `VMMLib`, incluindo interfaces virtuais, hierarquias de exceções e formas de despacho fechado | **Diagnosticadas no P03.** Não serão reproduzidas automaticamente na nova arquitetura; serão tratadas durante a migração dos respectivos componentes.                                                                                          |
| Arquitetura definitiva do 3D                                                                                            | **Em aberto até o P15a.** A arquitetura 2D não deve impor prematuramente uma solução 3D ainda não validada experimentalmente.                                                                                                                  |

---

## 9. Princípio de integração com aplicações externas

O VMM deverá fornecer **dados de malha**, e não estruturas específicas de um método numérico ou pacote de álgebra linear.

A separação conceitual é:

```text
VoronoiMeshMaker
    |
    +-- geometria
    +-- topologia
    +-- células
    +-- faces
    +-- owner / neighbour
    +-- adjacência
    +-- regiões / patches
    +-- métricas geométricas
             |
             v
      aplicação consumidora
             |
             +-- volumes finitos
             +-- montagem de matrizes
             +-- PETSc
             +-- outros solvers
             +-- pós-processamento
```

Essa separação é requisito arquitetônico do projeto.
