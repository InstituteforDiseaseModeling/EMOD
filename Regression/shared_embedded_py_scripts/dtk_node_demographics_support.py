import json, sys, os, shutil
import pandas as pd
import numpy as np


def CreateHeader( start_time, num_timesteps, timestep, num_channels ):

    inset_json = {}

    inset_json["DateTime"            ] = "Unknown"
    inset_json["DTK_Version"         ] = "Unknown"
    inset_json["Report_Type"         ] = "InsetChart"
    inset_json["Report_Version"      ] = "1.0"
    inset_json["Start_Time"          ] = start_time
    inset_json["Simulation_Timestep" ] = timestep
    inset_json["Timesteps"           ] = num_timesteps
    inset_json["Channels"            ] = num_channels
    
    return inset_json


def ConvertReportNodeDemographics( output_path, report_to_convert_name ):

    csv_fn = os.path.join( output_path, report_to_convert_name )
    
    json_fn = "InsetChart_"+report_to_convert_name
    json_fn = json_fn.replace( "csv", "json" )
    json_fn = os.path.join( output_path, json_fn )

    # data for header
    start_time = -1
    num_timesteps = 0
    timestep = 1 # assume vectors so assume 1
    num_channels = 0
    
    json_data = {}
    json_data[ "Channels" ] = {}
    
    with open( csv_fn, "r" ) as rs:
        header = rs.readline() # assume Time is first column
        header = header.rstrip()
        column_names = header.split(",")
        num_channels = len(column_names) - 2 # No channles for Time and NodeID
        for name in column_names:
            if( (name != "Time") and (name != "NodeID") ):
                json_data[ "Channels" ][ name ] = {}
                json_data[ "Channels" ][ name ][ "Units" ] = ""
                json_data[ "Channels" ][ name ][ "Data"  ] = []
        prev_time = -1
        for line in rs:
            line = line.rstrip()
            values = line.split(",")
            num_timesteps += 1
            for index in range(len(values)):
                name = column_names[ index ]
                if( name == "Time" ):
                    time = values[ index ]
                    if( start_time == -1 ):
                        start_time = time
                    if( time != prev_time ):
                        for channel_name in column_names:
                            if( (channel_name != "Time") and (channel_name != "NodeID") ):
                                json_data[ "Channels" ][ channel_name ][ "Data"  ].append( 0.0 )
                                prev_time = time
                elif( name != "NodeID" ):
                    json_data[ "Channels" ][ name ][ "Data"  ][ -1 ] += float(values[ index ])
                                    

    inset_json = CreateHeader( start_time, num_timesteps, timestep, num_channels )
    
    json_data[ "Header" ] = inset_json
    
    with open( json_fn, "w" ) as handle:
        handle.write( json.dumps( json_data, indent=4, sort_keys=False ) )
    
    print("Done converting "+report_to_convert_name)
    
    return
    
