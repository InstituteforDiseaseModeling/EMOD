GH-4249 - TransmissionGroups' PopulationSize gets out of sync with non-constant relative biting rate
DanB
9/17/2020

=== Issue===
This is related to #1865.

JoshS had a scenario that ran for over 20,000 days but at day 9120 new infections just stopped. JoshS was using both heterogeneity biting risk (i.e. Enable_Demographic_Risk) and Age_Dependent_Biting_Risk_Type=SURFACE_AREA_DEPENDENT. In #1865, we changed how a person is added to the TransmissionGroups by including the relative biting rate. Without any interventions, we only update a person's inclusion in the TransmissionGroups (i.e. population) when they are born, die, or migrate. The problem arose because as a new born the person's surface area is small so small biting rate which implies small inclusion into the TransmissionGroups population. However, if they die as an adult, their surface area is large and we subtract more than than was put into the TransmissionGroups poplation. New infections stopped because the TransmissionGroups developed a negative population.

Having both heterogeneity biting rate with age dependent makes the situation worse because the heterogeneity is a multiplication factor. Large heterogeneity factor makes the number subtracted at death much larger than at birth. However, you do see the problem without it.

One could probably test this with just age dependent biting risk.

Make all of the people less than 20, say 10.
Run for 10 years.
Have 90% migrate to a node with no people
The bug would cause the remaining people to stop getting infected.

Above shows a test with before and after. All people start at 12 years old. At day 3000, 70% are moved to a second node with no vectors. On day 3200, they all move back.

The difference in the Infected channel is also expected. In the previous code (red), the "populationSize" is based on everyone's biting rate at 12 years old (i.e. 1000 * biting_rate_12y). However, when we determine if a person can get infected, we use their current biting rate so we have:

infection_probability = my_current_biting_rate * ( total_contagion / (1000 * biting_rate_12y) )

As the person ages, my_current_biting_rate gets larger so the red line increases.

The blue line does not have this problem because the equation becomes:

infection_probability = my_current_biting_rate * ( total_contagion / (1000 * my_current_biting_rate) )

(my_current_biting_rate is the same for everyone because they are all the exact same age).