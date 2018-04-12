=============
SimpleVaccine
=============

The **SimpleVaccine** intervention class implements vaccine campaigns in the simulation. Vaccines can have
an effect on one of the following:

* Reduce the likelihood of acquiring an infection
* Reduce the likelihood of transmitting an infection
* Reduce the likelihood of death

To configure vaccines that have an effect on more than one of these, use
:doc:`parameter-campaign-individual-multieffectvaccine` instead.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-simplevaccine.csv


.. literalinclude:: ../json/campaign-simplevaccine.json
   :language: json