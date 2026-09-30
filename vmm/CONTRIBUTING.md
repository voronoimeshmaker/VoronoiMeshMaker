# Contribuindo

- Regras vigentes: `planning/project_guidelines.tex` e `planning/P05_BASELINE.md`.
- C++23; sem `virtual`, herança ou enum de despacho (R3, DEC-024); extensão por
  concepts, policies, composição e registros abertos.
- Includes: biblioteca padrão, bibliotecas externas, VMM; ordem alfabética em
  cada grupo, com os comentários separadores do projeto.
- Toda classe não trivial tem `vmm/tests/<módulo>/<Classe>/ut_<Classe>.cpp`
  (R1, R2); classes triviais listadas em `vmm/tools/ci/trivial_classes.txt`.
- Cabeçalho SPDX em todo arquivo (BSD-3-Clause; GPL-3.0-or-later só no
  backend CGAL e no gerador de golden files).
- Antes de abrir um PR:
  `python3 vmm/tools/ci/check_requirements.py --root vmm`,
  `ctest -L vmm` e, com `-DVMM_COVERAGE=ON`, o alvo `vmm_coverage`
  (90 % das linhas e 80 % dos ramos por arquivo, R25).
- Exemplos da galeria só entram depois que os testes das classes usadas passam
  (R30).
