======================
HIVDelayedIntervention
======================

**HIVDelayedIntervention** is an intermediate intervention class based on :doc:`parameter-campaign-individual-delayedintervention`,
but adds several features that are specific to the HIV model.
This intervention provides new types of distributions for setting the delay and also enables event
broadcasting after the delay period expires.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-hivdelayedintervention.csv

.. literalinclude:: ../json/campaign-hivdelayedintervention.json
   :language: json
