# P28 — documentação executada

2026-10-04. Guia Sphinx pt/en atualizado: representação triangular fixa,
colunas, regiões e subdivisões, consultas, persistência, limites de precisão e
contrato de interfaces/CVT. Comentários Doxygen ampliados nas APIs novas.
README e changelog identificam a versão 1.1 preparada, não publicada.

Exemplos depois dos testes:
- horizons/ex_horizons.cpp: faixas planas e terreno facetado, quatro subdivisões
  em duas regiões, gravação vmesh/vtu/vlayers.
- config/horizons_flat.cfg e horizons_flat.hgrid: equivalente ao caso plano.
- Comparação cmp dos .vlayers C++/CLI retornou zero.
- Galeria copia e oferece download do arquivo .hgrid acompanhante.
- Imagem terrain.png inspecionada: cores por região, faces e encontro na borda.

O primeiro build completo falhou por ausência de pydata_sphinx_theme.
Criado build/horizons-docs-venv com dependências, sem alterar Python do sistema.
Comando reproduzível:
```sh
bash vmm/docs/build_docs.sh build build/horizons-docs-venv
```
Esse comando executa 310 testes antes de Doxygen, galeria e Sphinx pt/en.

A construção completa passou com 16 avisos por idioma: Sphinx/Breathe interpreta
declarações extern template (write_native, read_native, métricas e invariantes)
como declarações template incompletas. O log contém os símbolos afetados;
não foram suprimidos os avisos nem se declara documentação sem avisos.
Guia, exemplos e imagens foram gerados em ambos os idiomas.
Resultado: build/vmm_docs/html/index.html e html/en/index.html.
Log final: build/horizons_docs_full.log.
