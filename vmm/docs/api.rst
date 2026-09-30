Referência da API
=================

Estabilidade
------------

Desde a 1.0, a API segue o versionamento semântico (DEC-041). Na série 1.x
nada do que está abaixo é removido nem muda de assinatura; funções, tipos e
campos novos podem entrar em versões menores.

* **Estável:** os headers de ``vmm/include/vmm`` no namespace ``vmm``, os
  códigos de erro, o formato ``.vmesh`` versão 1 e as chaves dos arquivos de
  configuração.
* **Interno, pode mudar em qualquer versão:** tudo o que
  ``vmm/backend/backend2d.hpp`` e ``backend3d.hpp`` declaram, exceto os nomes
  ``Backend2D`` e ``Backend3D`` e o membro ``build_partition`` (obter o
  backend com ``cgal_backend_2d()``/``cgal_backend_3d()``, chamar
  ``build_partition`` e passá-lo a ``build_mesh_2d``/``build_mesh_3d`` é
  estável); o namespace ``vmm::detail``; os contadores de
  ``BuildStats2D``/``BuildStats3D``.
* **Sem garantia:** compatibilidade binária (recompile ao atualizar) e malhas
  idênticas bit a bit entre versões menores; os invariantes da DEC-011 valem
  sempre.

Símbolos
--------

Seções por símbolo: Synopsis (declaração), Parameters, Notes, Level, See Also,
Location e Examples, extraídas dos comentários Doxygen dos headers públicos em
``vmm/include/vmm``.

.. doxygennamespace:: vmm
   :members:
   :undoc-members:
