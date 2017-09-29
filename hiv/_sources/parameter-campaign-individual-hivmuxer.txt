========
HIVMuxer
========

The **HIVMuxer** intervention class is a method of placing groups of individuals into a waiting
pattern for the next event, and is based on :doc:`parameter-campaign-individual-delayedintervention`.
**HIVMuxer** adds the ability to limit the number of times an individual can
be registered with the delay, which ensures that an individual is only provided with the delay one
time. For example, without **HIVMuxer**, an individual could be given an exponential delay twice,
effectively doubling the rate of leaving the delay.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-hivmuxer.csv

.. literalinclude:: ../json/campaign-hivmuxer.json
   :language: json