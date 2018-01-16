========================
StiCoInfectionDiagnostic
========================

The **StiCoInfectionDiagnostic** intervention class is based on :doc:`parameter-campaign-individual-simplediagnostic`
and allows for diagnosis of STI co-infection. It includes **SimpleDiagnostic** features and works in
conjunction with the **ModifyCoInfectionStatus** flag.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-sticoinfectiondiagnostic.csv

.. literalinclude:: ../json/campaign-sticoinfectiondiagnostic.json
   :language: json