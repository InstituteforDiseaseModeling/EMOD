GitHub-690
7/28/2016
ReferenceTrackingEventCoordinator: Coverage is tracked to the first value in the 
Time_Value_Map even if End_Year is configured to expire before that.

----
ReferenceTrackingEventCoordinator/ReferenceTrackingEventCoordinatorHIV: 
Coverage is tracked to the first value in the Time_Value_Map even if End_Year is configured 
to expire before that. Repro'ed using either ReferenceTrackingEventCoordinator or ReferenceTrackingEventCoordinatorHIV.

This experiment contains 3 simulations which has the following Time_Value_Map for distributing MaleCircumcision intervention:
"Time_Value_Map": {
"Times": [1962, 1964, 1966], 
"Values": [0.1, 0.5, 0.9] }

In each simulation, the End_Year in the RTEC is different. For the one with End_Year = 1960 
(i.e. expire before the map begins), the coverage is still being tracked to 0.1 in 1962, 
despite the fact that the RTEC should have expired by then.
----
Actually the RTEC's End_Year (1960) is even before the campaign event's Start_Year of 1962, 
it should probably have thrown a configuration error.
----

====
The problem is that RTEC::Update() was setting distribution_complete = true as appropriate, but it doesn't get checked
until STEC::IsFinished() is called.  Hence, the change was to add a check at the beginning of STEC::UpdateNodes() to
not do anything if distribution_complete was true.
====