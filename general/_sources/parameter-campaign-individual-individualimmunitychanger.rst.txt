=========================
IndividualImmunityChanger
=========================

The **IndividualImmunityChanger** intervention class acts essentially as a
:doc:`parameter-campaign-individual-multieffectvaccine`,
with the exception of how the behavior is implemented. Rather than
attaching a persistent vaccine intervention object to an individual’s intervention list (as a
campaign-individual-multieffectboostervaccine does), the **IndividualImmunityChanger** directly
alters the immune modifiers of the individual’s susceptibility object and is then immediately disposed
of. Any immune waning is not governed by :doc:`parameter-campaign-waningeffects`, as
:doc:`parameter-campaign-individual-multieffectvaccine` is, but rather
by the immunity waning parameters in the configuration file.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-individualimmunitychanger.csv

.. literalinclude:: ../json/campaign-individualimmunitychanger.json
   :language: json