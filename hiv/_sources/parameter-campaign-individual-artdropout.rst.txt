==========
ARTDropout
==========

The **ARTDropout** intervention class removes an individual from antiretroviral therapy (ART) and
interrupts their progress through the cascade of care. The individual's infectiousness will return
to a non-suppressed level, and a new prognosis will be assigned.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-artdropout.csv

.. literalinclude:: ../json/campaign-artdropout.json
   :language: json