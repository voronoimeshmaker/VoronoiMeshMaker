# P09 — Domínio 2D multirregião

- **Data:** 2026-09-29
- **Ambiente:** o do `planning/P07_infra.md`.
- **Convenção:** **[F]** fato verificado · **[I]** inferência · **[R]** recomendação.

## 1. Entregas

- **Formas** (`domain/shapes.hpp`):
  - `Rectangle`, `PolygonShape` (com buracos), `Circle`, `Ellipse`, `RegularNGon`;
  - concept `Shape2D`;
  - `ShapeOutline` com rótulo por aresta, reorientado junto com os rótulos;
  - `ShapeRegistry` aberto (nome → fábrica).
- **Meios e regiões** (`domain/declaration.hpp`): `MediumRegistry` aberto e `Declaration2D` por precedência, com camadas de região ou buraco e região de fundo opcional (DEC-018).
- **Partição** (`domain/partition.hpp`): `Partition2D` com vértices, segmentos compartilhados (esquerda/direita, patch) e laços por componente.
- **Construção exata** (`src/backend/cgal/partition.cpp`, GPL):
  - `Arrangement_2` com Epeck e rótulo de registro por aresta;
  - cobertura das faces por paridade de cruzamento a partir da face ilimitada, sem nenhum teste ponto-em-polígono aproximado;
  - patch = rótulo da forma de maior precedência que cobre a aresta.
- **Validador** (`domain/validator.hpp`):
  - erros: vazios (salvo com região de fundo), região anulada, laço aberto ou mal orientado;
  - avisos: região fragmentada, lasca (erro se configurado).

## 2. Testes

**[F]** 36 casos: 27 do módulo `domain`, 9 do backend. Destaques:

| Caso | Resultado |
|---|---|
| A1 (P04 §2.1) | áreas 150 / 1450 / 1400 m² e interfaces 200 m e 20 + 2√125 m exatas até 10⁻¹²; patches `superficie_agua` 40, `terreno` 160, `base` 200, laterais 15 |
| Quadrado com buraco e interface em "L" | áreas 0,45 / 0,51, interface 1,4, patches por lado |
| Forma com buraco próprio | o buraco é intencional, não vazio |
| Vazio cercado por quatro barras | erro `DomainVoid`; com região de fundo, a área 1 vai para o fundo |
| Região anulada | erro `RegionEmptied` |
| Planície cortada por um rio | 2 componentes, aviso `RegionFragmented` |
| Lasca de 10⁻⁷ | aviso `Sliver`; erro no modo estrito |
| Propriedade | 40 declarações aleatórias: soma das regiões = área total; soma das interfaces por par = total |
| Determinismo | duas construções dão vértices e segmentos idênticos |

## 3. Achado

**[F]** Um `PolygonShape` com anel interno deixa a área do buraco coberta por nenhuma camada. O construtor passou a tratar essa área como buraco intencional quando está dentro do anel de buraco da própria forma, e como vazio (erro) nos demais casos.
