import json

path_config = "config_flattened.json"

with open(path_config, "r") as file:
    po = json.load(file)
    del po['parameters']['Vector_Species_Params'][0]

with open(path_config, "w") as file:
    json.dump(po, file, indent=4)