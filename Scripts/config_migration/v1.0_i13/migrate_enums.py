#!/usr/bin/python

import regression_utils as ru
import sys
import json

import re

def underscore_to_camelcase(value):
    first = True
    res = []

    for u,w in re.findall('([_]*)([^_]*)',value):
        #if first:
        #    res.append(u+w)
        #    first = False
        if len(w)==0:    # trailing underscores
            res.append(u)
        else:   # trim an underscore and capitalize
            res.append(u[:-1] + w.title())

    return ''.join(res)

enums_list = [
"arabiensis_habitat_type",
"CLIMATE_STRUCTURE",
"eggHatchDelayDist",
"eggSaturation",
"evolution_polio_clock_type",
"funestus_habitat_type",
"gambiae_habitat_type",
"INCUBATION_DISTRIBUTION",
"IND_SAMPLING_TYPE",
"INFECTIOUS_DISTRIBUTION",
"JobPriority",
"larvalDensityDependence",
"LOAD_BALANCING_SCHEME",
"MALARIA_MODEL",
"MALARIA_STRAINS",
"MIGRATION_PATTERN",
"MIGRATION_STRUCTURE",
"MORTALITY_TIME_COURSE",
"PARASITE_SWITCH_TYPE",
"PKPD_MODEL",
"PoolTransmissionMode",
"POPULATION_DENSITY_INFECTIVITY_CORRECTION",
"POPULATION_SCALING",
"RANDOM_TYPE",
"REPORT_FORMAT",
"report_log_type_polio",
"SEASONAL_INFECTIVITY",
"Sim_Type",
"VDPV_virulence_model_type",
"VECTOR_SAMPLING_TYPE",
"vectorSugarFeeding",
"VisitingPoolSelectionMode",
"VITAL_BIRTH_DEPENDENCE",
]

enum_map = json.loads( open( "sim_enums.json" ).read() )

enum_map_lower = dict();
for k,v in enum_map.iteritems():
    new_k = k.lower();
    enum_map_lower[new_k] = v

#print enum_map_lower

configjson_file = open( sys.argv[1] )
def my_recursive_json_leaf_reader( ref_jsonblob, flat_json ):
    for val in ref_jsonblob:
        #if not leaf, call recursive_json_leaf_reader
        if json.dumps( ref_jsonblob[val] ).startswith( "{" ):
            my_recursive_json_leaf_reader( ref_jsonblob[val], flat_json )
        else:
            #flat_json[val] = ref_jsonblob[val]
            # if val in enum_list, change value of val to lookup result
            if val in enums_list:
                new_val = val
                if '_' in val:
                    new_val = underscore_to_camelcase(val)

                if new_val.lower() not in enum_map_lower:
                    print( "FIX BY HAND: Failed on " + val + " or " + new_val + ", not in enum_map." )
                    continue
                if str(ref_jsonblob[val]).isdigit():
                    ref_jsonblob[val] = enum_map_lower[new_val.lower()][str(ref_jsonblob[val])]

configjson_flat = json.loads( "{}" )
refjson = json.loads(configjson_file.read())
my_recursive_json_leaf_reader( refjson, configjson_flat )
print( json.dumps( refjson, sort_keys = True, indent =4 ) )
