===========================
NChooserEventCoordinatorSTI
===========================

The **NChooserEventCoordinatorSTI** coordinator class distributes an individual-level intervention to exactly N
people of a targeted demographic in STI simulations. This contrasts with other event coordinators
that distribute an intervention to a percentage of the population, not to an exact count. This event
coordinator is similar to the **NChooserEventCoordinator** for other simulation types, but replaces
start and end days with start and end years. See the following JSON example and table, which shows
all available parameters for this event coordinator.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-nchoosereventcoordinatorsti.csv

.. literalinclude:: ../json/campaign-nchoosereventcoordinatorsti.json
   :language: json
