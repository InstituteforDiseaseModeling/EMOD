===================
DelayedIntervention
===================

The **DelayedIntervention** intervention class introduces a delay between when the intervention is
distributed to the individual and when they receive the actual intervention. This is due to the
frequent occurrences of time delays as individuals seek care and receive treatment. This
intervention allows configuration of the distribution type for the delay as well as the fraction of
the population that receives the specified intervention.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-delayedintervention.csv

.. literalinclude:: ../json/campaign-delayedintervention.json
   :language: json
