==========
IVCalendar
==========

The **IVCalendar** intervention class contains a list of ages when an individual will receive the
actual intervention. In **IVCalendar**, there is a list of actual interventions where the
distribution is dependent on whether the individual's age matches the next date in the calendar.
This implies that at a certain age, the list of actual interventions will be distributed according
to a given probability. While a typical use case might involve the distribution of calendars by a
:doc:`parameter-campaign-node-birthtriggerediv` in the context of a routine vaccination schedule, calendars
may also be distributed directly to individuals.

.. include:: ../reuse/warning-case.txt

.. include:: ../reuse/campaign-example-intro.txt

.. csv-table::
    :header: Parameter, Data type, Minimum, Maximum, Default, Description, Example
    :widths: 10, 5, 5, 5, 5, 20, 5
    :file: ../csv/campaign-ivcalendar.csv

.. literalinclude:: ../json/campaign-ivcalendar.json
   :language: json

