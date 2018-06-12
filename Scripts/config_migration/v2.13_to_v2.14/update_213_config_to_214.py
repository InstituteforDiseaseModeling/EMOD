'''
- Replaces strings in files
- replacing of string can be conditioned on simulation types
- replace line based on matching substring e.g. to delete a line
'''


import json
import os
import argparse
import sys

#Constants
Idx_replace_this = 0
Idx_replace_whith_that = 1
Idx_simtypes = 2
Idx_replace_whole_line_at_submatch = 3
All_sim_types = ["GENERIC_SIM", "VECTOR_SIM" ,"MALARIA_SIM", "ENVIRONMENTAL_SIM", "POLIO_SIM", "AIRBORNE_SIM", "TB_SIM", "TBHIV_SIM", "STI_SIM","HIV_SIM","PY_SIM", "TYPHOID_SIM", "DENGUE_SIM"]
DeleteLine = 1
Rename = 2

add_table=[
            [{"Enable_Immunity_Distribution": 0,
               "Enable_Initial_Prevalence": 0,
               "Enable_Maternal_Infection_Transmission": 0,
               "Enable_Maternal_Protection": 0,
               "Enable_Natural_Mortality": 0,
               "Enable_Susceptibility_Scaling": 0,
               "Enable_Disease_Mortality":0,
               "Enable_Maternal_Antibodies_Transmission":0}, All_sim_types],
            [{"Enable_Skipping": 0}, ["GENERIC_SIM", "TB_SIM"]]
        ]

replace_table = [
                    ["Enable_Demographics_Other", "Enable_Demographics_Risk",   All_sim_types,                                                                                                                          Rename],
                    ["Enable_Maternal_Transmission", "Enable_Maternal_Infection_Transmission",  All_sim_types,                                                                                                          Rename],
                    ["Maternal_Transmission_Probability", "Maternal_Infection_Transmission_Probability",  All_sim_types,                                                                                                Rename],
                    ["TB_Drug_Clearance_Rate_HIV", "TB_Drug_Cure_Rate_HIV",     ["HIV_SIM", "TBHIV_SIM"],                                                                                                              Rename],
                    ["TB_Drug_Clearance_Rate_MDR", "TB_Drug_Cure_Rate_MDR",     ["HIV_SIM", "TBHIV_SIM"],                                                                                                              Rename],
                    ["TB_Drug_Clearance_Rate", "TB_Drug_Cure_Rate",             ["HIV_SIM", "TBHIV_SIM"],                                                                                                              Rename],
                    ["AbovePoverty", "",                                        All_sim_types,                                                                                                                  DeleteLine],
                    ["Enable_Disease_Mortality",                    "",  [                                                                      "HIV_SIM",                  "TYPHOID_SIM",  "DENGUE_SIM",   "POLIO_SIM"], DeleteLine],
                    ["Enable_Immunity",                             "",  [                                                      "MALARIA_SIM",  "HIV_SIM",                  "TYPHOID_SIM",  "DENGUE_SIM",   "POLIO_SIM"], DeleteLine],
                    ["Enable_Immune_Decay",                     "",  [                                                  "MALARIA_SIM",  "HIV_SIM",                  "TYPHOID_SIM",  "DENGUE_SIM",   "POLIO_SIM"], DeleteLine],
                    ["Enable_Immunity_Distribution",            "",  [                                                                  "HIV_SIM",                  "TYPHOID_SIM",                  "POLIO_SIM"], DeleteLine],
                    ["Enable_Initial_Prevalence",               "",  [                                                                  "HIV_SIM",  "TBHIV_SIM",    "TYPHOID_SIM",  "DENGUE_SIM",   "POLIO_SIM"], DeleteLine],
                    ["Enable_Demographics_Risk",                "",  ["GENERIC_SIM",    "STI_SIM",  "ENVIRONMENTAL_SIM",                "HIV_SIM",                  "TYPHOID_SIM",                  "POLIO_SIM"], DeleteLine],
                    ["Enable_Maternal_Infection_Transmission",  "",  [                                                                  "HIV_SIM",                  "TYPHOID_SIM",    "DENGUE_SIM",     "POLIO_SIM"], DeleteLine],
                    ["Enable_Superinfection",                   "",  [                                                                  "HIV_SIM",  "TBHIV_SIM",    "TYPHOID_SIM",  "DENGUE_SIM",   "POLIO_SIM"], DeleteLine],
                    ["Base_Infectious_Period",                      "", ["MALARIA_SIM", "HIV_SIM", "TBHIV_SIM", "TYPHOID_SIM", "DENGUE_SIM", "TB_SIM"], DeleteLine],
                    ["Infectious_Period_Min",                       "", ["MALARIA_SIM", "HIV_SIM", "TBHIV_SIM", "TYPHOID_SIM", "DENGUE_SIM", "TB_SIM"], DeleteLine],
                    ["Infectious_Period_Mean",                      "", ["MALARIA_SIM", "HIV_SIM", "TBHIV_SIM", "TYPHOID_SIM", "DENGUE_SIM", "TB_SIM"], DeleteLine],
                    ["Infectious_Period_Distribution",              "", ["MALARIA_SIM", "HIV_SIM", "TBHIV_SIM", "TYPHOID_SIM", "DENGUE_SIM", "TB_SIM"], DeleteLine],
                    ["Base_Infectivity",                                "", [ "TYPHOID_SIM",  "DENGUE_SIM", "POLIO_SIM", "MALARIA_SIM"], DeleteLine],
                    ["Urban",                                "", All_sim_types, DeleteLine],
                    ["Enable_Vital_Dynamics", "", ["HIV_SIM"], DeleteLine],
                    ["Enable_Immunity_Initialization_Distribution", "", All_sim_types, DeleteLine],
                    ["Enable_Superinfection", "", ["POLIO_SIM"], DeleteLine]
                ]

DefaultSimType = "HIV_SIM"

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
        return "TB_SIM"
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
    parser = argparse.ArgumentParser(description='Converts 2.13 configuration file to 2.14.')
    parser.add_argument("-r", action='store_true', default=False, help="recursive search in subdirectories")
    parser.add_argument("-addNewParameters", action='store_true', default=False, help="add new 2.14 parameters to config, otherwise only replace and delete e.g. for demographics files")
    parser.add_argument("-sortJson", action='store_true', default=False, help="sort the configuration parameters in alphabetic order")
    parser.add_argument("directory", nargs='?', default=".", help="directory where the configuration file(s) to be converted are located (default = . )")
    parser.add_argument("file_name",nargs='?', default="config.json", help="filename for file(s) that should be converted. With -r all files that end with file_name (e.g. config.json converts test_config.json) are found as well. (default = config.json)")
    parser.add_argument("-replace_table", nargs='?', default=[], help="pass your own replace table to the script e.g. [[\"TB_Drug_Cure_Rate\",\"\",\"All_sim_types\",1]]. Quotes have to preceded by a slash \\.")
    args = parser.parse_args()
    return args

def replace_in_dict(dic, sim_type):
    replaced = False
    if isinstance(dic, list):
        for d in dic:
            replaced = replace_in_dict(d, sim_type) or replaced
    elif isinstance(dic, dict):
        for key in list(dic.keys()):
            print(key)
            for entry in replace_table:
                if sim_type in entry[Idx_simtypes]: #if sim_type is none we are parsing not a config.json file
                    if entry[Idx_replace_this] == key:
                        if entry[Idx_replace_whole_line_at_submatch] == DeleteLine:
                            del dic[key]
                        elif entry[Idx_replace_whole_line_at_submatch] == Rename:
                            dic[entry[Idx_replace_whith_that]]= dic[key]    #new entry with old value i.e. rename
                            del dic[key]
                        replaced = True
            if dic.get(key, None) is not None:
                replaced = replace_in_dict(dic[key], sim_type) or replaced
    return replaced


#use directory
#WorkingDir = os.path.normcase(r"C:\\Users\\tfischle\\Github\\DtkTrunk_master_213\\Regression\\Multicore_Nosibe\\42_Vector_SimpleIndividualRepellent")
#name_config_file = "config.json"    # sub strings like ".json", "overrides.json" works as well
commandline_args =  getCommandLineArgs()
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
    # try to determine sim type and add parameters
    with open(config_file, 'r+') as f:
        try:
            j = json.load(f)
        except Exception as e:
            print("Error could not load :", config_file, "\n",  e)
            continue
        if "parameters" in j.keys():
            if "Simulation_Type" in j["parameters"].keys():
                sim_type = j["parameters"]["Simulation_Type"]    # works only in config.json
        else:
            sim_type = guessSimTypeFromFileName(config_file) if guessSimTypeFromFileName(config_file) is not None else DefaultSimType

        if commandline_args.addNewParameters:
            for row in add_table:
                if sim_type in row[1]:
                    for key, val in row[0].items():
                        if 'parameters' in j:
                            if j['parameters'].get(key, None) is None:      # add under "parameters"
                                j['parameters'][key]=val
                                param_added = True
                        else:
                            if j.get(key, None) is None:
                                j[key]=val
                                param_added = True

        replaced = replace_in_dict(j, sim_type) or replaced

        if replaced:
            f.seek(0)
            f.truncate()
            #new_file = '\n'.join(str(line) for line in content) #create string
            json.dump(j, f, sort_keys=commandline_args.sortJson, indent=4, separators=(',', ': '))





    # with open(config_file, 'w') as f:
    #     json.dump(j, f, sort_keys=True, indent=4, separators=(',', ': '))

    # with open(config_file, 'r+') as f:
    #     j = json.load(f)