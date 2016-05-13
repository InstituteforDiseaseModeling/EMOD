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
    print("looking up " + paramname)
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
                        new_element[lp] = TranslateElement(item[1])
                        
                        if lp == "Climate_Model" and new_element[lp] <> "CLIMATE_OFF":
                            new_element["Enable_Climate_Stochasticity"] = 1
    else:
        new_element = element

    return new_element

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

    newest_configjson = TranslateElement(configjson)

    with open(sys.argv[1], 'w') as file:
    # with open(sys.argv[1].replace('.json', '.new.json'), 'w') as file:
        json.dump(newest_configjson, file, indent=5, sort_keys=True)
