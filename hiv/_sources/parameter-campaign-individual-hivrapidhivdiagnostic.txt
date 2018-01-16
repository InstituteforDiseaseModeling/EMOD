=====================
HIVRapidHIVDiagnostic
=====================

The **HIVRapidHIVDiagnostic** intervention class builds on :doc:`parameter-campaign-individual-hivsimplediagnostic`
by also updating the individual's knowledge of their HIV status. This can affect their access to ART
in the future as well as other behaviors. This intervention should be used only if the individual’s
knowledge of their status should impact a voluntary male circumcision campaign.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-hivrapidhivdiagnostic.csv

.. literalinclude:: ../json/campaign-hivrapidhivdiagnostic.json
   :language: json