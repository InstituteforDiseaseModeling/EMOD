1/26/2022
DanB

GH-4629 - ReportInterventionPopAvg.csv: various issues

1) There was a concern that UsageDependentBednet was always giving a usage efficacy of 1.
This turned out that the test that was being used was using Usage_Config and not Usage_Config_List.

2) AdherentDrug's FractionHas Not Making Sense
One would see the metric drop on the first time step but not on following time steps.
The people who decide not to take the first dose, their current drug efficacy is zero. This allows
the intervention to expire. For those who did take the first dose, they will consider the second dose. 
Those who do NOT take the second dose still have drug efficacy > 0 due to taking the first dose
so the intervention stays with them.  After the first dose is taken, we don't see the interventions
expiring until the drug efficacy gets below FLT_EPSILON.

You might notice a little bump on day one of the Has_AdherentDrug-Piperaquine.  The people who 
stop using the intervention on day 101 are people who are not infected. Infected people go into a 
higher update loop (8 times per timestep) so the logic gets checked that in the same time step that 
they want to expire the drug. For the non-infected people, they only get updated once during the 
time step. It takes the second time step for them to determine that they can expire the intervention.

