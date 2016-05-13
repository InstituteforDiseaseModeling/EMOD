import sys, os, json, collections, re

global only_translate_leaves

def ShowUsage():
    print ('\nUsage: %s <config/campaign-file> <map-json-file> [--translateall]' % os.path.basename(sys.argv[0]))
    print ('\n Use --translateall to include non-leaf keys in translation\n (i.e. for campaign files)')

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
                    if lp == "Enable_Spatial_Output":
                        new_element[lp] = (0 if item[1] == "FLOATING_POINT_TABLE" else 0)

                    elif lp == "Age_Initialization_Distribution_Type":
                        if item[0] == "DEMOGRAPHICS_AGE":
                            if lp not in new_element and item[1] == 1:
                                new_element[lp] = "DISTRIBUTION_SIMPLE"
                        elif item[0] == "EnableAgeInitializationDistribution":
                            if item[1] == 1:
                                new_element[lp] = "DISTRIBUTION_COMPLEX"

                    else:
                        new_element[lp] = TranslateElement(item[1])
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

    with open(sys.argv[2], 'r') as file:
        global lookupparams
        lookupparams = json.load(file)

    new_configjson = ConvertTabData(configjson)
    newest_configjson = TranslateElement(new_configjson)

    with open(sys.argv[1], 'w') as file:
    # with open(sys.argv[1].replace('.json', '.new.json'), 'w') as file:
        json.dump(newest_configjson, file, indent=5, sort_keys=True)
