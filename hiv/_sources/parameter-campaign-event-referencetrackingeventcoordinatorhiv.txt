====================================
ReferenceTrackingEventCoordinatorHIV
====================================

The **ReferenceTrackingEventCoordinatorHIV** coordinator class define a particular coverage of an individual-
level intervention that should be present in a population over time for HIV simulations. The
coordinator tracks the actual coverage with the desired coverage; it will poll the population of
nodes it has been assigned to determine how many people have the distributed intervention. When
coverage is less than the desired coverage, it will distribute enough interventions to reach the
desired coverage. This coordinator is similar to the **ReferenceTrackingEventCoordinator**, but adds
HIV-specific disease state qualifiers, such that individuals must be in particular disease states to
qualify for the intervention.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-referencetrackingeventcoordinatorhiv.csv

.. literalinclude:: ../json/campaign-referencetrackingeventcoordinatorhiv.json
   :language: json