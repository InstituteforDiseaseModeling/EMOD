The purpose of this test is have something that makes sure that 
StandardEventCoordinator::preDistribute() is only called once
instead of once for each node. This really just impacts
Reference Tracker.  In RTEC, it is causing performance
issues as well as causing the later nodes to not get the correct
number of interventions.

When evalulating it, the Initiate_Prep channel in InsetChart
should match very closely.  If not, something is wrong.