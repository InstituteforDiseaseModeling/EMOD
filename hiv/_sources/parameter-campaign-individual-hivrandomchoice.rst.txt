===============
HIVRandomChoice
===============

The **HIVRandomChoice** intervention class is based on
:doc:`parameter-campaign-individual-simplediagnostic`, but adds parameters to change the logic in
how and where treatment is applied to individuals based on specified probabilities.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-hivrandomchoice.csv

.. literalinclude:: ../json/campaign-hivrandomchoice.json
   :language: json