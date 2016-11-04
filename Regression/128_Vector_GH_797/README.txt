October 10, 2016
GH-797
https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/797
NLHTIV - JsonTypeConfigurationException when distributing some Node-Level Interventions like
"AnimalFeedKill", "InsectKillingFence" or "OutdoorRestKill"

===============
Run Eradiation.exe with the attached config and campaign files with SampleInput files
05_Namawala_Vector_ITNs - Copy.zip

Got the following exception. SpaceSpraying or IndividualIntervention like SimpleBednet work fine.

JsonTypeConfigurationException:
Exception in Configuration.cpp at 597 in GET_CONFIG_STRING.
While trying to parse json data for param >>> class <<< in otherwise valid json segment...
null
Caught exception msg below:
Expected STRING
===============
I looked at AnimalFeedKill and noticed that the error occurs when cloning the intervention. 
When it is cloning, it calls SimpleVectorControlNode copy constructor which attempts to create a 
blocking config object that doesn't exist for AnimalFeedKill.
===============
