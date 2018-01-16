==========
STIBarrier
==========

The **STIBarrier** intervention is used to reduce the probability of STI or HIV transmission by
applying a time-variable probability of condom usage. Each **STIBarrier** intervention is directed
at a specific relationship type, and must be configured as a sigmoid trend over time.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-stibarrier.csv

.. literalinclude:: ../json/campaign-stibarrier.json
   :language: json
