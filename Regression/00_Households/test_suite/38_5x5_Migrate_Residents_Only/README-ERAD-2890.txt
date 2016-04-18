ERAD-2890

Not getting the expected result when trying to migrate residents only.
\\bayesianfil01\IDM\home\testuser\output\simulations\fa6\6fe\e23\fa66fee2-3dc1-e511-93f9-f0921c167860

2 MigrateTo interventions in the campaign:
Day 5 -- move everyone from node 1 to node 2.  
Day 10 --  move all residents  (Target_Residents_Only set to 1) from node 2 to node 3. 

Supposedly the “Is_Moving" flag indicates whether the individual migrating will become resident of the destination node or not.  In this simulation, the first move (on day 5) has the “Is_Moving” flag set to 0.  So it is expected tht node 2 will end up with some non-residents (those from node 1) and hence not everyone in node 2 will move to node 3 on day 10.  However, the result seems to suggests that everyone from node 2 moved to node 3 on day 10.

To Repro
=======
Run simulation with the config/campaign attached, pointing to a dll-path with libReportNodeDemographics.dll

Check ReportNodeDemographics.csv

Expected
=========
On day 10, only residents in node 2 would move to node 3 (those who originally moved from node 1 should not be targeted).

Actual
======
On day 10, everyone in node 2 moved to node 3.

!!!!!!!!
NOTE: If it works correctly, there should be an outbreak in Node 2 only since the non-residents do not move to node 3.  With the bug, everyone would have moved to node 3 and there would have been no outbreak.
!!!!!!!

