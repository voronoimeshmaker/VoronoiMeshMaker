# Licenças do repositório

| Parte | Licença | Arquivo |
|---|---|---|
| `vmm/` núcleo, IO, headers públicos, testes, exemplos, ferramentas (arquivos com `SPDX-License-Identifier: BSD-3-Clause`) | BSD-3-Clause | `LICENSE` |
| `vmm/src/backend/cgal/`, `vmm/tools/golden/` e a VMMLib legada (arquivos com `SPDX-License-Identifier: GPL-3.0-or-later`) | GNU GPL v3 ou posterior | `COPYING` |

Binários que ligam `vmm_backend_cgal` (inclusive a fachada `vmm`) ficam sujeitos à GPL, porque usam pacotes do
CGAL licenciados sob a GPL (DEC-008). O núcleo `vmm_core` e o `vmm_io` não dependem do CGAL.
