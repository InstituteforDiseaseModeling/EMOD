#!/usr/bin/python
TARGET_TIMESTEP = None
import json

def application( timestep ):
    timestep = int(float(timestep))
    params = None
    print(f"Hello from timestep {timestep}")
    if timestep == TARGET_TIMESTEP:
        with open('config.json') as infile:
            params = json.load(infile)['parameters']
            pass
        event_json = params['Event_Template']
        sim_timestep = params['Simulation_Timestep']
        campaign = {}
        campaign["Events"] = []
        event_json["Start_Day"] = float(timestep+sim_timestep)
        campaign["Events"].append( event_json )
        with open( "generated_campaign.json", "w" ) as camp_file:
            json.dump( campaign, camp_file )
            pass
        return "generated_campaign.json"
    else:
        return ""

