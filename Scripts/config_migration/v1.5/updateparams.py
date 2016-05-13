import sys, os, json, collections, re

global only_translate_leaves

def ShowUsage():
    print ('\nUsage: %s <config/campaign-file> <map-json-file> [--translateall]' % os.path.basename(sys.argv[0]))
    print ('\nNote: use --translateall for campaign files which include non-leaf keys in translation.\n')

def OrderedJsonLoad(in_dict):
    out_dict = collections.OrderedDict([])
    for pair in in_dict:
        out_dict[pair[0]] = pair[1]
    
    return out_dict

def LookupParam(paramname):
    # print("looking up " + paramname)
    global lookupparams
    if paramname in lookupparams:
        return lookupparams[paramname]

    # print("doesn't contain " + paramname)
    return paramname

def TranslateElement(element):
    # print("translating " + str(element))

    if type(element) is list:
        new_element = []
        for item in element:
            new_element.append(TranslateElement(item))
    elif type(element) is collections.OrderedDict:
        new_element = collections.OrderedDict([])
        for item in element.items():
            if only_translate_leaves and type(item[1]) is collections.OrderedDict:
                new_element[item[0]] = TranslateElement(item[1])
            else:
                lp = LookupParam(item[0])
                if lp != "":
                    if lp == "Age_Initialization_Distribution_Type":
                        if item[0] == "DEMOGRAPHICS_AGE":
                            if lp not in new_element and item[1] == 1:
                                new_element[lp] = "DISTRIBUTION_SIMPLE"
                        elif item[0] == "EnableAgeInitializationDistribution":
                            if item[1] == 1:
                                new_element[lp] = "DISTRIBUTION_COMPLEX"

                    elif lp == "Anemia_Mortality_Threshold" or lp == "Anemia_Severe_Threshold":
                        new_element[lp] = item[1] * 15
                        print("The scale of " + lp + " is changed to actual hemoglobin measurement in g/dL.")

                    elif lp == "Animal_Reservoir_Type":
                        new_element[lp] = "NO_ZOONOSIS"

                    elif lp == "Enable_Spatial_Output":
                        new_element[lp] = (1 if item[1] == "FLOATING_POINT_TABLE" else 0)
                        new_element["Spatial_Output_Channels"] = [ "Prevalence", "New_Infections" ]
                        # Could to some translating of old spatial-output params to new channel-names and add them
                        # to the array here if we wanted to, but mostly the old channels weren't used and ones that
                        # were set to true were mostly that way by default (even though spatial-output wasn't
                        # actually on.  So just remove them for now and put these 2 there as examples.

                    elif lp == "Habitat_Type" or lp == "Required_Habitat_Factor":
                        new_element[lp] = [ item[1] ]

                    elif lp == "Merozoites_Per_Hepatocyte":
                        new_element[lp] = item[1] / ( 0.25 * 11 )
                        new_element["Base_Sporozoite_Survival_Fraction"] = 0.25
                        new_element["Mean_Sporozoites_Per_Bite"] = 11
                        print(lp + " is divided by a factor corresponding to the mean number of successful sporozoites per bite, namely the product of ""Base_Sporozoite_Survival_Fraction"" and ""Mean_Sporozoites_Per_Bite"".")

                    elif lp == "Random_Type":
                        if item[1] == "USE_PSUEDO_DES":
                            new_element[lp] = "USE_PSEUDO_DES"
                        else:
                            new_element[lp] = item[1]

                    else:
                        new_element[lp] = TranslateElement(item[1])
                        
                        if lp == "Climate_Model" and new_element[lp] <> "CLIMATE_OFF":
                            new_element["Enable_Climate_Stochasticity"] = 1
                        elif lp == "Enable_Vector_Aging":
                            new_element["Enable_Temperature_Dependent_Feeding_Cycle"] = 0
                        elif lp == "Semipermanent_Habitat_Decay_Rate":
                            new_element["Rainfall_In_mm_To_Fill_Swamp"] = 1000.0
                            new_element["Vector_Larval_Rainfall_Mortality"] = "NONE"
                        elif lp == "Malaria_Model":
                            new_element["Number_Of_Asexual_Cycles_Without_Gametocytes"] = 1
                            new_element["RBC_Destruction_Multiplier"] = 1
                        elif lp == "Antibody_Capacity_Growth_Rate":
                            new_element["Antibody_CSP_Decay_Days"] = 90
                            new_element["Antibody_CSP_Killing_Inverse_Width"] = 1.5
                            new_element["Antibody_CSP_Killing_Threshold"] = 20
                        elif lp == "Vector_Sampling_Type":
                            new_element["Vector_Larval_Rainfall_Mortality"] = "NONE"
                            new_element["Wolbachia_Infection_Modification"] = 1.0
                            new_element["Wolbachia_Mortality_Modification"] = 1.0
                            new_element["HEG_Homing_Rate"] = 0.0
                            new_element["HEG_Fecundity_Limiting"] = 0.0

                # else:
                    # print("Removing parameter \"" + item[0] + "\"")
    else:
        new_element = element

    return new_element

def ConvertTabData( current ):
    if only_translate_leaves == False:
        return current

    key_grim_reaper = []

    if current["parameters"]["Sim_Type"] == "MALARIA_SIM" or current["parameters"]["Sim_Type"] == "VECTOR_SIM":
        current["parameters"]["Vector_Species_Params"] = collections.OrderedDict([])
        for key in current["parameters"]:
            for seek_key_pref in [ "arabiensis", "gambiae", "funestus", "farauti" ]:
                if key.startswith( seek_key_pref + "_" ):
                    new_key = key.replace( seek_key_pref + "_", "" )
                    if seek_key_pref not in current["parameters"]["Vector_Species_Params"]:
                        current["parameters"]["Vector_Species_Params"][seek_key_pref] = collections.OrderedDict([])
                    current["parameters"]["Vector_Species_Params"][seek_key_pref][ new_key ] = current["parameters"][key]
                    key_grim_reaper.append( key )

    for key in key_grim_reaper:
        if key in current["parameters"]:
            del current["parameters"][ key ]
        else:
            print( "For some reason " + key + " does not appear to be in the config!?!?\n") 
    del key_grim_reaper[:]

    if current["parameters"]["Sim_Type"] == "MALARIA_SIM":
        current["parameters"]["Malaria_Drug_Params"] = collections.OrderedDict([])
        for seek_key_pref in [ "AR_LM", "AR", "TBD", "PQ", "CQ", "PED", "QN", "SP", "TQ" ]:
            for key in current["parameters"]:
                if key.startswith( seek_key_pref + "_" ):
                    new_key = key.replace( seek_key_pref + "_", "" )
                    if seek_key_pref not in current["parameters"]["Malaria_Drug_Params"]:
                        current["parameters"]["Malaria_Drug_Params"][seek_key_pref] = collections.OrderedDict([])
                    current["parameters"]["Malaria_Drug_Params"][seek_key_pref][ new_key ] = current["parameters"][key]
                    key_grim_reaper.append( key )
            for key in key_grim_reaper:
                if key in current["parameters"]:
                    del current["parameters"][ key ]
                else:
                    print( "For some reason " + key + " does not appear to be in the config!?!?\n") 
            del key_grim_reaper[:]

    else:
        print( "Not MALARIA_SIM, not doing drug type conversion.\n" )

    return current

if __name__ == "__main__":
    if len(sys.argv) < 3:
        ShowUsage()
        exit(0)

    only_translate_leaves = (len(sys.argv) != 4 or sys.argv[3] != '--translateall')

    with open(sys.argv[1], 'r') as file:
        configjson = json.load(file, object_pairs_hook=OrderedJsonLoad)

    try:
        
        # if it is already V1.5, Sim_Type should not be there, instead should be Simulation_Type
        if only_translate_leaves:
            vt = configjson["parameters"]["Sim_Type"]
        else:
            vt = configjson["Events"][0]["event_coordinator_config"]

    except:
        print("The input file, " + sys.argv[1] + ", is either V1.5 ready or not the right type, so exit without any change.")
        exit(0)


    with open(sys.argv[2], 'r') as file:
        global lookupparams
        lookupparams = json.load(file)

    new_configjson = ConvertTabData(configjson)
    newest_configjson = TranslateElement(new_configjson)

    with open(sys.argv[1], 'w') as file:
    # with open(sys.argv[1].replace('.json', '.new.json'), 'w') as file:
        json.dump(newest_configjson, file, indent=5, sort_keys=True)

    print("Successfully finished migrating file, " + sys.argv[1] + ".");
