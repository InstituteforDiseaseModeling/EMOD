================
BirthTriggeredIV
================

The **BirthTriggeredIV** intervention class monitors for birth events and then distributes an actual intervention
to the new individuals as specified in **Actual_IndividualIntervention_Config**.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-birthtriggerediv.csv

.. literalinclude:: ../json/campaign-birthtriggerediv.json
   :language: json
