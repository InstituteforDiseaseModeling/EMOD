#convert_intervention_state_to_status.py
# -----------------------------------------------------------------------------
# The purpose of this script is to convert campaign.json files:
# - Invalid_Intervention_States -> Disqualifying_Properties
# - Intervention_State          -> New_Property_Value
#
# The Valid_Intervention_States defined in the config.json file are expected to
# be moved to a demographics file IndividualProperty by hand.  It is assumed that
# the IndividualProperty used will be InterventionStatus.
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
    json.dump( data, output_file, indent=4 )
    output_file.close()
    return

def UpdateStatus( intervention ):
    if( intervention.has_key( "Invalid_Intervention_States") ):
        new_list = []
        for state in intervention["Invalid_Intervention_States"]:
            ip_name = "InterventionStatus:" + state
            new_list.append( ip_name )
        intervention[ "Disqualifying_Properties" ] = new_list
        del intervention["Invalid_Intervention_States"]

    if( intervention.has_key( "Intervention_State") ):
        intervention["New_Property_Value"] = "InterventionStatus:" + intervention["Intervention_State"]
        del intervention["Intervention_State"]

    for val in intervention.values():
        if( type(val) is OrderedDict ):
            UpdateStatus( val )
        if( type(val) is list ):
            for list_val in val:
                if( type(list_val) is OrderedDict ):
                    UpdateStatus( list_val )

    return


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
        UpdateStatus( intervention )

    WriteJson( campaign_data, output_fn )
    print( "wrote output file = " + output_fn )
