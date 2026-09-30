Erros
=====

Falhas previsíveis voltam como ``vmm::Result<T>`` (``std::expected``), com um
``vmm::Error`` que traz código estável, categoria, contexto, a entidade
envolvida (sítio, célula, face, região ou patch) e ``std::source_location``.
``message()`` monta o texto em português ou inglês (``set_language``).
``vmm::Exception`` (sem herança de ``std::exception``) só é lançada quando um
invariante interno é violado, o que indica um defeito (DEC-016).

Exceções no lugar de ``Result``
-------------------------------

Quem prefere exceções a testar cada ``Result`` usa ``vmm::value_or_throw``
(DEC-040): a função devolve o valor ou lança ``vmm::Exception`` com o mesmo
erro. Um ``try``/``catch`` substitui os testes; ``what()`` traz a mensagem em
inglês e ``error().message()`` a do idioma atual. A biblioteca continua a não
lançar exceções por conta própria, exceto para invariantes internos.

.. code-block:: cpp

   #include <vmm/error/exception.hpp>
   #include <vmm/vmm.hpp>

   try {
       vmm::MeshRequest2D request;
       auto& d = request.declaration;
       const auto rock = vmm::value_or_throw(d.media().add("rock"));
       const auto block = vmm::value_or_throw(d.add_region("block", rock, vmm::Rectangle({0, 0}, {1, 1})));
       request.sources = {vmm::sites_for(block, vmm::UniformRandomSource(0.05))};
       const auto result = vmm::value_or_throw(vmm::generate_mesh_2d(request));
   } catch (const vmm::Exception& e) {
       std::println("{}", e.what());
   }

O exemplo :doc:`../gallery/value_or_throw` mostra a mensagem de um erro
capturado.
