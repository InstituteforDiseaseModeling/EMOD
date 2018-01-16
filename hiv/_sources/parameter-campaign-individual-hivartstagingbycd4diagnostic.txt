============================
HIVARTStagingByCD4Diagnostic
============================

The **HIVARTStagingByCD4Diagnostic** intervention class builds on the :doc:`parameter-campaign-individual-hivsimplediagnostic`
intervention by checking for treatment eligibility based on CD4
count. It uses the lowest-ever recorded CD4 count for that individual, based on the history of past
CD4 counts conducted using the HIVDrawBlood intervention. To specify the outcome based on age bins
instead of CD4 testing, use :doc:`parameter-campaign-individual-hivartstagingcd4agnosticdiagnostic`.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-hivartstagingbycd4diagnostic.csv

.. literalinclude:: ../json/campaign-hivartstagingbycd4diagnostic.json
   :language: json