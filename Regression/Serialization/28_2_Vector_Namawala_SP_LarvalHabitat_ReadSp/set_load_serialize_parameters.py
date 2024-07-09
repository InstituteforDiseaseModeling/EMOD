import json

path_config = "config_load_sp.json"

with open(path_config, "r") as file:
    po = json.load(file)
    po['parameters']['Serialization_Times'] = [21]
    po['parameters']['Serialized_Population_Filenames'] = ["testing/state-00010.dtk"]
    po['parameters']['Start_Time'] = 0
    po['parameters']["Simulation_Duration"] = 22
    po['parameters']["Vector_Species_Params"][0]["Habitats"][0]["Max_Larval_Capacity"] = 234
 

with open(path_config, "w") as file:
    json.dump(po, file, indent=4)