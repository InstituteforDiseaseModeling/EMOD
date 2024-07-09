#!/usr/bin/python

# Take a flat specific config.json (from regression, say) and a default config and find the values
# that are different in the specific, generating an override with just these (i.e., delete any nodes
# from the local copy of the default that are the same as the config.

import json
import sys
import os

# TBD: Add code to flatten, but for now assume it's flat.

def recursive_node_purger( default_reference_json, specific_flat_json, override_config_json ):
    for node in default_reference_json:
        if json.dumps( default_reference_json[ node ] ).startswith( "{" ):
            recursive_node_purger( default_reference_json[ node ], specific_flat_json, override_config_json[ node ] )
            #print( len( default_reference_json[ node ] ) )
            if len( override_config_json[ node ] ) == 0:
                del override_config_json[ node ]
        else:
            # leaf node, compare values
            if node not in specific_flat_json["parameters"] or node in specific_flat_json["parameters"] and default_reference_json[ node ] == specific_flat_json["parameters"][node]:
                del override_config_json[ node ]
            else:
                override_config_json[ node ] = specific_flat_json["parameters"][node]
if len(sys.argv) < 3:
    print( "\nUsage: extract_overrides.py <default_config_json> <full_config_json> > <param_overrides.json>\n" )
    print( "This tool extracts the minimal param_overrides.json file given a default file and an existing config.json. \nThe output is written to stdout and should be redirected to a param_overrides.json file. \nNote that the exact path to the default file is also part of the output param_overrides.json and may need to be edited post-hoc." )
    sys.exit()

if os.path.exists( sys.argv[1] ) == False:
    print( "Default file path {0} does not seem to exist.".format( sys.argv[1] ) )
    sys.exit()

if os.path.exists( sys.argv[2] ) == False:
    print( "Config file path {0} does not seem to exist.".format( sys.argv[2] ) )
    sys.exit()

default_config_json = json.loads( open( sys.argv[1] ).read() )
specific_config_json = json.loads( open( sys.argv[2] ).read() )
override_config_json = json.loads( open( sys.argv[1] ).read() ) # output json we're going to modify 
override_config_json["Default_Config_Path"] = sys.argv[1]

recursive_node_purger( default_config_json, specific_config_json, override_config_json )
print( json.dumps( override_config_json, indent=4 ) )
