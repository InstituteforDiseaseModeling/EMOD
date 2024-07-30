import numpy as np
import json


def application(config_filename="config.json"):
    with open(config_filename) as infile:
        cdj = json.load(infile)
    campaign_filename = cdj["parameters"]["Campaign_Filename"]
    with open(campaign_filename) as infile:
        campaign_json = json.load(infile)
    random_effect = np.random.uniform(0, 1)
    campaign_json["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"]["Killing_Config"]["Initial_Effect"] = random_effect
    with open(campaign_filename, 'w') as campaign:
        json.dump(campaign_json, campaign, indent=4, sort_keys=True)


if __name__ == "__main__":
    # execute only if run as a script
    application("config.json")
