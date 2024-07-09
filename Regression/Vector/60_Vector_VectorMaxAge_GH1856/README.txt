12/3/2017
GH-1856
Vector: Eradication crashes when Vector_Aging is enabled

----

00:00:04 [0] [I] [Simulation] Update(): Time: 149.0 Rank: 0 StatPop: 303 Infected: 303
Assertion failure, (i_age <= I_MAX_AGE), is false in file Eradication\VectorPopulation.cpp at line 284

----

In this scenario, Vector_Aging was enabled but Vector_Mortality was not.
This means you can have very old vectors.  The assert was in some code done
to improve performance and it assumed that vectors would not be older than
150 days.

The fix is to say the vector is 150 days old when they are older than that
for the mortality calculation.  The difference when you get over 150 are
very small so this is a good enough approximation.

One should note that Jaline ran into this assert once in one of her sims.