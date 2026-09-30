# P13 — Documentação da entrega (a)

- **Data:** 2026-09-29
- **Ambiente:** o do `planning/P07_infra.md`. Documentação num ambiente virtual isolado em `~/.cache/vmm-agent-build/docs-venv`, com Sphinx 8, pydata-sphinx-theme 0.22, Breathe, MyST, sphinx-design, sphinx-intl, matplotlib, pyvista; Doxygen do sistema.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Entregas

| Tarefa | Entrega |
|---|---|
| 1. Sphinx + PyData + Breathe/Doxygen | site em `vmm/docs/` (separado de `docs_sphinx/`, que descreve a VMMLib e tem alterações suas não commitadas) |
| 2. Referência da API | `api.rst` com `doxygennamespace:: vmm` a partir dos headers públicos |
| 3. Galeria | extensão `vmm/docs/_ext/vmm_gallery.py`: roda cada `vmm/examples/<nome>/ex_<nome>.cpp` compilado, lê o cabeçalho (Title/Description) e gera página com texto, figura, saída, código e download. **Se um exemplo falhar, o build falha** |
| 4. Paleta Estuário | `_static/estuario.css` (claro e escuro); figuras pintadas por meio (água #0072B2, sólido #A0703A, gás #D6E4EC a 40 %) e interfaces em #D55E00 |
| 5. i18n e guia do usuário | páginas em português (início rápido e guia com domínio, sítios, malha e iteradores, arquivos, erros); `sphinx-intl` instalado, `locale_dirs` configurado |
| 6. Workflow de publicação | não criado (ver §3) |
| 7. Gate do AGENTS.md (R30) | `build_docs.sh` roda `ctest -L vmm` antes da galeria e para se algum teste falhar |
| 8. Diretrizes | `planning/project_guidelines.tex` **não** foi movido (ver §3) |

**Exemplos da galeria:** `quickstart` (duas regiões e um buraco), `anchor_a1` e `anchor_a2`.

## 2. Evidência

**[F]**
- `vmm/docs/build_docs.sh ~/.cache/vmm-agent-build/dev ~/.cache/vmm-agent-build/docs-venv`: gate de **153/153** testes passando, Doxygen, Sphinx e galeria com 3 páginas e 3 figuras; HTML em `~/.cache/vmm-agent-build/dev/vmm_docs/html`.
- **Avisos:** 16 são do Breathe, que não interpreta declarações `template` com concepts do C++20 (limitação conhecida; as páginas saem mesmo assim).
- **Build fora da árvore:** o Sphinx roda sobre uma cópia das fontes, e nada é gerado na árvore (R24).
- **Figuras:** A1 e *quick start* conferidas visualmente (meios, interface conforme em "L", buraco).

## 3. Desvios e pendências

1. **Figuras com matplotlib em vez de PyVista.** O pyvista foi instalado, mas a renderização *offscreen* do VTK no WSL exige OSMesa/EGL e é frágil. A extensão lê o `.vtu` diretamente e desenha com matplotlib na paleta pedida. **[R]** Trocar por PyVista na CI, se o runner tiver EGL.
2. **Inglês:** catálogo `vmm/docs/locale/en/LC_MESSAGES/docs.po` (48 mensagens); `build_docs.sh` gera `html/` (pt) e `html/en/` (en).
3. **Publicação:** `.github/workflows/vmm-docs.yml` compila, roda o gate, gera os dois idiomas e publica no GitHub Pages. Precisa de *Settings > Pages > Source: GitHub Actions* no repositório; ainda não rodou (sem push).
4. **`project_guidelines.tex` não foi movido.** A DEC-026 manda movê-lo no P13, mas isso é uma iteração só de movimentação, que convém fazer junto com a retirada da VMMLib (P06 §11–12).
5. **Seções PETSc:** as 9 funções principais (`generate_mesh_2d`, `build_mesh_2d`, `generate_sites`, `check_invariants`, `compute_metrics`, `write_native`, `write_vtu`, `renumber`, `sparse_pattern`) têm Parameters, Notes, Level, See Also, Location e Examples (Synopsis é a própria declaração). O restante da API tem descrição breve.

## 4. Cobertura (R25), medida para toda a biblioteca

**[F]** Build Debug com `-DVMM_COVERAGE=ON`; 153 testes; gcovr 7.2 via `vmm_coverage`:

| Métrica | Valor |
|---|---|
| Linhas | **97,4 %** |
| Ramos | **91,9 %** |
| Funções | **90,3 %** |
| Arquivos abaixo de 90 % / 80 % | **0**, com 1 exceção declarada (`vmm/tools/ci/coverage_exceptions.txt`) |

A exceção é `vmm/src/facade/facade.cpp`: 100 % das linhas e 62,9 % dos ramos. Os ramos que faltam são falhas de construção e de invariante, que não dá para provocar pela API pública com o backend CGAL; os dois caminhos são testados diretamente no construtor e no verificador.

**[I]** Como no P03, a cobertura de templates só conta o que foi instanciado nos testes.
