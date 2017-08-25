GitHub-521
6-6-2016
Assertion failure in Eradication\RelationshipGroups.cpp

Encountered the following assertion:
Assertion failure, ((unsigned int)propertyValueToIndexMap.at(propertyName).at(propertyValue) <= max_index), is false in file Eradication\RelationshipGroups.cpp at line 196

In RelationshipManager::RemoveFromPrimaryRelationships(), we gain a very significant
reduction in runtime by batching the deleting of the relationships from the transmission group.
However, this means that we try to function with entries in the map that do not exist.
For example, when a person migrates, their relationship should be removed.  Since it is not removed
immediately, the index value could be invalid.  The check makes sure that the index is at least
a valid value.  This implies that we need other checks to make sure we don't spread the disease
when people are not in a normal relationship (i.e. in the same node).

The "solution" was not to remove the assert and put an 'if' check around the use of the potentially
invalid value.

NOTE: I still don't like this solution.  I understand the huge performance gain, but I wish I better
understood what his happening with these potentially bad values.


