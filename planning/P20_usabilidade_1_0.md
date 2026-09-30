# P20 — Facilidade de uso e API 1.0

- **Data:** 2026-09-30
- **Ambiente:** WSL `Ubuntu-26.04-Test`; g++ 15.2.0 e 14.3; CMake 4.2.3; CGAL 6.2.1; Boost 1.92.
- **Estado:** pronto para o commit do João; a 1.0.0 sai com o commit, a tag e o push, que são feitos por ele.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Pedido

"Pode fazer a facilidade de uso e o 3. A paralelização ficará para depois." A facilidade de uso inclui o ajudante que
troca `Result` por exceção e o executável com arquivo de configuração. Bindings em Python ficaram fora: exigem
dependência nova e uma DEC própria. O "3" é o congelamento da API 1.0.

## 2. CI quebrada: causa encontrada

**[F]** Desde o 3D, todos os jobs da CI falhavam no passo Build. Localmente, o GCC 14 deu um único erro:
`ut_Voronoi3D.cpp:184`, `-Werror=dangling-reference`.

O teste percorria `run(...).build.mesh.sites()` num `for` por intervalo. O GCC 15 prolonga a vida do temporário
(P2718, C++23), mas o GCC 14 não: nele o código lia memória liberada. A correção guarda o resultado numa variável.

Com isso, e com `CMAKE_CXX_SCAN_FOR_MODULES OFF`, o build com GCC 14 fica limpo e os 264 testes rápidos passam.

**[I]** O job do Clang 18 não pôde ser reproduzido aqui (não há Clang instalado). Os passos Build da CI agora publicam
as primeiras linhas de erro como anotações públicas; se o Clang ainda falhar, elas dizem por quê.

## 3. Facilidade de uso (DEC-040)

**`vmm::value_or_throw`** (`vmm/error/exception.hpp`)
- Recebe um `Result` e devolve o valor ou lança `vmm::Exception` com o mesmo `Error`.
- Um lvalue dá uma referência ao valor; um rvalue o move, sem copiar a malha.
- Há também uma sobrecarga para `Status`.
- A biblioteca continua a lançar exceções só por invariantes internos (DEC-016).

**Módulo `app`**, no alvo `VoronoiMeshMaker::vmm`:
- `MeshConfig`: formato `chave = valor`, com seções `[region nome]`, `[hole]` e `[background nome]`. Os erros dizem
  a linha e a seção.
- `SiteSourceRegistry2D/3D`: registros abertos de fontes de sítios por nome (`uniform`, `count`, `grid`,
  `hexagonal`, `explicit`).
- `ConfigRegistries`: reúne as formas e as fontes, e acrescenta as formas 3D `stl` e `extrusion`.
- `make_request_2d/3d` e `run_config`: um só template com uma struct de traços por dimensão, sem despacho fechado.

**`vmm-mesh`**
- Executável fino sobre `run_config`, instalado em `bin/`.
- Opções: `--output`, `--language pt|en`, `--version` e `--help`.

## 4. API 1.0 (DEC-041)

- **Versão:** 1.0.0 com versionamento semântico. O pacote é `SameMajorVersion`. `vmm/core/version.hpp` expõe a
  versão, e um teste a mantém igual à do CMake.
- **Estável:** os headers públicos no namespace `vmm`, os códigos de erro, o `.vmesh` versão 1 e as chaves dos
  arquivos de configuração.
- **Interno:** os tipos e membros dos headers de backend, exceto `Backend2D`/`Backend3D` como tipo, `build_partition`
  e `cgal_backend_2d/3d`; o namespace `vmm::detail`; os contadores de `BuildStats2D/3D`.
- **Documentação:** a referência da API tem uma seção "Estabilidade". Os headers de backend e `BuildStats` dizem que
  são internos.

## 5. Testes e documentação

**GTest por classe:**
- `ConfigSection`, `MeshConfig`, `SiteSourceRegistry2D`, `SiteSourceRegistry3D`, `ConfigRegistries`;
- `ValueOrThrow`, no arquivo de `Exception`;
- `Version`;
- `ConfigEntry` e `ConfigRunReport` estão na lista de classes triviais.

**Integração:** `integration/ConfigRun`.
- O pedido montado do arquivo dá a mesma malha, bit a bit, que o escrito com a API.
- Erros com linha e seção; fundo 2D; gravação e releitura dos arquivos; registros de usuário.

**Executável (CTest):**
- os dois exemplos `.cfg` do manual, gravando na árvore de build;
- `--version`, e os casos sem arquivo, com argumento inválido e com arquivo inexistente.

**Manual:**
- página "Arquivos de configuração";
- seção sobre `value_or_throw` no guia de erros;
- "Sem escrever C++" no início rápido;
- galeria com `value_or_throw`, `soil2d` e `layers3d` (os `.cfg` são executados pelo `vmm-mesh` e desenhados);
- tudo em português e inglês.

## 6. Verificação

**[F] No estado final:**
- g++ 15, Debug (build dev): 289/289 testes, inclusive os lentos.
- Build de cobertura: 289/289; 97,6% das linhas e 91,8% dos ramos. Nenhum arquivo abaixo do mínimo; `src/app`
  tem de 87% a 99% dos ramos.
- Verificação estática (`check_requirements`) sem problemas; o módulo `app` está no grafo de dependências.
- Documentação em português e inglês, com o portão de 289/289. A galeria executou `value_or_throw` e os dois
  `.cfg`. Os 32 avisos do breathe são os mesmos das builds anteriores.
- Pacote instalado (Release sem LTO nem `-march=native`):
  - `find_package(VoronoiMeshMaker 1.0)` num projeto externo compilou, gerou a malha 3D e rodou `run_config` com
    `value_or_throw`;
  - `bin/vmm-mesh --version` imprime 1.0.0.
- g++ 14.3 (o compilador da CI), Debug: build limpo com `-Werror`; 289/289 testes.
- Clang 18: não verificado localmente; só a CI mostra.

## 7. Para publicar (João)

1. Commit.
2. `git tag -a v1.0.0 -m "VoronoiMeshMaker 1.0.0"`.
3. `git push` e `git push origin v1.0.0`.
4. Conferir se a CI passa, inclusive o Clang 18, e se o Pages foi publicado.
