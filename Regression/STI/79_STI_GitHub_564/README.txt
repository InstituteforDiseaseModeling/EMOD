6-17-2017
GitHub-564
Encountered NullPointerException in Kernel::RelationshipGroups::DepositContagion

----
The following exception was encountered when running this attached sim sti_sim_nullptr_exception.zip:

NullPointerException:
Exception in Eradication\RelationshipGroups.cpp at 324 in Kernel::RelationshipGroups::DepositContagion.
Variable Failed to get relationship pointer for pool index 0
was NULL.

Repro
Extract zip and navigate to the extracted directory.
-C config.json -I \bayesianfil01\idm\public\input\tip\MigrationTest -O testing

This sim uses STI/60_STI_All_Rel_Types_All_Rel_States as a base. An outbreak event was added and Base_Infectivity was cranked up.

Result
00:00:01 [0] [W] [RelationshipMgr] Kernel::RelationshipManager::GetRelationshipById: Failed to find relationship 1 in the container of 104 nodeRelationships at this node. Is this person an immigrant?
00:00:01 [0] [E] [Eradication]
NullPointerException:
Exception in Eradication\RelationshipGroups.cpp at 324 in Kernel::RelationshipGroups::DepositContagion.
Variable Failed to get relationship pointer for pool index 0
was NULL.
----

The null pointer exception was being thrown at time 60.  Hence, this test will only run 100 days.

The issue turns out to be in RelationshipGroup and is related to the optimization that
doesn't immediately remove relationships from the transmission group (MAX_DEAD_REL_QUEUE_SIZE).
The map was getting messed up when relationships left and came back to the node.
