============
HIVDrawBlood
============

The **HIVDrawBlood** intervention class builds on :doc:`parameter-campaign-individual-hivsimplediagnostic`
to represent phlebotomy for CD4 or viral load testing. It allows for a test
result to be recorded and used for future health care decisions, but does not intrinsically lead to
a health care event. A future health care decision will use this recorded CD4 count or viral load,
even if the actual CD4/viral load has changed since last phlebotomy. The result can be updated by
distributing another **HIVDrawBlood** intervention.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-hivdrawblood.csv

.. literalinclude:: ../json/campaign-hivdrawblood.json
   :language: json