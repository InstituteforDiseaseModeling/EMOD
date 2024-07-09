==========================================
Testing StartNewRelationship intervention
==========================================

With a 5 nodes each of 

- Create a new individual property: 
                "Property": "StartNewRels"
                "Values": [ "YAY_SINGLE", "CREATED", "IS_COMPLICATED" ]
                "Initial_Distribution": [ 0.50, 0.25, 0.25 ]


- Add a campaign intervention that change:
    StartNewRels.CREATED
    StartNewRels.IS_COMPLICATED

- Add campaign intervention to StartNewRelationships for:
    MARITAL, on Females, to Partner_Has_IP YAY_SINGLE
    COMERCIAL, on Males, to Partner_Has_IP YAY_SINGLE
    INFORMAL, on Males, to Partner_Has_IP IS_COMPLICATED
    TRANSITORY, on Males, to Partner_Has_IP IS_COMPLICATED

Validate gereration of new relationships outside PFA
Validate usage of Relationship_Type
Validate Gender of Originator and Target are properly placed (create a join with the dataframes from EventsRecorder and StartNewRelationship)
Validate report columns
Try to generate plots with the channels related to StartNewRelationship.  If plot generation fails it should also fail the Test.




