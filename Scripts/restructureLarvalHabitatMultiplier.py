import argparse
import json
import jsontools as jt
import copy as cp
from numbers import Number
import sys


def restructure_inner_lhm(lhm):
    entry_structure = {"Habitat": "ALL_HABITATS",
                       "Species": "ALL_SPECIES",
                       "Factor": 0.0}

    newlhm = []

    # If it's a number at the top level, create a new all-habitat, all-species entry
    if isinstance(lhm["LarvalHabitatMultiplier"], Number):
        newlhm.append(cp.copy(entry_structure))
        newlhm[-1]["Factor"] = lhm["LarvalHabitatMultiplier"]
    # Otherwise, dig in to the structure
    else:
        for habitat, value in lhm["LarvalHabitatMultiplier"].items():
            # If the entry is a number, create a new all-species entry for this habitat
            if isinstance(value, Number):
                newlhm.append(cp.copy(entry_structure))
                newlhm[-1]["Factor"] = value
                newlhm[-1]["Habitat"] = habitat
            # Otherwise, dig deeper
            else:
                for species, species_val in value.items():
                    newlhm.append(cp.copy(entry_structure))
                    newlhm[-1]["Factor"] = species_val
                    newlhm[-1]["Habitat"] = habitat
                    newlhm[-1]["Species"] = species

    return newlhm


def restructure_config_lhm(data):
    num_found = 0

    lhm, ignored = jt.find_key_context("LarvalHabitatMultiplier", data)

    # While we're still finding LarvalHabitatMultipliers...
    while lhm != {}:
        newlhm = restructure_inner_lhm(lhm)

        lhm["LarvalHabitatMultiplier"] = newlhm
        num_found += 1

        # Look for the next LarvalHabitatMultiplier
        lhm, ignored = jt.find_key_context("LarvalHabitatMultiplier", data, num_found)


def restructure_campaign_lhm(data):
    num_found = 0

    lhm, ignored = jt.find_key_context("Larval_Habitat_Multiplier", data)

    # While we're still finding Larval_Habitat_Multipliers...
    while lhm != {}:
        lhm_graft = {"LarvalHabitatMultiplier": lhm["Larval_Habitat_Multiplier"]}
        newlhm = restructure_inner_lhm(lhm_graft)
        lhm_graft["LarvalHabitatMultiplier"] = newlhm

        lhm["Larval_Habitat_Multiplier"] = lhm_graft
        num_found += 1

        # Look for the next LarvalHabitatMultiplier
        lhm, ignored = jt.find_key_context("Larval_Habitat_Multiplier", data, num_found)


def restructure_file(file_path, as_campaign):
    # Get data from json file
    with open(file_path) as file:
        file_data = json.load(file)

    # Fix up json file
    if as_campaign:
        restructure_campaign_lhm(file_data)
    else:
        restructure_config_lhm(file_data)

    # Write data back to json file
    with open(file_path, 'w') as f:
        json.dump(file_data, f, sort_keys=True, indent=4, separators=(',', ': '))


def restructure_files_in_collection(file_path, as_campaign):
    files = []
    # Read in the collection
    with open(file_path, "r") as file:
        for line in file:
            files.append(line.strip())

    # Replace chunks in every file listed
    for file in files:
        restructure_file(file, as_campaign)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--file_path", help="Path to the JSON file for addition")
    parser.add_argument("--collection_path", help="Path to the file pointing to JSON files for addition")
    parser.add_argument('--as_campaign', default=False, action='store_true')
    args = parser.parse_args()

    if args.file_path and args.collection_path or not args.file_path and not args.collection_path:
        print("Exactly one of file_path or collection_path must be specified")
        sys.exit(2)
    if args.file_path:
        restructure_file(args.file_path, args.as_campaign)
    elif args.collection_path:
        restructure_files_in_collection(args.collection_path, args.as_campaign)
