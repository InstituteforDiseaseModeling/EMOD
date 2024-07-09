import json, sys, os, shutil

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

        
def application( output_path ):

    csv_fn = os.path.join( output_path, "ReportVectorStats.csv" )
    json_fn = csv_fn.replace( "csv", "json" )

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
        
        prev_time = -1.0
        curr_time = -1.0
        for line in rs:
            line = line.rstrip()
            values = line.split(",")
            for index in range(len(values)):
                name = column_names[ index ]
                if( name == "Time" ):
                    curr_time = values[ index ]
                    if( start_time == -1 ):
                        start_time = curr_time
                    if( prev_time != curr_time ):
                        prev_time = curr_time
                        num_timesteps += 1
                        for name in column_names:
                            if( (name != "Time") and (name != "NodeID") ):
                                json_data[ "Channels" ][ name ][ "Data"  ].append( 0.0 )
                elif( name != "NodeID" ):
                    json_data[ "Channels" ][ name ][ "Data"  ][-1] += float(values[ index ])
                    
                

    inset_json = CreateHeader( start_time, num_timesteps, timestep, num_channels )
    
    json_data[ "Header" ] = inset_json
    
    with open( json_fn, "w" ) as handle:
        handle.write( json.dumps( json_data, indent=4, sort_keys=False ) )
    
    
    print("Done converting ReportVectorStats")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("need output path")
        exit(0)

    output_path = sys.argv[1]

    application( output_path )