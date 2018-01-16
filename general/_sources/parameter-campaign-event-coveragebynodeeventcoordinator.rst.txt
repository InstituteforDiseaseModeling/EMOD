==============================
CoverageByNodeEventCoordinator
==============================

The **CoverageByNodeEventCoordinator** coordinator class distributes individual-level interventions and is
similar to the **StandardInterventionDistributionEventCoordinator**, but adds the ability to specify
different demographic coverages by node. If no coverage has been specified for a particular node ID,
the coverage will be zero. See the following JSON example and table, which shows all available
parameters for this event coordinator.

.. note::

   This can only be used with individual-level interventions, but |EMOD_s| will not produce an error
   if you attempt to use it with an node-level intervention.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-coveragebynodeeventcoordinator.csv

.. literalinclude:: ../json/campaign-coveragebynodeeventcoordinator.json
   :language: json