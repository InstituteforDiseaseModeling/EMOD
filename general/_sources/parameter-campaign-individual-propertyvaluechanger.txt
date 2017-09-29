====================
PropertyValueChanger
====================

The **PropertyValueChanger** intervention class assigns new individual property values to
individuals. You must update one property value and have the option to update another using
**New_Property_Value**. This parameter is generally used to move patients from one intervention
state in the health care cascade (InterventionStatus) to another, though it can be used for any
individual property. Individual property values are user-defined in the demographics file (see
:ref:`demo-properties` for more information). Note that the |HINT_s| feature
does not need to be enabled to use this intervention. To instead change node properties, use
:doc:`parameter-campaign-node-nodepropertyvaluechanger`.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-propertyvaluechanger.csv

.. literalinclude:: ../json/campaign-propertyvaluechanger.json
   :language: json