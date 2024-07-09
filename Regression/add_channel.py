import json, sys, os, argparse


def AddChannelFromInsetChart( filename, channelName ):
    
    with open( filename, "r" ) as file:
        json_data = json.load( file )
    
    if ("Channels" not in json_data.keys()) or ("Header" not in json_data.keys()):
        print(filename + "not InsetChart")
        return
    
    if channelName in json_data["Channels"].keys():
        return
        
    print("Converting " + filename)
         
    channel = {}
    channel["Units"] = ""
    channel["Data"] = []

    num = int(json_data["Header"]["Timesteps"])
    for i in range(num):
        channel["Data"].append(0)
        
    json_data["Header"]["Channels"] = json_data["Header"]["Channels"] + 1
    json_data["Channels"][ channelName ] = channel
    
    with open( filename, "w" ) as file:
        file.write( json.dumps( json_data, indent=4, sort_keys=True ) )

def ConvertFilesInDirectories( directory, channelName ):
    filename = os.path.join( directory, "InsetChart.json" )
    if os.path.isfile( filename ):
        AddChannelFromInsetChart( filename, channelName )
    
    for obj in os.listdir( directory ):
        sub_dir = os.path.join( directory, obj )
        if os.path.isdir( sub_dir ):
            ConvertFilesInDirectories( sub_dir, channelName )
        
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', help='Directory InsetChart.json files or subdirectories that do.')
    parser.add_argument('channel_name', help='Name of the channel to add')
    args = parser.parse_args()
    
    ConvertFilesInDirectories( args.directory, args.channel_name )

    