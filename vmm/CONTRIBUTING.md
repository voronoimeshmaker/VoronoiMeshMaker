# Contribuindo

- Regras vigentes: `vmm/docs/guidelines.rst` (página "Diretrizes do projeto") e `planning/P05_BASELINE.md`.
- C++23; sem `virtual`, herança ou enum de despacho (R3, DEC-024); extensão por
  concepts, policies, composição e registros abertos.
- Includes: biblioteca padrão, bibliotecas externas, VMM; ordem alfabética em
  cada grupo, com os comentários separadores do projeto.
- Toda classe não trivial tem `vmm/tests/<módulo>/<Classe>/ut_<Classe>.cpp`
  (R1, R2); classes triviais listadas em `vmm/tools/ci/trivial_classes.txt`.
- Cabeçalho SPDX em todo arquivo (BSD-3-Clause; GPL-3.0-or-later só no
  backend CGAL).
- Antes de abrir um PR:
  `python3 vmm/tools/ci/check_requirements.py --root vmm`,
  `ctest -L vmm` e, com `-DVMM_COVERAGE=ON`, o alvo `vmm_coverage`
  (90 % das linhas e 80 % dos ramos por arquivo, R25).
- Exemplos da galeria só entram depois que os testes das classes usadas passam
  (R30).

## Fluxo de revisão e publicação

Trabalhe em uma branch e abra um pull request para `main`. Aguarde a revisão
e todos os checks obrigatórios do workflow `vmm-ci` antes de integrar.
O administrador configura em Settings → Branches a proteção de `main`:
Require a pull request before merging, Require status checks to pass before
merging (selecionar os checks de `vmm-ci`) e, se disponível, impedir bypass.
Não habilite force-push nem exclusão da branch protegida.

O workflow `vmm-docs` gera artefatos para revisão em `main` e em pull requests.
Somente push de tag `v*` publica no Pages. A versão `1.1.0-dev` tem banner de
desenvolvimento; remover o sufixo exige o fechamento e a aprovação da versão.
Tags e publicação são responsabilidade do mantenedor.
