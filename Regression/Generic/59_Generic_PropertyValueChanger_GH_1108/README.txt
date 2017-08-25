2/14/2017
GH-1108 - Changing of Properties with New_Property_Value Does Effect PropertyReport

----
There is a Get/SetPropertyReportString() method that is used to speed up the PropertyReport. When changing an individual's property via New_Property_Value, we don't clear the string like we do in PropertyValueChanger.

To solve this, I think we should do the following:

- Make ChangeProperty() a public method of InterventionsContainer and get rid of the IPropertyValueChangerEffects interface.
- Move the broadcast of the PropertyChange event from PropertyValueChanger to InterventionsContainer::ChangeProperty()
- Change BaseIntervention to use InterventionsContainer::ChangeProperty() when updating the New_Property_Value.

----
Look in the PropertyReport.json for the main effect of this change.