Início rápido
=============

Requisitos
----------

* compilador C++23 (GCC 14 ou Clang 18, no mínimo; DEC-022);
* CMake 3.28;
* CGAL 6.2 (headers) e Boost 1.83; GMP e MPFR recomendados;
* GoogleTest, só para os testes.

Compilar e instalar
-------------------

.. code-block:: bash

   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ctest --test-dir build -L vmm
   cmake --install build --prefix $HOME/.local

Usar num projeto CMake
----------------------

.. code-block:: cmake

   find_package(VoronoiMeshMaker 0.1 REQUIRED)
   target_link_libraries(meu_solver PRIVATE VoronoiMeshMaker::vmm VoronoiMeshMaker::vmm_io)

Primeira malha
--------------

.. code-block:: cpp

   #include <vmm/io/vtu.hpp>
   #include <vmm/vmm.hpp>

   int main() {
       vmm::MeshRequest2D request;
       auto& d = request.declaration;
       const auto rock = *d.media().add("rock");
       const auto outer = *d.add_region("outer", rock, vmm::Rectangle({0, 0}, {1, 1}));
       const auto inner = *d.add_region("inner", rock, vmm::Rectangle({0, 0}, {0.7, 0.7}));
       request.sources = {vmm::sites_for(outer, vmm::UniformRandomSource(0.05)),
                          vmm::sites_for(inner, vmm::UniformRandomSource(0.03))};
       const auto result = vmm::generate_mesh_2d(request);
       if (!result) return 1;                      // result.error().message() explica o motivo
       return vmm::write_vtu(result->mesh, "malha.vtu") ? 0 : 1;
   }

As regiões são declaradas por precedência: ``inner`` pinta por cima de ``outer``.
A malha resultante tem interfaces conformes entre as duas regiões e passa por
todos os invariantes da DEC-011 antes de ser devolvida.
