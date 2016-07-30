#!/usr/bin/python

import json
import collections
import os
import tempfile
import string
from hashlib import md5

# below is list of 'global' variables that are shared across >1 modules in the regression suite of code. 
# probably all of them could ultimately be made at least a static member of MyRegressionRunner or Monitor.

cache_cwd = os.getcwd()
final_warnings = ""
reg_threads = []
version_string = None

def recursive_json_overrider( ref_json, flat_input_json ):
    for val in ref_json:
        #if not leaf, call recursive_json_leaf_reader
        if json.dumps( ref_json[val] ).startswith( "{" ) and val != "Vector_Species_Params" and val != "Malaria_Drug_Params" and val != "TB_Drug_Params" and val != "HIV_Drug_Params" and val != "STI_Network_Params_By_Property":
            recursive_json_overrider( ref_json[val], flat_input_json )
        # do VSP and MDP as special case. Sigh sigh sigh
        elif val == "Vector_Species_Params" or val == "Malaria_Drug_Params" or val == "TB_Drug_Params" or val == "HIV_Drug_Params" or val == "STI_Network_Params_By_Property":
            # could "genericize" this if we need to... happens to work for now, since both VSP and MDP are 3-levels deep...
            if val not in flat_input_json:
                flat_input_json[val] = { }
            elif val == "STI_Network_Params_By_Property" and not("NONE" in flat_input_json[val].keys()):
                continue

            #if val == "STI_Network_Params_By_Property" and "NONE" in ref_json[val].keys() and len( flat_input_json[val].keys() ) > 0:
            #    ref_json[val].pop( "NONE" )

            for species in ref_json[val]:
                if species not in flat_input_json[val]:
                    if( isinstance( ref_json[val][species], dict ) ):
                        flat_input_json[val][species] = { }
                for param in ref_json[val][species]:
                    if( isinstance( ref_json[val][species], dict ) ):
                        if param not in flat_input_json[val][species]:
                            flat_input_json[val][species][param] = ref_json[val][species][param]
                    else:
                        flat_input_json[val][species] = ref_json[val][species]
        else:
            if val not in flat_input_json:
                flat_input_json[val] = ref_json[val]


def flattenConfig( configjson_path ):
    if os.path.exists( configjson_path ) == False:
        print( "Path " + configjson_path + " supposedly doesn't exist!!!" )
        return None

    configjson_flat = {}
    #print( "configjson_path = " + configjson_path )
    with open( configjson_path) as handle:
        configjson = json.load( handle )

    recursive_json_overrider( configjson, configjson_flat )

    # get defaults from config.json and synthesize output from default and overrides
    if "Default_Config_Path" in configjson_flat:
        default_config_path = configjson_flat["Default_Config_Path"]

        with open( os.path.join( ".", default_config_path ) ) as handle:
            default_config_json = json.load( handle )

        recursive_json_overrider( default_config_json, configjson_flat )
    else:
        print( "Didn't find 'Default_Config_Path' in '{0}'".format( configjson_path ) )
        raise Exception( "Bad Default_Config_Path!!!" )

    # still need that parameter top level node
    configjson = {}
    configjson["parameters"] = configjson_flat

    # don't need backslashes in the default config path 
    # messing with anything else downstream now that it is flattened
    if "Default_Config_Path" in configjson["parameters"]:
        configjson["parameters"].pop("Default_Config_Path")

    # any last minute modifications
    # print( "Running with serialization OFF!" )
    # configjson["parameters"]["Serialization_Test_Cycles"] = 0

    # let's write out a flat version in case someone wants
    # to use regression examples as configs for debug mode
    with open( configjson_path.replace( "param_overrides", "config" ), 'w' ) as handle:
        json.dump(configjson, handle, sort_keys=True, indent=4)
    
    return configjson


def md5_hash(handle):
    handle.seek(0)
    md5calc = md5()
    while True:
        data = handle.read( 10240 ) # reasonable value from examples
        if len(data) == 0:
            break;
        md5calc.update(data)
    hash = md5calc.hexdigest()
    return hash
        
def md5_hash_of_file( filename ):
    #print( "Getting md5 for " + filename )
    file_handle = open( filename )
    hash = md5_hash( file_handle )
    #md5calc = md5()
    #while True:
        #file_bytes = file_handle.read( 10240 ) # value picked from example!
        #if len(file_bytes) == 0:
            #break
        #md5calc.update( file_bytes )
    file_handle.close()
    #hash = md5calc.hexdigest()
    return hash



def areTheseJsonFilesTheSame( file1, file2, key = None ):
    #print( "Comparing " + file1 + " and " + file2 )
    
    def get_json_data( filename, key ):
        with open( filename ) as handle:
            json_data = json.load( handle )
            if key is not None:
                #print( json_data.keys() )
                if key not in json_data.keys():
                    print( "ERROR: Requested key \"" + key + "\" not in json data. Found: " + str( json_data.keys() ) )
                    return None
                json_data = json_data[key]
                
        return json_data
        
    def get_data_hash( data ):
        with tempfile.TemporaryFile() as handle:
            json.dump( data, handle )
            hash = md5_hash( handle )
        
        return hash

    if os.path.exists( file1 ) == False:
        return False

    ref_data = get_json_data( file1, key )
    if ref_data == None:
        return False
    ref_hash = get_data_hash( ref_data )

    #print( "md5 of reference insetchart.json = " + str(ref_hash) )

    test_data = get_json_data( file2, key )
    test_hash = get_data_hash( test_data )
    
    #print( "md5 of test insetchart.json = " + str(test_hash) )
    
    return False if test_hash != ref_hash else True

def file_len(fname):
    with open(fname) as f:
        for i, l in enumerate(f):
            pass
    return i + 1


def make_event_map(json_in):
    event_map = {}
    for event in json_in["Events"]:
        try:
            event_map[event['Event_Name']] = (hash( json.dumps(event, sort_keys=True) ), event)
        except KeyError, e:
            print event
            raise Exception( 'The event printed above is missing the required %s key' % str(e) )
    return event_map

def strToEventList( str_in ):
    events_idx = string.find(str_in, 'Events')  # -1 is not found
    start_idx = string.find(str_in, '[', events_idx)  # -1 is not found

    psn = start_idx+1
    square_count = 1
    curly_count = 0
    events_str_array = []
    while square_count > 0 and psn <= len(str_in):
        cur = str_in[psn]

        #if args.verbose:
        #    print (psn, cur, square_count, curly_count, len(events_str_array))

        if cur == '[':
            square_count += 1
        elif cur == ']':
            square_count -= 1
        elif cur == '{':
            if curly_count == 0:
                begin_psn = psn
            curly_count += 1
        elif cur == '}':
            if curly_count == 1:
                events_str_array.append( str_in[begin_psn:psn+1] )
            curly_count -= 1
        psn += 1

    return events_str_array


def flattenCampaign(override_fn, verbose):
    #print "Looking for " + override_fn + " relative to " + os.getcwd()
    if os.path.exists( override_fn ) == False:
        #print( "Path \'" + override_fn + "\' supposedly doesn't exist!!! Looking relative to " + os.getcwd() )
        return None

    override_str = open( override_fn ).read()
    override_json = json.loads( override_str )
    override_event_map = make_event_map(override_json)
    override_event_str_list = strToEventList( override_str )

    try:
        base_fn = override_json["Default_Campaign_Path"]
    except KeyError, e:
        raise Exception( 'The override campaign file %s is missing a required key: %s' % (sys.argv[1], str(e)) )

    base_str = open( base_fn ).read()
    base_json = json.loads( base_str )
    base_event_map = make_event_map(base_json)
    base_event_str_list = strToEventList( base_str )

    merged_json = {}

    for key, value in base_json.items():
        if key == "Events":
            # Skip events
            if verbose:
                print "Skipping key: %s" % key
            continue
        elif key in override_json:
            if verbose:
                print "Key from override: %s" % key
            # Use modified value, if any
            merged_json[key] = override_json[key]
        else:
            if verbose:
                print "Key from base: %s" % key
            # Use base
            merged_json[key] = value

    merged_json["Default_Campaign_Path"] = override_json["Default_Campaign_Path"]
    merged_str = json.dumps(merged_json, indent=4, sort_keys=True )
    merged_events_str = ""

    override_event_name_to_string_map = {}
    for override_event_str in override_event_str_list:
        override_event_json = json.loads(override_event_str)
        event_name = override_event_json["Event_Name"]
        override_event_name_to_string_map[event_name] = override_event_str

    for override_event_str in override_event_str_list:
        override_event_json = json.loads(override_event_str)
        event_name = override_event_json["Event_Name"]

        if event_name not in base_event_map:
            if verbose:
                print "USE OVERLAY (EXCLUSIVE): %s" % event_name
            merged_events_str += "        %s,\n"%override_event_str


    for base_event_str in base_event_str_list:
        base_event_json = json.loads(base_event_str)
        event_name = base_event_json["Event_Name"]

        base_hash = base_event_map[event_name][0]

        if event_name in override_event_map:
            if verbose :
                print "USE OVERLAY: %s" % event_name
            override_event_str = override_event_name_to_string_map[event_name]
            merged_events_str += "        %s,\n"%override_event_str
        else:
            if verbose :
                print "USE BASE: %s" % event_name
            merged_events_str += "        %s,\n"%base_event_str

    merged_events_str = merged_events_str[:-2]  # Chop final ",\n"


    insert_psn = merged_str.rfind('}')
    insert_psn = merged_str.rfind('\n', insert_psn)
    merged_str = merged_str[0:insert_psn-1] \
               + ',\n' \
               + '    "Events":\n' \
               + '    [\n' \
               + merged_events_str + "\n" \
               + '    ]\n' \
               + '}'


    saveto_fn = override_fn.replace( "campaign_overrides", "campaign" )
    with open( saveto_fn, 'w' ) as handle:
        if verbose:
            print "Writing flattened campaign to %s" % saveto_fn
        handle.write( merged_str )


    return json.loads(merged_str)
