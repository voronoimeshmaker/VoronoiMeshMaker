# P12 — IO e integração com os problemas-âncora

- **Data:** 2026-09-29
- **Ambiente:** o do `planning/P07_infra.md`.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Escritores da 0.1 (DEC-019, DEC-030)

| Formato | Arquivos | Estado **[F]** |
|---|---|---|
| Nativo (`.vmesh`) | `io/native.hpp` | escrita e leitura, versão 1, ida e volta exata (17 algarismos, `from_chars`), erros com número de linha (`ParseError`, `UnsupportedVersion`, `InconsistentData`) |
| VTK XML (`.vtu`) | `io/vtu.hpp` | células `VTK_POLYGON` montadas a partir das faces; campos: região, meio, sítio de entrada, número de laços, área, razão de aspecto, maior não ortogonalidade |
| OpenFOAM | — | fora por decisão do João (DEC-030) |

**Formato nativo:** as seções previstas no P06 §10 foram mantidas, com uma mudança: owner e neighbour vão na própria linha da face (`nv v0 v1 … owner [neighbour]`), para que cada face seja legível isoladamente.

**Separação entre visualização e persistência:** o `.vtu` só serve para visualizar; o formato nativo guarda a malha completa (patches, regiões, meios, sítios e mapeamento de entrada).

## 2. Problemas-âncora

**[F]** `tests/integration/Anchors` e os exemplos da galeria, usando `vmm/tools/anchors/anchors.hpp`:

| Âncora | Células (exemplo) | Invariantes | Verificações específicas |
|---|---|---|---|
| A1 | 1 964 | OK | interface canal/solo_sup = 20 + 2√125 m; solo_sup/solo_inf = 200 m; patches `superficie_agua` 40, `terreno` 160 |
| A1 com ar | — | OK | 4 regiões; água/ar = 40 m, solo/ar = 160 m; **2 junções triplas** |
| A2 | 71 153 | OK | ilha sem contato com a planície; planície em 2 componentes (aviso); interface canal/ilha > 100 m |

Nos três casos houve ida e volta exata pelo formato nativo e gravação `.vtu` bem-sucedida. Não ortogonalidade no A2 (exemplo):
- faces internas: até 5,7·10⁻⁹ rad (planície), 9,6·10⁻¹⁰ (canal), 4,2·10⁻¹⁰ (ilha);
- interfaces: até 74,8°, o esperado para o E1 sem pares espelhados (DEC-028).

## 3. Desempenho (metas do P04)

**[F]** Ver a tabela em `planning/P07_infra.md` §4:
- **Tempo:** 10⁶ células em 10,2 s (Release, 1 thread), contra a meta de 30 s.
- **Memória:** 1,34 KB/célula, contra a meta de 1 KB. **[R]** Recalibrar para 1,5 KB (DEC-020 permite uma recalibração).
- **Crescimento N log N:** não medido em série (só N = 10⁶); fica para o P14.

## 4. Verificação por um solver

Não se aplica, porque não há OpenFOAM (DEC-030). A malha é consumida pelo solver próprio do João via formato nativo e API de adjacência. **[R]** Um exemplo de leitura do `.vmesh` no solver do João seria a verificação de ponta a ponta mais valiosa.
