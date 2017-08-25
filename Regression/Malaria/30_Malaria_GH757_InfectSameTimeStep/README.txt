September 19, 2016
GH-757
https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/757
Make infectious bites infect humans in same time step

====
In this test, the vector in Node 1 deposits the contagin at time 18 and is killed at time 19 before depositing any more.
The person in Node 2 moves to Node 1 at time 19.  Before the fix, they are not bitten but become infected.  After the fix,
the person does not become infected.

Node 3 exists only to change the random number stream.  If it is not present before the fix, the person does not get
infected because IndividualMalaria::CalculateInfectiousBites() returns zero even though it thinks the person has been
exposed.


