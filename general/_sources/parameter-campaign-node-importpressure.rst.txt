==============
ImportPressure
==============

The **ImportPressure** intervention class extends the **ImportCases** outbreak event. Rather than importing a
deterministic number of cases on a scheduled day, **ImportPressure** applies a set of per-day rates
of importation of infected individuals, over a corresponding set of durations. **ImportPressure**
inherits from **Outbreak**; the **Antigen** and **Genome** parameters are defined as they are for all
**Outbreak** events.

.. warning::

    Be careful when configuring import pressure in multi-node simulations.
    **Daily_Import_Pressures**  defines a rate of per-day importation for *each* node that the
    intervention is distributed to. In a 10 node simulation with  **Daily_Import_Pressures** = [0.1,
    5.0], the total importation rate summed over all nodes will be 1/day and 50/day during the two
    time periods. You must divide the per-day importation rates by the number of
    nodes.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-importpressure.csv

.. literalinclude:: ../json/campaign-importpressure.json
   :language: json