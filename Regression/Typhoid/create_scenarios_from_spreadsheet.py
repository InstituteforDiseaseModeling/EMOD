import csv
import sys
import json
import os
import shutil

def num(s):
    try:
        return float(s)
    except ValueError:
        return s

params_json = {}
for col in range(1,19):
    params_json[str(col)] = {}
with open( sys.argv[1], 'rb') as f:
    reader = csv.reader(f) 
    for row in reader:
        param_name = row[0]
        for col in range(1,19):
            params_json[str(col)][param_name] = num(row[col])

for scenario in params_json.keys():
    config_name_id = scenario
    config_name = "Typhoid_" + str(config_name_id)
    if os.path.exists( config_name ) == False:
        os.mkdir( config_name )

    param_overrides_path = os.path.join( config_name, "param_overrides.json" )
    po_json = {}
    po_json["parameters"] = {}
    with open( param_overrides_path, "w" ) as po:
        po_json["parameters"] = params_json[config_name_id]
        po_json["parameters"]["Default_Config_Path"] = "defaults/typhoid_default_config.json"
        po_json["parameters"]["Config_Name"] = config_name
        demog_hint = po_json["parameters"].pop( "demographics" )
        if demog_hint == "large":
            po_json["parameters"]["x_Base_Population"] = 10
        elif demog_hint == "small":
            po_json["parameters"]["x_Base_Population"] = 0.1
        campaign_hint = po_json["parameters"].pop( "outbreaks" )
        
        if campaign_hint == "one large":
            shutil.copy( "campaign_onelarge_template.json", os.path.join( config_name, "campaign.json" ) )
            po_json["parameters"]["Simulation_Duration"] = 30
        elif campaign_hint == "two large":
            shutil.copy( "campaign_twolarge_template.json", os.path.join( config_name, "campaign.json" ) )
            po_json["parameters"]["Simulation_Duration"] = 60
        elif campaign_hint == "annual":
            shutil.copy( "campaign_annual_template.json", os.path.join( config_name, "campaign.json" ) )
            po_json["parameters"]["Simulation_Duration"] = 1825

        po.write( json.dumps( po_json, sort_keys = True, indent=4 ) )

print( "Done!" )
#print( json.dumps( params_json, sort_keys = True, indent=4 ) )
