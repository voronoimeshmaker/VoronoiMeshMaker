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
