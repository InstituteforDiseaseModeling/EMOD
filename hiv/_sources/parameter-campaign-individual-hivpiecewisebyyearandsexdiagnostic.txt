==================================
HIVPiecewiseByYearandSexDiagnostic
==================================

The **HIVPiecewiseByYearAndSexDiagnostic** intervention class builds on
:doc:`parameter-campaign-individual-hivsimplediagnostic` to configure the roll-out of an intervention
over time. Unlike :doc:`parameter-campaign-individual-hivsigmoidbyyearandsexdiagnostic`,
which requires the time trend to have a sigmoid shape, this intervention allows for any trend of
time to be configured using piecewise or linear interpolation. The trends over time can be
configured differently for males and females. Note that the term "diagnosis" is used because this
builds on the diagnostic classes in |EMOD_s|. However, this intervention is typically used not like
a clinical diagnostic, but more like a trend in behavior or coverage over time.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-hivpiecewisebyyearandsexdiagnostic.csv

.. literalinclude:: ../json/campaign-hivpiecewisebyyearandsexdiagnostic.json
   :language: json