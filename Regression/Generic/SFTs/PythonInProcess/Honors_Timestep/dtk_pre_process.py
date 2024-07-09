#!/usr/bin/python

import json
import dtk_in_process

def application( json_config_path, debug=False ):
    new_config_filename = "config.json"
    with open(new_config_filename, 'r') as infile:
        old_config = json.load(infile)
        pass
    campaign_build_day = old_config['parameters']['Build_Campaign_Event_At']
    simulation_timestep = old_config['parameters']['Simulation_Timestep']
    if debug:
        print(f"campaign build day modulo timestep: {campaign_build_day % simulation_timestep}")
    if campaign_build_day % simulation_timestep != 0:
        campaign_build_day += campaign_build_day%simulation_timestep
        pass
    if debug:
        print(f'Specified build day: {campaign_build_day}')
    old_config['parameters']['Event_Template'] = {
            "Event_Coordinator_Config": {
                "Demographic_Coverage": 0.05,
                "Intervention_Config": {
                    "Antigen": 0,
                    "Genome": 0,
                    "class": "OutbreakIndividual"
                    },
                "Target_Demographic": "Everyone",
                "class": "StandardInterventionDistributionEventCoordinator"
            },
            "Event_Name": "Outbreak_From_Python",
            "Nodeset_Config": {
                "class": "NodeSetAll"
            },
            "Start_Day": None,
            "class": "CampaignEvent"
        }
    with open(new_config_filename, 'w') as outfile:
        json.dump(old_config, outfile, indent=4, sort_keys=True)
        pass

    dtk_in_process.TARGET_TIMESTEP = campaign_build_day

    return new_config_filename 
