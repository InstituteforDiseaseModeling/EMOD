==================
MultiEffectVaccine
==================

The **MultiEffectVaccine** intervention class implements vaccine campaigns in the simulation.
Vaccines can effect all of the following:

* Reduce the likelihood of acquiring an infection
* Reduce the likelihood of transmitting an infection
* Reduce the likelihood of death

After distribution, the effect wanes over time.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-multieffectvaccine.csv

.. literalinclude:: ../json/campaign-multieffectvaccine.json
   :language: json
