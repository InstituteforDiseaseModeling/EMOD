GitHub-533
6/10/2016
ReferenceTrackingEventCoordinatorHIV: Target_Disease_State does not work as expected when the 
Time_Value_Map contains coverage of 1

=== Resolution ===
An attempt to reduce the amount of checking was not allowing one to override qualifiesDemographically().
Had to remove optimization.

=== Details ===
The following experiment contains simulations which uses ReferenceTrackingEventCoordinatorHIV to 
track vaccination coverage:
http://idmtvapp17:41523/#explore/Simulations?filters=ExperimentId=f7badf5b-c92e-e611-93fa-0050569e0ef3&orderby=DateCreated+desc&mode=list&count=20&offset=0

This experiment contains 3 sets of simulations in which the Time_Value_Map contains a single 
tracking coverage of 0.5, 0.99 and 1 respectively, each with Target_Disease_State set to Everyone, 
HIV_Positive and HIV_Negative.

When the tracking coverage is 0.5 and 0.99, the behavior observed is as expected -- there the 
appropriate number of Vaccinated events when Target_Disease_State is set to Everyone, HIV_Negative,
 than HIV_Positive.

However, when tracking coverage is set to 1, the coverage seems to be the same regardless of the 
Target_Disease_State (as if it is set to Everyone in all cases).

--- Repro ---
use campigs here: \idmtvfil21\idm\home\egall\output\simulations\fdd\b54\63c\fddb5463-c92e-e611-93fa-0050569e0ef3
and modify RTEC_HIV's Time_Values_Map to contain coverage 0.5, 0.99 and 1 respectively and 
look at the number of Vaccinated events.

Or refer to the following experiment:
http://idmtvapp17:41523/#explore/Simulations?filters=ExperimentId=f7badf5b-c92e-e611-93fa-0050569e0ef3&orderby=DateCreated+desc&mode=list&count=20&offset=0

For each of the simulation, press "Chart" and select "Number of events -- Vaccinated" channel.

--- Actual Result ---
When coverage is 1, the coverage appears to be applied to everyone regardless of what 
Target_Disease_State is set to.