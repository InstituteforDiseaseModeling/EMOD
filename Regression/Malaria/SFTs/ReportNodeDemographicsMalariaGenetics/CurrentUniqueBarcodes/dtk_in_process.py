#!/usr/bin/python
import json


def application(timestep):
    timestep = int(float(timestep))
    with open('campaign.json') as infile:
        campaign = json.load(infile)
    interventions = campaign['Events'][0]['Event_Coordinator_Config']['Intervention_Config']['Intervention_List']
    # update Start_Day so the campaign is used, otherwise it gets discarded
    campaign['Events'][0]['Start_Day'] = timestep + 1
    # slowly replacing As with Cs to make unique Barcode for each outbreak
    for outbreak in interventions:
        barcode = outbreak['Barcode_String']
        new_gene = "C" if timestep < len(barcode) else "T"
        timestep = timestep % len(barcode)
        new_barcode = str(barcode)[:timestep] + new_gene + str(barcode)[timestep + 1:]
        outbreak['Barcode_String'] = new_barcode

    with open("campaign.json", "w") as camp_file:
        json.dump(campaign, camp_file)
        pass
    # for saving all the campaigns generated
    # with open(f"campaign_timestep_{timestep}.json", "w") as camp_file:
    #     json.dump(campaign, camp_file)
    #     pass

    return "campaign.json"
