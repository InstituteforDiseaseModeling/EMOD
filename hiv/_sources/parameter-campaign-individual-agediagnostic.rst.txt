=============
AgeDiagnostic
=============

The **AgeDiagnostic** intervention class identifies the age threshold of individuals. The minimum
and maximum age ranges are configured (for example, 0-5 and 5-20), and the event is based on the
resulting age of the individuals. This intervention should be used in conjunction with
**StandardInterventionDistributionEventCoordinator**.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-agediagnostic.csv

.. literalinclude:: ../json/campaign-agediagnostic.json
   :language: json