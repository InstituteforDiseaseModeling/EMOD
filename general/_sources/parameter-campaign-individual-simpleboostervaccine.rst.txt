====================
SimpleBoosterVaccine
====================

The **SimpleBoosterVaccine** intervention class is derived from :doc:`parameter-campaign-individual-simplevaccine`
and preserves many of the same parameters. The behavior is much like **SimpleVaccine**, except that upon distribution
and successful take, the vaccine's effect is determined by the recipient's immune state. If the
recipient’s immunity modifier in the corresponding channel (acquisition, transmission, or mortality) is
above a user-specified threshold, then the vaccine’s initial effect will be equal to the
corresponding priming parameter. If the recipient’s immune modifier is below this threshold, then
the vaccine's initial effect will be equal to the corresponding boosting parameter. After
distribution, the effect wanes, just like **SimpleVaccine**. In essence, this intervention
provides a **SimpleVaccine** intervention with one effect to all naive (below- threshold)
individuals, and another effect to all primed (above-threshold) individuals; this behavior is
intended to mimic biological priming and boosting.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-simpleboostervaccine.csv

.. literalinclude:: ../json/campaign-simpleboostervaccine.json
   :language: json