import json
import os
import argparse

# Constants
Idx_replace_this = 0
Idx_replace_whith_that = 1
Idx_simtypes = 2
Idx_replace_whole_line_at_submatch = 3
All_sim_types = ["GENERIC_SIM", "VECTOR_SIM", "MALARIA_SIM", "ENVIRONMENTAL_SIM", "POLIO_SIM", "AIRBORNE_SIM", "TBHIV_SIM", "STI_SIM","HIV_SIM","PY_SIM", "TYPHOID_SIM", "DENGUE_SIM"]
DeleteLine = 1
Rename = 2
Rename_Or_Add = 3

replace_table = [
    ["Base_Population_Scale_Factor",              "x_Base_Population",                               All_sim_types, Rename,        None],
    ["Incubation_Period_Log_Mean",                "Incubation_Period_Log_Normal_Mu",                 All_sim_types, Rename,        None],
    ["Incubation_Period_Log_Width",               "Incubation_Period_Log_Normal_Sigma",              All_sim_types, Rename,        None],
    ["Base_Incubation_Period",                    "Incubation_Period_Constant",                      All_sim_types, Rename,        None],
    ["Reporting_Period_Log_Mean",                 "Reporting_Period_Log_Normal_Mu",                  "DENGUE_SIM",  Rename,        None],
    ["Reporting_Period_Log_Width",                "Reporting_Period_Log_Normal_Sigma",               "DENGUE_SIM",  Rename,        None],
    ["Infectious_Period_Mean",                    "Infectious_Period_Gaussian_Mean",                 All_sim_types, Rename,        None],
    ["Infectious_Period_Width",                   "Infectious_Period_Gaussian_Std_Dev",              All_sim_types, Rename,        None],
    ["Base_Infectious_Period",                    "Infectious_Period_Exponential",                   All_sim_types, Rename,        None],
    ["Listed_Events",                             "Custom_Individual_Events",                        All_sim_types, Rename,        None],
    ["Immunity_Initialization_Distribution_Type", "Susceptibility_Initialization_Distribution_Type", All_sim_types, Rename,        None],
    ["Enable_Abort_Zero_Infectivity",             "Enable_Termination_On_Zero_Total_Infectivity",    All_sim_types, Rename_Or_Add, 0],
    ["Enable_Immunity_Distribution",              "Enable_Initial_Susceptibility_Distribution",      All_sim_types, Rename,        None],
    ["Enable_Maternal_Transmission",              "Enable_Maternal_Infection_Transmission",          All_sim_types, Rename,        None],
    ["TB_Active_Period_Std_Dev",                  "TB_Active_Period_Gaussian_Std_Dev",               All_sim_types, Rename,        None],
    ["Reporting_Onset_Delay_Mean",                "Reporting_Period_Constant",                       All_sim_types, Rename,        None],

    ["x_Population_Immunity", "",                    All_sim_types, DeleteLine],
    ["Enable_Demographics_Gender", "",               All_sim_types, DeleteLine],
    ["Animal_Reservoir_Type", "",                    All_sim_types, DeleteLine],
    ["Environmental_Incubation_Period", "",          All_sim_types, DeleteLine],
    ["Typhoid_Environmental_Ramp_Up_Duration", "",   "TYPHOID_SIM", DeleteLine],
    ["Typhoid_Environmental_Ramp_Down_Duration", "", "TYPHOID_SIM", DeleteLine],
    ["Typhoid_Environmental_Ramp_Duration", "",      "TYPHOID_SIM", DeleteLine],
    ["Typhoid_Environmental_Peak_Start", "",         "TYPHOID_SIM", DeleteLine],
    ["Typhoid_Environmental_Cutoff_Days", "",        "TYPHOID_SIM", DeleteLine],
    ["Typhoid_Carrier_Probability_Male", "",         "TYPHOID_SIM", DeleteLine],
]

replace_value_table = [
    ["Infectious_Period_Distribution", "FIXED_DURATION",       "CONSTANT_DISTRIBUTION",    All_sim_types],
    ["Infectious_Period_Distribution", "EXPONENTIAL_DURATION", "EXPONENTIAL_DISTRIBUTION", All_sim_types],
    ["Infectious_Period_Distribution", "GAUSSIAN_DURATION",    "GAUSSIAN_DISTRIBUTION",    All_sim_types],
    ["Egg_Hatch_Delay_Distribution",   "EXPONENTIAL_DURATION", "EXPONENTIAL_DISTRIBUTION", All_sim_types],
    ["Incubation_Period_Distribution", "FIXED_DURATION",       "CONSTANT_DISTRIBUTION",    All_sim_types],
    ["Incubation_Period_Distribution", "GAUSSIAN_DURATION",    "GAUSSIAN_DISTRIBUTION",    All_sim_types],
    ["Incubation_Period_Distribution", "EXPONENTIAL_DURATION", "EXPONENTIAL_DISTRIBUTION", All_sim_types],
    ["Incubation_Period_Distribution", "LOG_NORMAL_DURATION",  "LOG_NORMAL_DISTRIBUTION",  All_sim_types],
    ["TB_Active_Period_Distribution",  "EXPONENTIAL_DURATION", "EXPONENTIAL_DISTRIBUTION", All_sim_types],
    ["Reporting_Period_Distribution",  "LOG_NORMAL_DURATION",  "LOG_NORMAL_DISTRIBUTION",  All_sim_types],
]

add_table = [
    [{"Serialization_Type":                 "NONE"}, All_sim_types],
    [{"Enable_Infectivity_Reservoir":       0},      All_sim_types],
    [{"Custom_Coordinator_Events":          []},     All_sim_types],
    [{"Custom_Node_Events":                 []},     All_sim_types],
    [{"Report_Coordinator_Event_Recorder":  0},      All_sim_types],
    [{"Report_Node_Event_Recorder":         0},      All_sim_types],
    [{"Report_Surveillance_Event_Recorder": 0},      All_sim_types],
    [{"Typhoid_Carrier_Probability":        0.5},    "TYPHOID_SIM"],
    [{"Environmental_Peak_Start":           360},    ["ENVIRONMENTAL_SIM", "POLIO_SIM", "TYPHOID_SIM"]],
    [{"Environmental_Ramp_Down_Duration":   170},    ["ENVIRONMENTAL_SIM", "POLIO_SIM", "TYPHOID_SIM"]],
    [{"Environmental_Ramp_Up_Duration":     30},     ["ENVIRONMENTAL_SIM", "POLIO_SIM", "TYPHOID_SIM"]],
    [{"Environmental_Cutoff_Days":          160},    ["ENVIRONMENTAL_SIM", "POLIO_SIM", "TYPHOID_SIM"]],
]


# Functions
def getFilesFromSubDir(path, file_name, file_list):
    ''' returns files (with path) as a list to every file in a directory that maches the passed filter_fct criteria'''
    for file in [f for f in os.listdir(path) if name_config_file in f]:
        filepath = path + "/" + file
        file_list.append(os.path.normcase(filepath))
    for file in [f for f in os.listdir(path) if os.path.isdir(path+"/"+f)]:
        subdir = path+"/"+ file
        print ("checking:", subdir)
        getFilesFromSubDir(subdir, file_name, file_list)
    return file_list


def guessSimTypeFromFileName(file_path):
    file_name = os.path.basename(file_path).lower()
    if "tb" in file_name:
        return "TBHIV_SIM"
    if "malaria" in file_name:
        return "MALARIA_SIM"
    if "polio" in file_name:
        return "POLIO_SIM"
    if "generic" in file_name:
        return "GENERIC_SIM"
    if "hiv" in file_name:
        return "HIV_SIM"
    if "sti" in file_name:
        return "STI_SIM"
    if "typhoid" in file_name:
        return "TYPHOID_SIM"
    if "vector" in file_name:
        return "VECTOR_SIM"
    if "dengue" in file_name:
        return "DENGUE_SIM"
    return None


def getCommandLineArgs():
    parser = argparse.ArgumentParser(description='Converts 2.18 configuration file to 2.20.')
    parser.add_argument("-r", action='store_true', default=False, help="recursive search in subdirectories")
    parser.add_argument("-addNewParameters", action='store_true', default=False, help="add new 2.20 parameters to config, otherwise only replace and delete e.g. for demographics files")
    parser.add_argument("-sortJson", action='store_true', default=False, help="sort the configuration parameters in alphabetic order")
    parser.add_argument("directory", nargs='?', default=".", help="directory where the configuration file(s) to be converted are located (default = . )")
    parser.add_argument("file_name",nargs='?', default="config.json", help="filename for file(s) that should be converted. With -r all files that end with file_name (e.g. config.json converts test_config.json) are found as well. (default = config.json)")
    parser.add_argument("-replace_table", nargs='?', default=[], help="pass your own replace table to the script e.g. [[\"TB_Drug_Cure_Rate\",\"\",\"All_sim_types\",1]]. Quotes have to preceded by a slash \\.")
    parser.add_argument("-defaultType", choices=All_sim_types, default="HIV_SIM", help="Assume that files of an unknown sim type will be assumed to be of this type")
    args = parser.parse_args()
    return args


def replace_keys_in_dict(dic, sim_type):
    replaced = False

    # For each replacement entry in the table...
    for entry in replace_table:

        this_entry_replaced = False

        # If the determined sim type isn't in the entry table, go to the next entry
        if sim_type not in entry[2]:
            continue

        num_found = 0
        values, ignored = find_key_context(entry[0], dic, num_found)

        # While we're still finding new values that could be replaced...
        while values != {}:
            num_found += 1

            # If the key should be replaced, replace it
            if entry[3] is Rename:
                val = values[entry[0]]
                values[entry[1]] = val
                del values[entry[0]]
                this_entry_replaced = True
            # If the key should be deleted, delete it
            elif entry[3] is DeleteLine:
                del values[entry[0]]
                this_entry_replaced = True

            values, ignored = find_key_context(entry[0], dic, num_found)

        # If we never did anything with this entry but it should exist, add it
        if not this_entry_replaced and entry[3] is Rename_Or_Add:
            # Check first that it's not already been renamed
            values, ignored = find_key_context(entry[1], dic, 0)
            if values == {}:
                this_entry_replaced = add_param(dic, entry[1], entry[4])

        replaced = replaced or this_entry_replaced

    return replaced


def replace_values_in_dict(dic, sim_type):
    replaced = False
    num_found = 0

    # For each value replacement entry in the table...
    for entry in replace_value_table:

        # If the determined sim type isn't in the entry table, go to the next entry
        if sim_type not in entry[3]:
            continue

        num_found = 0
        values, ignored = find_key_context(entry[0], dic, num_found)

        # While we're still finding new values that could be replaced...
        while values != {}:
            num_found += 1

            # If the value should be replaced, replace it
            if values[entry[0]] == entry[1]:
                values[entry[0]] = entry[2]
                replaced = True

            values, ignored = find_key_context(entry[0], dic, num_found)

    return replaced


def find_key_context(needle, haystack, ignore_first=0):
    found = {}
    if isinstance(haystack, type(dict())):
        if needle in haystack.keys():
            if ignore_first == 0:
                return haystack, ignore_first
            else:
                ignore_first -= 1
        elif len(haystack.keys()) > 0:
            for key in haystack.keys():
                result, ignore_first = find_key_context(needle, haystack[key], ignore_first)
                if result:
                    return result, ignore_first
    elif isinstance(haystack, type([])):
        for node in haystack:
            result, ignore_first = find_key_context(needle, node, ignore_first)
            if result:
                return result, ignore_first
    return found, ignore_first


def add_param(dict, key, val):
    param_added = False

    if 'parameters' in dict:
        if dict['parameters'].get(key, None) is None:
            dict['parameters'][key] = val
            param_added = True
    else:
        if dict.get(key, None) is None:
            dict[key] = val
            param_added = True

    return param_added


commandline_args = getCommandLineArgs()
WorkingDir = os.path.normpath(commandline_args.directory)
name_config_file = commandline_args.file_name

if commandline_args.r:
    file_list = []
    dirs = getFilesFromSubDir(WorkingDir, name_config_file, file_list)
else:
    dirs = [WorkingDir + "\\" + name_config_file]

if commandline_args.replace_table:
    replace_table = json.loads(commandline_args.replace_table)
    for idx, row in enumerate(replace_table):
        if row[Idx_simtypes] == "All_sim_types":
            replace_table[idx][Idx_simtypes] = All_sim_types

print("converting: ", [os.path.normpath(f) for f in dirs])

for config_file in dirs:
    replaced = False  # File changed?  
    param_added = False # File changed? 2
    # try to determine sim type and add parameters
    with open(config_file, 'r+') as f:
        try:
            j = json.load(f)
        except Exception as e:
            print("Error could not load :", config_file, "\n",  e)
            continue
        found_type = find_key_context("Simulation_Type", j)
        if found_type != ({}, 0):
            sim_type = found_type[0]["Simulation_Type"]
        else:
            sim_type = guessSimTypeFromFileName(config_file) if guessSimTypeFromFileName(config_file) is not None else commandline_args.defaultType

        if commandline_args.addNewParameters:
            for row in add_table:
                if sim_type in row[1]:
                    for key, val in row[0].items():
                        param_added = add_param(j, key, val)

        replaced = replace_keys_in_dict(j, sim_type) or replaced

        replaced = replace_values_in_dict(j, sim_type) or replaced

        if replaced or param_added:
            f.seek(0)
            f.truncate()
            json.dump(j, f, sort_keys=commandline_args.sortJson, indent=4, separators=(',', ': '))
