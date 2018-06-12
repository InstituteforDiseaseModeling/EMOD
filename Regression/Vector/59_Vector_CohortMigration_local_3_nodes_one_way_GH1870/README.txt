12/3/2017
GH-1870
Simulation gets stuck at time step 18 when vector aging and mortality are off

----

After I changed the Egg_Batch_Size from 0 to 10, my simulation got stuck
at time step 18, vector aging and mortality were disabled.
-Ye Chen

----

This takes test 56 and changes the Egg_Batch_Size to > 0 (i.e. 10).  
This causes a bug in the vector refactoring to appear in Update_Immature_Queue() 
when there are cohorts in the Immature queue but no males.
The test is just to see that we get past time 18 in processing.
