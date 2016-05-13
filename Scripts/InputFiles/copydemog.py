import json
import os

input_path = "C:\\Users\\ewenger\\LOCAL\\SVN\\Data_Files\\Seattle"
demog_name = "Seattle_30arcsec_demographics.json"

# copy and modify values
demog_json_file = open( os.path.join( input_path, demog_name) )
demog_json = json.loads( demog_json_file.read() )

demog_json["Defaults"].pop("IndividualAttributes")
demog_json["Defaults"]["NodeAttributes"]= { "Zoonosis" : 0 }

for node in demog_json["Nodes"]:
    node.pop("NodeAttributes")
    node.pop("IndividualAttributes", 0)
    if node["NodeID"] == 45:
        node["NodeAttributes"] = { "Zoonosis" : 1 }

f = open( os.path.join(input_path, "Seattle_30arcsec_demographics_zoonosis.json"), 'w' )
f.write( json.dumps( demog_json, sort_keys=True, indent=4 ) )
f.close()