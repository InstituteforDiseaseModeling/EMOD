================================
HIVSigmoidByYearAndSexDiagnostic
================================

The **HIVSigmoidByYearAndSexDiagnostic** intervention class builds on :doc:`parameter-campaign-individual-hivsimplediagnostic`
by allowing the probability of "positive diagnosis" to be configured sigmoidally in time. For a linear approach, use
:doc:`parameter-campaign-individual-hivpiecewisebyyearandsexdiagnostic`.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-hivsigmoidbyyearandsexdiagnostic.csv

.. literalinclude:: ../json/campaign-hivsigmoidbyyearandsexdiagnostic.json
   :language: json