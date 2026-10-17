Arquivos de configuração
========================

O executável ``vmm-mesh`` gera uma malha 2D ou 3D a partir de um arquivo de
texto, sem escrever C++ (DEC-040). Ele é instalado com a biblioteca, em
``bin/``.

.. code-block:: bash

   vmm-mesh solo.cfg
   vmm-mesh --language en --output resultados/solo solo.cfg

Os arquivos de saída ficam ao lado do arquivo de configuração, com o nome dele
(``solo.vmesh`` e ``solo.vtu``), a menos que a chave ``output`` ou a opção
``--output`` digam outro caminho. Em caso de erro, a mensagem indica a linha e
a seção do arquivo, e o código de saída é 1.

Formato
-------

Uma instrução por linha, ``chave = valor``; ``#`` começa um comentário. As
chaves antes da primeira seção são globais. Cada seção ``[region nome]``
declara uma região; as declaradas depois cobrem as anteriores onde se
sobrepõem, como na API (precedência, DEC-018).

.. code-block:: ini

   dimension = 2
   seed = 7

   [region solo]
   shape = rectangle
   lo = 0 0
   hi = 4 2
   top = superficie
   sites = uniform
   sites.spacing = 0.12

   [region lente]
   medium = areia
   shape = ellipse
   center = 2.6 1
   axes = 0.8 0.35
   sites = uniform
   sites.spacing = 0.06

   [hole]
   shape = circle
   center = 1 1
   radius = 0.15
   tag = poco

**Chaves globais**

=================== ==========================================================
``dimension``       2 ou 3 (obrigatória)
``seed``            semente dos sítios (padrão 0)
``output``          caminho dos arquivos de saída, sem extensão
``formats``         ``vmesh``, ``vtu`` ou os dois (padrão: os dois)
``interface_pairs`` espaçamento dos pares espelhados nas interfaces (opcional)
``tolerance``       tolerância relativa de pontos (padrão 1e-12)
=================== ==========================================================

**Seções**

Para otimização, use ``cvt_iterations`` (inteiro não negativo),
``cvt_tolerance`` (padrão 1e-6) e ``cvt_relaxation`` (padrão 1).
Os dois últimos exigem ``cvt_iterations``. Zero iterações avalia a malha
inicial; veja :doc:`cvt` para convergência, horizontes e formatos de saída.

* ``[region nome]``: ``shape`` e seus parâmetros, ``sites`` e os parâmetros da
  fonte com o prefixo ``sites.``, ``medium`` (padrão: o nome da região;
  regiões com o mesmo meio o compartilham).
* ``[hole]``: ``shape`` e seus parâmetros.
* ``[background nome]`` (só 2D): a região que preenche os vazios; ``sites`` e
  ``medium`` como numa região.

Os valores numéricos são separados por espaços ou vírgulas. Os demais valores
são nomes (patches). Um parâmetro de forma ``file`` é relativo à pasta do
arquivo de configuração.

Formas
------

================= ===============================================================
2D
================= ===============================================================
``rectangle``     ``lo``, ``hi``; patches ``bottom``, ``right``, ``top``, ``left``
``polygon``       ``xy`` (x0 y0 x1 y1 ...); patch ``tag``
``circle``        ``center``, ``radius``; patch ``tag``
``ellipse``       ``center``, ``axes``, ``angle`` (rad, opcional); patch ``tag``
``regular_ngon``  ``center``, ``radius``, ``n``, ``rotation`` (opcional); ``tag``
================= ===============================================================

================= ===============================================================
3D
================= ===============================================================
``cuboid``        ``lo``, ``hi``; patches ``x-``, ``x+``, ``y-``, ``y+``, ``z-``, ``z+``
``sphere``        ``center``, ``radius``; patch ``tag``
``cylinder``      ``base``, ``radius``, ``height``; patches ``side``, ``bottom``, ``top``
``extrusion``     ``xy`` (contorno), ``z`` (base e topo); patches ``tag``, ``bottom``, ``top``
``stl``           ``file``; ``patch`` (nome do patch de um STL binário)
================= ===============================================================

Fontes de sítios
----------------

============= ================================================================
``uniform``   ``spacing``; ``min_distance`` e ``margin`` (frações, opcionais)
``count``     ``count``; ``margin`` (opcional)
``grid``      ``spacing``; ``origin`` e ``margin`` (opcionais)
``hexagonal`` só 2D: ``spacing``; ``origin`` e ``margin`` (opcionais)
``explicit``  ``xy`` (2D) ou ``xyz`` (3D): as coordenadas dos sítios
============= ================================================================

Os exemplos :doc:`../gallery/soil2d` e :doc:`../gallery/layers3d` da galeria
são executados pelo ``vmm-mesh`` a cada build da documentação.

Pelo C++
--------

O mesmo arquivo pode ser lido por um programa: ``MeshConfig::read`` devolve a
configuração, ``make_request_2d`` e ``make_request_3d`` o pedido da fachada, e
``run_config`` faz tudo o que o ``vmm-mesh`` faz. Formas e fontes novas entram
nos registros abertos de ``ConfigRegistries`` e passam a valer nos arquivos:

.. code-block:: cpp

   #include <vmm/app/run.hpp>

   auto registries = vmm::ConfigRegistries::with_builtins();
   (void)registries.shapes_2d.add("quadrado", [](const vmm::ShapeParameters& p,
                                                 const vmm::PolygonizeOptions& o) {
       const auto lado = p.numbers.at("lado")[0];
       return vmm::Rectangle({0, 0}, {lado, lado}).outline(o);
   });
   const auto config = vmm::value_or_throw(vmm::MeshConfig::read("malha.cfg"));
   const auto report = vmm::value_or_throw(vmm::run_config(config, registries));

Células fragmentadas
-------------------

Em domínios não convexos, uma célula pode ter vários pedaços desconexos.
O comportamento padrão mantém esses pedaços juntos (DEC-035). O executável
imprime os avisos em stderr e a contagem no resumo. Para rejeitar o resultado
antes de gravar arquivos, use a chave global ``reject_fragmented = true``;
``false`` mantém o padrão. A falha usa ``FragmentedCells`` (VMM-404), com os IDs
no contexto. ``--quiet`` suprime avisos e o resumo, mas preserva erros e o código
de saída.

Na API C++, ``MeshRequest2D`` e ``MeshRequest3D`` têm ``reject_fragmented``;
os resultados expõem ``fragmented_cells`` com IDs na numeração da malha retornada.
O CVT informa a lista do último iterado aceito. Nas colunas, a indicação é
propagada da fragmentação da base; ela não é uma análise volumétrica independente
após desaparecimento de camadas.

Fragmented cells (English)
~~~~~~~~~~~~~~~~~~~~~~~~~~

Disconnected pieces are kept together by default. Set the global key
``reject_fragmented = true`` to reject fragmented meshes before writing files.
The CLI prints warnings to stderr; ``--quiet`` suppresses warnings and progress,
but keeps fatal errors and the exit status. The C++ request flag is additive,
and the result list uses canonical cell IDs. CVT reports its last accepted
iterate; layered output propagates the base classification.
