********************
   MAIN SCENARIO
********************
- Start by infecting 10% of population with STI
- Select women that have exited a Marital relationship that ended because their partner died 
- Wait 30-60 days before starting purification relationship
- Select women in the WaitToStartPurifyingRelationship state then look for a partner that IsPurifier 
- NLHTIV is listening for event to set WifeInheritanceStage IP 
- Select women in the purification stage that has exited a COMERCIAL relationship that has ended with a partner who is Purifier
- Start a MARITAL relationship with a partner that  CanInheritWives - and use normal marital parameters for duration, condom usage, and coital act rates

 
------------------------------- 
DAY BASED EVENTS:
-------------------------------
 - On Day 1, 
        OutbreakIndividual, Infect to 10% of all individuals
 - On Day 1, 
        set IsPurifier=YES, to 5% of MALEs
 - On Day 1, 
        set CanInheritWives=YES, to 10% of MALEs 

--------------------------------
CONDITION TRIGGERED EVENTS:
--------------------------------
- On Started_Purification, 
        set WifeInheritanceStage=PURIFICATION, to broadcaster FEMALEs id

- (TODO):  On Started_Purification,
        modify STIBarrier - remove use of condom.
        
- On Started_Inherited_Marriage, 
        set WifeInheritanceStage=INHERITED_MARRIAGE, to broadcaster FEMALEs id

- On ExitedRelationship, for any {FEMALE, HasRelationship, of Type=MARITAL, Recently=ENDED, Due_to= PARTNER_DIED, 
        WaitToStartPurifyingRelationship:
            between 30 to 60 days 
            with UNIFORM_DISTRIBUTION
        trigger StartNewRelationship, COMMERCIAL, with any MALE w/IP  IsPurifier=YES  
            and broadcast Started_Purification event.

- On ExitedRelationship, 
        Originator: FEMALE, HasRelationship, of Type=COMMERCIAL, Recently=ENDED, Due_to=NA, 
            with {MALE, who IsPurifier=YES)
        trigger StartNewRelationship, MARITAL, 
            with any MALE w/IP  CanInheritWives=YES
            and broadcast Started_Inherited_Marriage event.

