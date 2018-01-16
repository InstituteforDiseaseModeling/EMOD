=====================================
CommunityHealthWorkerEventCoordinator
=====================================

The **CommunityHealthWorkerEventCoordinator** coordinator class is used to model a health care worker's ability
to distribute interventions to the nodes and individual in their coverage area. This coordinator
distributes a limited number of interventions per day, and can create a backlog of individuals or
nodes requiring the intervention. For example, individual-level interventions could be distribution
of drugs  and node-level interventions could be spraying houses with insecticide.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-communityhealthworkereventcoordinator.csv

.. literalinclude:: ../json/campaign-communityhealthworkereventcoordinator.json
   :language: json