import json
import numpy as np
import dtk_ImportPressure_Support as ips
import random

KEY_CAMPAIGN_DURATIONS = "Durations"
KEY_CAMPAIGN_DIP = "Daily_Import_Pressures"
KEY_NODE_LIST = "Node_List"
MAX_DURAITON = 800
NUM_OF_BUCKETS = np.random.randint(2, 10)
MAX_RATE = 20

def generate_random_node_list():
    random_list = []
    len_list = random.randint(2,5)
    for i in range(len_list):
        random_node = random.randint(1, 5)
        if random_node not in random_list:
            random_list.append(random_node)
    return sorted(random_list)

def set_random_campaign_file(campaign_filename="campaign.json", campaign_template_filename= "campaign_template.json", debug = False):
    """
    set Durations and Daily_Import_Pressures in campaign file
    :param campaign_filename: name of campaign file(campaign.json)
    :param debug:
    :return: None
    """
    with open(campaign_template_filename, "r") as infile:
        campaign_json = json.load(infile)
    durations = ips.generate_durations(NUM_OF_BUCKETS, MAX_DURAITON)
    if sum(durations) != MAX_DURAITON:
        print( "total duration is {0}, expected {1}.\n".format(sum(durations), MAX_DURAITON) )
    rates = ips.generate_rates(NUM_OF_BUCKETS,MAX_RATE)
    campaign_json["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"][KEY_CAMPAIGN_DURATIONS] = durations
    campaign_json["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"][KEY_CAMPAIGN_DIP] = rates
    random_list = generate_random_node_list()
    campaign_json["Events"][0]["Nodeset_Config"][KEY_NODE_LIST] = random_list

    with open(campaign_filename, "w") as outfile:
        json.dump(campaign_json, outfile, indent = 4, sort_keys = True)

    if debug:
        print( "durations are : {}.\n".format(durations) )
        print( "daily inport pressures are : {}.\n".format(rates) )
    return

def application(config_filename="config.json", debug = True):

    campaign_filename = "campaign.json"
    campaign_template_filename = "campaign_template.json"
    if debug:
        print( "campaign_filename: " + campaign_filename + "\n" )
        print( "campaign_template_filename: " + campaign_template_filename + "\n" )
        print( "debug: " + str(debug) + "\n" )

    set_random_campaign_file(campaign_filename, campaign_template_filename, debug)
    return config_filename


if __name__ == "__main__":
    # execute only if run as a script
    application("config.json")
