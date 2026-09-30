Erros
=====

Falhas previsíveis voltam como ``vmm::Result<T>`` (``std::expected``), com um
``vmm::Error`` que traz código estável, categoria, contexto, a entidade
envolvida (sítio, célula, face, região ou patch) e ``std::source_location``.
``message()`` monta o texto em português ou inglês (``set_language``).
``vmm::Exception`` (sem herança de ``std::exception``) só é lançada quando um
invariante interno é violado, o que indica um defeito (DEC-016).
