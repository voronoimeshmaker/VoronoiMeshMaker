.. SPDX-License-Identifier: BSD-3-Clause

CVT com fronteiras regionais fixas
==================================

O CVT implementado usa Lloyd com densidade geométrica uniforme em 2D ou 3D. A cada iteração calcula centroides, propõe novos sítios e reconstrói a malha contra a mesma partição regional. Nenhuma propriedade física do mohid-ng participa desse cálculo.

API C++
-------

Inclua ``vmm/cvt.hpp``. ``optimize_cvt(partition, sites, options)`` aceita ``Partition2D`` com ``SiteSet`` ou ``Partition3D`` com ``SiteSet3D``. As partições e os rótulos regionais dos sítios permanecem fixos. Os sítios iniciais devem ser válidos para a partição. Pesos de diagramas de potência não são suportados.

``CvtOptions`` define ``max_iterations`` (50), ``max_backtracks`` (24), ``relaxation`` (1) e ``relative_tolerance`` (1e-6). A relaxação deve estar em (0,1]. A tolerância se aplica à maior distância sítio–centroide dividida pela diagonal da partição. Zero iterações reconstrói e avalia a malha inicial.

``CvtResult`` contém a última malha aceita e ``CvtReport``. ``energy`` inclui a energia inicial e as energias aceitas; ``relative_displacement`` contém os deslocamentos máximos aceitos. ``relative_residual`` informa o resíduo sítio–centroide. ``rejected_steps`` e ``last_rejection`` explicam reduções de passo.

``converged`` indica que o resíduo atingiu a tolerância; ``stalled`` indica que não houve passo admissível com redução de energia numericamente resolvida. Se ambos forem falsos, foi atingido o limite de iterações. Sucesso da função significa uma malha válida devolvida, não necessariamente convergência.

Energia e proteção geométrica
-----------------------------

``cvt_energy(mesh)`` integra a distância quadrática aos sítios geradores, com densidade uniforme, por decomposição orientada em triângulos ou tetraedros. A função pressupõe geometria válida e sítios geradores reais; os centroides armazenados numa malha extrudada não tornam essa malha automaticamente Voronoi ou CVT.

Uma proposta inválida, uma reconstrução inválida ou uma energia sem redução resolvida provoca divisão do passo por dois. Centroides fora de regiões não convexas não são aceitos diretamente. A quantidade de sítios é fixa; não há inserção ou remoção automática. Estagnação pode ocorrer e não é apresentada como convergência.

Cada reconstrução usa os mesmos segmentos/triângulos da partição, compatibiliza interfaces e verifica volumes/áreas, fechamento e incidência. Vértices, pontos médios e centros das faces regionais são conferidos contra o suporte finito de referência. Planos, retas, limites e junções ficam fixos; a subdivisão das faces pode mudar. A amostragem adicional não substitui a conformidade garantida pela construção do backend.

Horizontes e perda das colunas
------------------------------

``cvt_domain(layered_mesh)`` extrai os contornos e as interfaces entre regiões, mantendo os horizontes regionais. Faces entre camadas da mesma região não são fronteiras obrigatórias. Regiões totalmente ausentes são removidas da partição compacta; ``source_region`` relaciona os novos índices aos originais. Passe essa partição e sítios 3D admissíveis a ``optimize_cvt``.

O resultado é ``Mesh3D`` geral, sem promessa de preservar colunas ou subdivisões internas dos intervalos. Ele pode ser gravado em vmesh/VTU. Não reutilize os metadados de ``LayeredMesh`` para essa topologia. O mapa célula–sítio refere-se à ordem dos sítios passados à otimização.

Configuração e limites
----------------------

No ``vmm-mesh``, ``cvt_iterations`` ativa a otimização; ``cvt_tolerance`` e ``cvt_relaxation`` são opcionais. Funciona em 2D, 3D e com ``horizons``. Neste último caso, novos sítios volumétricos são amostrados de forma reproduzível com ``seed``, mantendo a quantidade de células por região da extrusão. Os centroides das colunas não são reutilizados. A API C++ permite fornecer sítios explícitos à partição extraída.

Com CVT volumétrico, o CLI grava vmesh/VTU e informa energia e estado de convergência; não grava um novo vlayers. Use um nome de saída distinto de uma extrusão anterior para não confundir arquivos antigos. Pares inicialmente espelhados não permanecem necessariamente espelhados após a otimização.

Não há garantia de mínimo global, ortogonalidade em interfaces, número ideal de células ou eliminação de células finas. Superfícies curvas permanecem na aproximação facetada fornecida. O custo inclui reconstrução e validação em cada tentativa; as estruturas geométricas ainda não são reutilizadas incrementalmente. O recurso se aplica às partições e sítios aceitos pelo gerador existente, não a qualquer malha poliedral externa sem referência regional.
