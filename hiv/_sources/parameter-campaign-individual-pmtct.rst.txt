=====
PMTCT
=====

The **PMTCT** (Prevention of Mother-to-Child Transmission) intervention class is used to define the
efficacy of **PMTCT** treatment at time of birth. This can only be used for mothers who are not on
suppressive ART and will automatically expire 40 weeks after distribution. Efficacy will be reset to
0 once it expires.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-pmtct.csv

.. literalinclude:: ../json/campaign-pmtct.json
   :language: json