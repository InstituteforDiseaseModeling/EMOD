========================
NodePropertyValueChanger
========================

The **NodePropertyValueChanger** intervention class sets a given node property to a new value. You can
also define a duration in days before the node property reverts back to its original value, the
probability that a node will change its node property to the target value, and the number of days
over which nodes will attempt to change their individual properties to the target value. This
node-level intervention functions in a similar manner as the individual-level intervention,
:doc:`parameter-campaign-individual-propertyvaluechanger`.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-nodepropertyvaluechanger.csv

.. literalinclude:: ../json/campaign-nodepropertyvaluechanger.json
   :language: json
