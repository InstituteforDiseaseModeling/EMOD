#convert_nlhtiv_to_trigger_condition_list.py
# -----------------------------------------------------------------------------
# The purpose of this script is to convert campaign.json files that use NodeLevelHealthTriggeredIV
# to only using Trigger_Condition_List.  Users could have:
# - "Trigger_Condition" with it containing an event name
# - "Trigger_Condition" with the value "TriggerString" plus the parameter "Trigger_Condition_String" containing the event name
# - "Trigger_Condition" with the value "TriggerList" plus the parameter "Trigger_Condition_List" with an array of event names.
# This script will convert these different posibilities so that only "Trigger_Condition_List" is used.
# -----------------------------------------------------------------------------

import sys, os, json, collections, struct
from collections import OrderedDict

def ReadJson( json_fn ):
    json_file = open( json_fn,'r')
    json_data = json.load( json_file, object_pairs_hook=OrderedDict )
    json_file.close()
    return json_data

def WriteJson( data, output_fn ):
    output_file = open( output_fn, 'w' )
    json.dump( data, output_file, indent=4, separators=(',',': ') )
    output_file.close()
    return

def UpdateNLHTIV( nlhtiv ):
    if( nlhtiv.has_key("Trigger_Condition") ):
        tc_list = []

        tc = nlhtiv["Trigger_Condition"]
        #del nlhtiv["Trigger_Condition"]

        if( tc == "TriggerList" ):
            if( nlhtiv.has_key("Trigger_Condition_List") ):
                tc_list = nlhtiv["Trigger_Condition_List"]
            else:
                raise Exception( "Expected Trigger_Condition_List" )
        elif( tc == "TriggerString" ):
            if( nlhtiv.has_key("Trigger_Condition_String") ):
                tc_list.append( nlhtiv["Trigger_Condition_String"] )
                del nlhtiv["Trigger_Condition_String"]
            else:
                raise Exception("Expected Trigger_Condition_String")
        else:
            tc_list.append( tc )

        nlhtiv = OrderedDict([("Trigger_Condition_List",tc_list) if k == "Trigger_Condition" else (k, v) for k, v in nlhtiv.items()])
    elif( not nlhtiv.has_key("Trigger_Condition_List" ) ):
        raise Exception("Expected Trigger_Condition")

    return nlhtiv


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print ("\nUsage: %s <input filename> <output filename>" % os.path.basename(sys.argv[0]))
        exit(0)

    input_fn  = sys.argv[1]
    output_fn = sys.argv[2]

    campaign_data = ReadJson( input_fn )

    for campaign_event in campaign_data[ "Events" ]:
        event_coord = campaign_event[ "Event_Coordinator_Config" ]
        intervention = event_coord[ "Intervention_Config" ]
        print( "EC=" + event_coord["class"] )
        if( (intervention["class"] == "NodeLevelHealthTriggeredIV") or (intervention["class"] == "NodeLevelHealthTriggeredIVScaleUpSwitch") ):
            intervention = UpdateNLHTIV( intervention )
            event_coord[ "Intervention_Config" ] = intervention

    WriteJson( campaign_data, output_fn )
    print( "wrote output file = " + output_fn )
