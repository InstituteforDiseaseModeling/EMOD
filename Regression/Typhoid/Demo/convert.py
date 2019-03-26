#!/usr/bin/python

import json
import copy
import sys

print( "Processing file: 'campaign.json' (sorry, this is not user-configurable yet)." )
raw_camp = json.loads( open( "campaign_vaccine_assumption_HINT.json" ).read() )
new_camp = raw_camp
print( "Opening and using 'config.json' to find 'Base_Year'." )
config_json = json.loads( open( "config.json" ).read() )

if "Prov:nance" in raw_camp.keys() and raw_camp[ "Provenance" ] == "convert.py":
    print( "This file has already been processed." )
    sys.exit()

new_camp[ "Provenance" ] = "convert.py"

for event in raw_camp[ "Events" ]:
    iv = event[ "Event_Coordinator_Config" ][ "Intervention_Config" ]
    if iv["class"] == "TyphoidVaccine":
        print( "-> Processing a 'TyphoidVaccine' entry from your file." )
        if "Start_Day" in event:
            start_day = event[ "Start_Day" ]
            print( "Found 'Start_Day': Use {0} for Reference_Timer.".format( start_day ) )
        else:
            start_yr = float(event[ "Start_Year" ])
            base_yr = float(config_json[ "parameters"]["Base_Year" ])
            if base_yr > start_yr:
                print( "ERROR: Start_Year = {0}, Base_Year = {1}. Base_Year can't be > Start_Year.".format( start_yr, base_yr ) )
                sys.exit()
            start_day = (start_yr - base_yr) * 365
            print( "Found 'Start_Year': Use {0} for Reference_Timer.".format( start_day ) )

        if "Durability_Map_Years" in iv[ "Changing_Effect" ]:
            print( "Convert Times from absolute years to relative timesteps." )
            new_times = []
            for yr in iv[ "Changing_Effect" ][ "Durability_Map_Years" ][ "Times" ]:
                print( "Convert absolute yr (" + str( yr ) + ") to relative time." )
                if( yr < start_yr ):
                    print( "ERROR: Your year ({0}) value is less than your Start_Year ({1}).".format( yr, start_yr ))
                    sys.exit()

                rel_timestep = ( yr - start_yr ) * 365
                iv[ "Changing_Effect" ][ "Durability_Map_Years" ][ "Times" ] = []
                new_times.append( rel_timestep )
            iv[ "Changing_Effect" ][ "Durability_Map" ] = {}
            iv[ "Changing_Effect" ][ "Durability_Map" ][ "Times" ] = new_times
            iv[ "Changing_Effect" ][ "Durability_Map" ][ "Values" ] = iv[ "Changing_Effect" ][ "Durability_Map_Years" ][ "Values" ]
            del iv[ "Changing_Effect" ][ "Durability_Map_Years" ]

        bti = json.loads( """
        {
            "Event_Coordinator_Config": {
                "Intervention_Config": {
                    "Trigger_Condition": "TriggerList",
                    "Trigger_Condition_List": [ "Births" ],
                    "class": "NodeLevelHealthTriggeredIV",
                    "Actual_IndividualIntervention_Config": {
                        "Mode": "Dose",
                        "Changing_Effect": {
                            "Initial_Effect": 1.0,
                            "class": "WaningEffectMapLinear",
                            "Expire_At_Durability_Map_End": 1,
                            "Reference_Timer_____": 1,
                            "Durability_Map": {
                                "Times": [],
                                "Values": []
                            }
                        },
                        "Route": "TBD",
                        "class": "TyphoidVaccine"
                    }
                },
                "class": "StandardInterventionDistributionEventCoordinator"
            },
            "Nodeset_Config": {
                "class": "NodeSetAll"
            },
            "Start_Day": 1,
            "Target_Demographic": "Everyone",
            "class": "CampaignEvent"
        }
        """
        )
        bti[ "Event_Coordinator_Config" ][ "Intervention_Config" ][ "Actual_IndividualIntervention_Config" ] = copy.deepcopy( iv )
        bti[ "Event_Coordinator_Config" ][ "Intervention_Config" ][ "Actual_IndividualIntervention_Config" ][ "Changing_Effect" ][ "Reference_Timer" ] = start_day;
        new_camp[ "Events" ].append( bti )

print( "Writing new file: 'campaign_new.json' (sorry, this is not user-configurable yet)." )
with open( "campaign_new.json", 'w' ) as cn:
    cn.write( json.dumps( new_camp, sort_keys=True, indent=4 ) )