=================================
ReferenceTrackingEventCoordinator
=================================

The **ReferenceTrackingEventCoordinator** coordinator class defines a particular coverage of an individual-level
persistent intervention that should be present in a population over time. The coordinator tracks the actual
coverage with the desired coverage; it will poll the population of nodes it has been assigned to
determine how many people have the distributed intervention. When coverage is less than the desired
coverage, it will distribute enough interventions to reach the desired coverage.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-referencetrackingeventcoordinator.csv

.. literalinclude:: ../json/campaign-referencetrackingeventcoordinator.json
   :language: json