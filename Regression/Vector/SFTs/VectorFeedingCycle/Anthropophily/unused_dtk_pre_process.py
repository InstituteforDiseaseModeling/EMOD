import numpy as np
import json


def application(config_filename="config.json"):
    with open(config_filename) as infile:
        cdj = json.load(infile)
    random_effect = np.random.uniform(0, 1)
    cdj["parameters"]["Vector_Species_Params"][0]["Anthropophily"] = random_effect
    with open('config.json', 'w') as outfile:
        json.dump(cdj, outfile)


if __name__ == "__main__":
    # execute only if run as a script
    application("config.json")
