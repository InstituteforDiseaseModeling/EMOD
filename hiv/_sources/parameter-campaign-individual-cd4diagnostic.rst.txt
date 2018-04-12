=============
CD4Diagnostic
=============

The **CD4Diagnostic** intervention class is similar to :doc:`parameter-campaign-individual-simplediagnostic`,
but adds the ability to divide individual populations based on configurable CD4
count ranges. It uses the individual’s current actual CD4 count, regardless of when a CD4 test has
been performed. An event can then be applied based on the Low or High group to which the individuals
have been moved.


.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-cd4diagnostic.csv

.. literalinclude:: ../json/campaign-cd4diagnostic.json
   :language: json