ERAD-2921

Duration_At_Node_Fixed allows one to specify the amount of time for the individual to spend at the destination node. However, the resulting duration seems to be one day longer than the specified amount.

REPRO
=========
Run sim using the attached campigs where the Duration_At_Node_Fixed is specified to be 10 days. (Make sure libReportNodeDemographics.lib is in the dll-path to generate the desired report)

EXPECED RESULT
===============
On day 5, people from node 1 migrate to node 2 and stay for 10 days before migrating back to node 1.

ACTUAL RESULT
==============
On day 5, people from node 1 migrate to node 2 but stay there for 11 days instead of 10.