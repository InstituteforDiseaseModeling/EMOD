
import json
import random

KEY_INITIAL_EFFECT = "Initial_Effect"
KEY_DEMOGRAPHIC_COVERAGE = "Demographic_Coverage"

def set_random_campaign_file(campaign_filename="campaign.json", campaign_template_filename = "campaign_template.json", debug = False):
    """
    set vaccine initial_effect and demographic_coverage and outbreak demographic_coverage in campaign file
    :param campaign_filename: name of campaign file(campaign.json)
    :param debug:
    :return: None
    """
    with open(campaign_template_filename, "r") as infile:
        campaign_json = json.load(infile)
    initial_effect = random.random()
    demographic_coverage_v = random.random()
    demographic_coverage_o = random.random()
    campaign_json["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"]["Waning_Config"][KEY_INITIAL_EFFECT] = initial_effect
    campaign_json["Events"][0]["Event_Coordinator_Config"][KEY_DEMOGRAPHIC_COVERAGE] = demographic_coverage_v
    campaign_json["Events"][1]["Event_Coordinator_Config"][KEY_DEMOGRAPHIC_COVERAGE] = demographic_coverage_o
    with open(campaign_filename, "w") as outfile:
        json.dump(campaign_json, outfile, indent = 4, sort_keys = True)

    if debug:
        print( "vaccine initial effect is : {}.\n".format(initial_effect) )
        print( "vaccine demographics coverage is  : {0}.\n".format(demographic_coverage_v) )
        print( "outbreak demographics coverage is  : {0}.\n".format(demographic_coverage_o) )

    return

    
def update_config_file( config_filename, campaign_filename ):
    with open(config_filename, "r") as in_file:
        config_json = json.load(in_file)
    
    config_json["parameters"]["Campaign_Filename"] = campaign_filename
    
    with open(config_filename, "w") as out_file:
        json.dump(config_json, out_file, indent = 4, sort_keys = True)

    return
    
    
def application(config_filename="config.json", campaign_filename = "campaign.json", campaign_template_filename = "campaign_template.json",debug = False):
    if debug:
        print( "campaign_filename: " + campaign_filename + "\n" )
        print( "campaign_template_filename: " + campaign_template_filename + "\n" )
        print( "debug: " + str(debug) + "\n" )

    set_random_campaign_file(campaign_filename, campaign_template_filename, debug)
    update_config_file( config_filename, campaign_filename )
    return config_filename

if __name__ == "__main__":
    # execute only if run as a script
    application("config.json")

