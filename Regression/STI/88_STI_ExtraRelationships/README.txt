GH-1728 - Changing Risk type cause individuals to be superspreaders
7/18/2017

If during an individual's initialization they are configured such that they are
allowed extra commercial relationships (i.e. Prob_Extra_Relationship_XXX > 0) and
they change Risk types, they will considered a superspreader and be allowed extra 
relationships of all types. Note that the value Max_Simultaneous_Relationships_XXX 
does not matter. If this is zero and Prob_Extra_Relationship_XXX > 0, then it still
applies.

The problem was because the super spreader bitmask was 0x8. If extra commercial 
relationships are allowed, it would set this bit. When the person changed risk type, 
the logic to update promiscuity_flags would first check if the person was a super spreader. 
If they were, then they would automatically be allowed extra relationships of all types. 
By setting the bitmax to 0x80, the super spreader flag and the commercial flag do not 
conflict.

--------------
To verify this, run this test before this change and after.  Before the change, you should
observe in RelationshipStart.csv that the individuals in all relationships are super spreaders
and their A_extra_relational_bitmask is 10.  You should also observe that after 1980 when
individuals are changed from Risk:MEDIUM to Risk:LOW, A_extra_relational_bitmask becomes 15
instead of 8.  After the change, no one is a super spreader and the bitmask becomes 8.

If you run the check_for_concurrent.py script on the output, before the change you should
see > 3000 concurrent relationships that are LOW-LOW, but after the change zero.
