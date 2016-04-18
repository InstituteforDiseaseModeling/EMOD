ERAD-2888

Normally one turns off migration via the Enable_X_Migration flags. But as a workaround for ERAD-2887, 
I need to keep the flag enabled but desire to turn off its effect by setting the corresponding 
scale factor to 0. However this approach failed to work – the same migration activity is still observed
after setting the multiplier to 0.

Run regression scenario 00_Households/test_suite/35_5x5_MigrateTo_Moving and run it with custom reporter libReportNodeDemographics

Open output file ReportNodeDemographics.csv. In the first 5 timesteps (before the MigrateTo intervention kicks in),
some migration activity is observed (which is expected as local migration is enabled).

Modify param_overrides.json to add "x_Local_Migration": 0.

Rerun the scenario.

RESULT
=======
\\bayesianfil01\IDM\home\egall\output\simulations\2016_01_20_12_50_26_236000
Same migration activity still observed in the first few time steps.

EXPECTED
==========
No migration observed in the first few (5) time steps.

!!!!!!!!!!!!!!!!
NOTE: I changed this so that we targeted the outbreak at node 5.  Without human or vector migration, the infection should not spread to people outside node 5.  This can be verified by looking at ReportNodeDemographics.csv.  If you use the pivot table and look at NumInfected, you should see only infections in Node 5.
!!!!!!!!!!!!!!!!
