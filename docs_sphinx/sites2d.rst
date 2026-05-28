Sites2D
=======

The ``Sites2D`` module defines and generates two-dimensional Voronoi generator
sites. A site is only the generator point and its metadata; it is not a Voronoi
cell yet. The Voronoi cells are built later from these sites.

Design Rules
------------

The public API uses English names for classes, functions, enums, and public
fields. This keeps the library consistent with C++ and CGAL conventions.

The current naming convention is:

* ``Site2D`` for one generator point.
* ``SiteSet`` for an ordered collection of generator sites.
* ``SiteValidationOptions`` for geometric validation rules.
* ``CartesianGrid2D`` for Cartesian generator patterns.
* ``TriangularIIGrid2D`` for the Triangular II pattern from the reference paper.
* ``TriangularIVGrid2D`` for the Triangular IV pattern from the reference paper.
* ``UniformRandom2D`` for reproducible random generator placement.
* ``HexagonalGrid2D`` for hexagonal Voronoi-cell patterns.

The implementation follows value types and overloads instead of inheritance.
New patterns should be added as small configuration structs plus overloads of
``make_sites`` and ``append_sites``.

Generation And Boundary Filtering
---------------------------------

Site patterns are conceptually independent from ``Boundary2D``. A pattern knows
how to generate candidate points in the plane. The boundary-aware factory
overloads use a generation box, usually the boundary bounding box, and then
filter candidates with ``SiteValidationOptions``.

This separation is intentional:

* the same pattern can be reused with different domains;
* boundary clipping remains centralized in validation code;
* minimum distance to the boundary is controlled by one option;
* patterns can later be rotated using the common rotate-generate-rotate-back
  workflow.

The two main workflows are:

.. code-block:: cpp

   // Raw candidates in a geometric box, independent from Boundary2D.
   const auto raw = vmm::s2d::make_raw_sites(box, pattern);

   // Boundary-filtered sites, validated and sequentially numbered.
   const auto sites = vmm::s2d::make_sites(boundary, pattern, options);

Sequential Numbering
--------------------

``SiteSet`` keeps sites in a contiguous vector-like order. Final generated
sets must have sequential identifiers because finite-volume matrix assembly
will use those ids as stable row and column indices.

Factories renumber the accepted sites before returning. Validation can enforce
this rule through ``SiteValidationOptions::require_sequential_ids``.

CartesianGrid2D
---------------

``CartesianGrid2D`` generates candidates from Cartesian cell centroids.
The default is ``epsilon = 0``, which places each generator at the centroid.

For ``theta = 0``, the Cartesian eccentricity formula used by the factory is:

.. math::

   x = x_c + \epsilon \frac{\Delta x}{2}

.. math::

   y = y_c - \epsilon \frac{\Delta y}{2}

with:

.. math::

   -1 < \epsilon < 1

``epsilon`` is exposed by ``with_epsilon``. ``with_eccentricity`` is kept as a
readable alias.

TriangularIIGrid2D
------------------

``TriangularIIGrid2D`` represents the Triangular II pattern from the reference
paper. Each square patch is divided into two triangular control volumes.

The eccentricity parameter is named ``epsilon``. The implementation follows the
legacy generator convention while keeping the public name aligned with the
paper:

.. math::

   a =
   \begin{cases}
   \frac{1}{3} + \frac{\epsilon}{6}, & 0 \le \epsilon < 1 \\
   \frac{1 + \epsilon}{3}, & -1 < \epsilon < 0
   \end{cases}

where ``a * spacing`` is the local offset used by the 2h x 2h repeating block.
With ``epsilon = 0``, the generator is at the triangle centroid. Positive and
negative eccentricities move it to opposite sides of the admissible line.

TriangularIVGrid2D
------------------

``TriangularIVGrid2D`` represents the Triangular IV pattern from the reference
paper. Each square patch is divided into four triangular control volumes by
connecting the square center to the four corners.

The eccentricity parameter is also named ``epsilon``. The implementation uses
the same local offset convention as the legacy generator:

.. math::

   a =
   \begin{cases}
   \frac{1}{3} + \frac{\epsilon}{6}, & 0 \le \epsilon < 1 \\
   \frac{1 + \epsilon}{3}, & -1 < \epsilon < 0
   \end{cases}

With ``epsilon = 0``, each generator is at the centroid of its triangular
control volume. Nonzero ``epsilon`` moves the generator along the local
orientation used by the Triangular IV pattern.

UniformRandom2D
---------------

``UniformRandom2D`` is a reproducible rejection sampler. It draws candidates
inside the boundary bounding box and accepts only candidates that pass site
validation. The seed, target count, maximum number of attempts, and exact-count
requirement are configurable.

HexagonalGrid2D
---------------

``HexagonalGrid2D`` generates sites whose Voronoi cells are hexagonal in the
domain interior. This pattern is different from ``TriangularIIGrid2D`` and
``TriangularIVGrid2D`` and does not use ``epsilon``.

The expected canonical construction is a staggered triangular lattice:

.. math::

   \Delta y = \frac{\sqrt{3}}{2}\Delta x

with alternate rows shifted by ``0.5 * spacing``.

Definition Of Done For New Site Patterns
----------------------------------------

Every new ``Sites2D`` pattern should include:

* an English public type name;
* value-type configuration, without inheritance or virtual functions;
* validation of spacing, counts, seed limits, and eccentricity ranges;
* raw generation independent from ``Boundary2D`` when applicable;
* boundary-filtered ``make_sites`` and ``append_sites`` overloads;
* sequential site ids in the final ``SiteSet``;
* unit tests for raw coordinates, validation failures, boundary filtering, and
  composition with another pattern;
* a VTK example that exports the boundary and generated sites together.
