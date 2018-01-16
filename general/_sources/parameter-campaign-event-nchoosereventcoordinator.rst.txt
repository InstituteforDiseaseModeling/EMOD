========================
NChooserEventCoordinator
========================

The **NChooserEventCoordinator** coordinator class is used to distribute an individual-level intervention to
exactly N people of a targeted demographic. This contrasts with other event coordinators that
distribute an intervention to a percentage of the population, not to an exact count. See the
following JSON example and table, which shows all available parameters for this event coordinator.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-nchoosereventcoordinator.csv

.. literalinclude:: ../json/campaign-nchoosereventcoordinator.json
   :language: json