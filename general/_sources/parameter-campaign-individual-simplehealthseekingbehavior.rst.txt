===========================
SimpleHealthSeekingBehavior
===========================

The **SimpleHealthSeekingBehavior** intervention class models the time delay that typically occurs
between when an individual experiences onset of symptoms and when they seek help from a health care
provider. Several factors may contribute to such delays including accessibility, cost, and trust in
the health care system. This intervention models this time delay as an exponential process; at every
time step, the model draws randomly to determine if the individual will receive the specified
intervention. As an example, this intervention can be nested in a **NodeLevelHealthTriggeredIV** so
that when an individual is infected, he or she receives a **SimpleHealthSeekingBehavior**,
representing that the individual will now seek care. The individual subsequently seeks care with an
exponentially distributed delay and ultimately receives the specified intervention.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-simplehealthseekingbehavior.csv

.. literalinclude:: ../json/campaign-simplehealthseekingbehavior.json
   :language: json
