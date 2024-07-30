# inset_chart_concat.py
# -----------------------------------------------------------------------------
# Create a third InsetChart.json file that is file 1 + file 2.
# -----------------------------------------------------------------------------

import sys, os, json, collections, struct
import argparse

def ReadJson( json_fn ):
    json_file = open( json_fn,'r')
    json_data = json.load( json_file )
    json_file.close()
    return json_data

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('file_1', help='file to add data to')
    parser.add_argument('file_2', help='file to add to the end of file 1')
    args = parser.parse_args()

    json_data_1 = ReadJson( args.file_1 )
    json_data_2 = ReadJson( args.file_2 )

    if( json_data_1["Header"]["Channels"] != json_data_2["Header"]["Channels"] ):
        print("\nData files dont have the same number of Channels.")
        exit(0)
        
    json_data_1["Header"]["Timesteps"] += json_data_2["Header"]["Timesteps"]

    channels = set(json_data_1["Channels"])

    for chan_title in channels:
        num_steps_1 = len(json_data_1["Channels"][chan_title]["Data"])
        num_steps_2 = len(json_data_2["Channels"][chan_title]["Data"])

        initial_value = 0
        
        if( (chan_title.startswith("Cumulative") or chan_title.startswith("Campaign Cost")) and (num_steps_1 > 0) ):
            initial_value = json_data_1["Channels"][chan_title]["Data"][num_steps_1-1]
            
        for tstep_idx in range( 0, num_steps_2 ):
            json_data_1["Channels"][chan_title]["Data"].append( initial_value + json_data_2["Channels"][chan_title]["Data"][tstep_idx] )

    with open( "InsetChart_concat.json", 'w' ) as handle:
        json.dump(json_data_1, handle, sort_keys=True, indent=4)
