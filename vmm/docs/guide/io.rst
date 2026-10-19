Arquivos
========

* **Formato nativo** (``write_native`` / ``read_native``): texto versionado, com
  ida e volta exata (17 algarismos significativos). É o formato de persistência
  e o dos golden files.
* **VTK XML** (``write_vtu``): visualização no ParaView (polígonos em 2D,
  poliedros em 3D), com região, meio, sítio de entrada, área ou volume, razão de
  aspecto e não ortogonalidade por célula.
* **STL** (``read_stl``, ``read_stl_surface``, ``write_stl``): superfícies de
  domínio 3D, ASCII (um patch por ``solid``) ou binário.

Não há escritor OpenFOAM (DEC-030).

Entradas inválidas e falta de recursos
-------------------------------------------

Os leitores devolvem ``Result`` também para falhas de leitura e exceções padrão
ocorridas dentro da chamada. ``ResourceExhausted`` (VMM-504) distingue falta de
memória ou limites de contêiner de ``ParseError``. Contagens de arquivos nativos
são limitadas pelos IDs e, em fluxos pesquisáveis, pelos bytes restantes. Fluxos
sem busca são consumidos incrementalmente; a contagem não determina uma alocação
imediata. Dados adicionais após o término, valores não finitos e IDs inválidos
são rejeitados. STL exige facetas completas; o formato nativo continua na versão 1.

Readers return ``Result`` for malformed input and standard exceptions occurring
inside the call. ``ResourceExhausted`` identifies allocation/container failures;
``ParseError`` identifies invalid input. Seekable streams validate counts against
remaining bytes; non-seekable streams grow incrementally. The native format
remains version 1.

Persistência de colunas experimental
------------------------------------

``VMM_LAYERS 2`` acrescenta, antes do bloco nativo, uma linha com as
coordenadas xy dos geradores (contagem de escalares seguida dos valores) e
uma linha com as flags de fallback por célula (0 ou 1). O leitor aceita a
versão 1, deixando esses metadados vazios: vazio significa desconhecido.
O formato nativo interno permanece versão 1. A renumeração preserva as flags.

Experimental ``VMM_LAYERS 2`` stores base generators and per-cell centroid
fallback flags. Version 1 remains readable with unknown (empty) provenance.

``VMM_LAYERS 3`` estende a versão 2 com a quantidade de colunas, uma linha
contada de cotas por coluna e uma linha contada de KFloor. KFloor é o primeiro
índice de camada não vazia; o número de camadas representa coluna vazia.
Os formatos 1 e 2 continuam legíveis. O escritor usa v2 quando não há cotas
constantes por coluna, e v3 quando esses metadados estão presentes.
