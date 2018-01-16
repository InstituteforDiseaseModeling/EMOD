===================
HIVSimpleDiagnostic
===================

The **HIVSimpleDiagnostic** intervention class is based on the :doc:`parameter-campaign-individual-simplediagnostic`
intervention, but adds the ability to specify outcomes upon both positive and negative diagnosis
(whereas **SimpleDiagnostic** only allows for an outcome resulting from a positive diagnosis).
**HIVSimpleDiagnostic** tests for HIV status without logging the HIV test to the individual’s
medical history. To log the HIV test to the medical history, use :doc:`parameter-campaign-individual-hivrapidhivdiagnostic`
instead.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-hivsimplediagnostic.csv

.. literalinclude:: ../json/campaign-hivsimplediagnostic.json
   :language: json