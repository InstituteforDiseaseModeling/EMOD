#!/usr/bin/python

"""
This module converts a config.json file from HIV-Ongoing (1/1/2022) to a version
that Malaria-Ongoing can read (3/30/2022).  It might be able to handle older
versions of HIV-Ongoing but I'm not sure how much older.
"""

import os, sys, json, argparse

#These are the parameters that are set in SimulationConfig.cpp for HIV_SIM
HIV_SIM_fixed_parameter_values = {
    "Enable_Immunity": 1,
    "Enable_Immune_Decay" : 0,
    "Enable_Initial_Susceptibility_Distribution" : 0,
    "Enable_Maternal_Infection_Transmission" : 1,
    "Enable_Vital_Dynamics" : 1,
}

#These parameters are desired to be in the config but are not in the schema.
non_schema_parameters = [
    "Geography",
    "Num_Cores"
]

#These parameters changed names from 2.18 to 2.20
parameters_to_rename = {
    "Base_Population_Scale_Factor":  "x_Base_Population",   
    "Enable_Abort_Zero_Infectivity" : "Enable_Termination_On_Zero_Total_Infectivity"  
}

#These are new parameters in Malaria-Ongoing
parameters_new = {
    "Inset_Chart_Has_Interventions" : [],
    "Serialized_Population_Writing_Type" : "NONE",
    "Serialized_Population_Reading_Type" : "NONE"
}

#It is hard to believe but this is the only distribution in the config that needs to be updated.
distribution_parameters_to_update = [
    "Incubation_Period_Distribution"
]

#In the newer code, Linear and Piecewise are not supported
duration_to_distribution = {
    "FIXED_DURATION"          : "CONSTANT_DISTRIBUTION",
    "UNIFORM_DURATION"        : "UNIFORM_DISTRIBUTION",
    "GAUSSIAN_DURATION"       : "GAUSSIAN_DISTRIBUTION",
    "EXPONENTIAL_DURATION"    : "EXPONENTIAL_DISTRIBUTION",
    "POISSON_DURATION"        : "POISSON_DISTRIBUTION",
    "LOG_NORMAL_DURATION"     : "LOG_NORMAL_DISTRIBUTION",
    "WEIBULL_DURATION"        : "WEIBULL_DISTRIBUTION",
    "BIMODAL_DURATION"        : "DUAL_CONSTANT_DISTRIBUTION",
    "DUAL_TIMESCALE_DURATION" : "DUAL_EXPONENTIAL_DISTRIBUTION"
}

#A dictionary that explains how the parameters associated with this distribution
#should be renamed.
Incubation_Period_Distribution = {
    "FIXED_DURATION": {
        "Base_Incubation_Period" : "Incubation_Period_Constant"
    },
    "UNIFORM_DURATION" : {
        "Incubation_Period_Min" : "Incubation_Period_Min",
        "Incubation_Period_Max" : "Incubation_Period_Max"
    },
    "GAUSSIAN_DURATION" : {
        "Incubation_Period_Mean" : "Incubation_Period_Gaussian_Mean",
        "Incubation_Period_Std_Dev" : "Incubation_Period_Gaussian_Std_Dev"
    },
    "EXPONENTIAL_DURATION": {
        "Base_Incubation_Period" : "Incubation_Period_Exponential"
    },
    "LOG_NORMAL_DURATION" : {
        "Incubation_Period_Log_Mean" : "Incubation_Period_Log_Normal_Mu",
        "Incubation_Period_Log_Width" : "Incubation_Period_Log_Normal_Sigma"
    }    
}

#Assumed existing file with the schema a disease built executable on
#the Malaria-Ongoing branch.
MO_schema_filename = "schema_MO_for_HIV.json"


def GetPossibleConfigParameters( schema_filename ):
    '''
    Return the list of parameters that can be defined in config.json.
    This flattens out the parameters and gets rid of the classes like IndividualHumanHIV.
    '''
    schema_json = {}
    with open( schema_filename ) as schema_file:
        schema_json = json.loads( schema_file.read() )
    schema_config = schema_json["config"]
    
    possible_parameters = {}
    for key in schema_config.keys():
        possible_parameters.update( schema_config[ key ] )
        
    return possible_parameters
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('old_config',      default=None, nargs='?', help='config.json file to convert')
    parser.add_argument('new_config_name', default=None, nargs='?', help='name of new converted config file')
    
    args = parser.parse_args()
    
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit()

    new_config_json = {}
    new_config_json[ "parameters" ] = {}
    
    old_config_json = {}
    with open( args.old_config ) as old_config_file:
        old_config_json = json.loads( old_config_file.read() )

    #move parameters_to_rename
    for old_key, new_key in parameters_to_rename.items():
        new_config_json[ "parameters" ][ new_key ] = old_config_json[ "parameters" ][ old_key ]
        del old_config_json[ "parameters" ][ old_key ]
    
    #move parameters that are not in the schema
    for key in non_schema_parameters:
        new_config_json[ "parameters" ][ key ] = old_config_json[ "parameters" ][ key ]
        del old_config_json[ "parameters" ][ key ]
    
    #add parameters_new
    for new_key, default_value in parameters_new.items():
        new_config_json[ "parameters" ][ new_key ] = default_value
        
    #move & convert distributions
    for dist_key in distribution_parameters_to_update:
        old_value = old_config_json[ "parameters" ][ dist_key ]
        new_value = duration_to_distribution[ old_value ]
        new_config_json[ "parameters" ][ dist_key ] = new_value
        del old_config_json[ "parameters" ][ dist_key ]
        
        if dist_key == "Incubation_Period_Distribution":
            for old_key, new_key in Incubation_Period_Distribution[ old_value ].items():
                new_config_json[ "parameters" ][ new_key ] = old_config_json[ "parameters" ][ old_key ]
                del old_config_json[ "parameters" ][ old_key ]
                
    
    #Force setting of parameters that have fixed values due to simulation type
    for key, value in HIV_SIM_fixed_parameter_values.items():
        old_config_json[ "parameters" ][ key ] = value
    
    #find the possible parameters for the config file from the schema
    possible_parameters = GetPossibleConfigParameters( MO_schema_filename )
    #with open( "flattened_config_params.json", "w" ) as handle:
    #    handle.write( json.dumps( possible_parameters, indent=4, sort_keys=True ) )
    
    #move parameters still found in MO_schema_filename
    keys_to_move = []
    for key, value in old_config_json["parameters"].items():
        move_entry = False
        if key in  possible_parameters.keys():
            schema_for_param = possible_parameters[ key ]
            if "depends-on" in schema_for_param.keys():
                for other_param, other_param_possible_values in schema_for_param[ "depends-on" ].items():
                    other_param_value = old_config_json[ "parameters" ][ other_param ]
                    if type(other_param_possible_values) is int:
                        if other_param_value == other_param_possible_values:
                            move_entry = True
                    elif type(other_param_possible_values) is str:
                        other_param_possible_values_list = other_param_possible_values.split(",")
                        if other_param_value in other_param_possible_values_list:
                            move_entry = True
                    else:
                        raise "unknown parameter type: " + other_param_possible_values
                            
            else:
                move_entry = True
        elif key.startswith( "logLevel" ):
            move_entry = True
        if move_entry:
            keys_to_move.append( key )

    #move the parameters from the old file to the new one that are not
    #the ones fixed by the simulation type
    for key in keys_to_move:
        if key not in HIV_SIM_fixed_parameter_values.keys():
            new_config_json[ "parameters" ][ key ] = old_config_json[ "parameters" ][ key ]
            del old_config_json[ "parameters" ][ key ]
            
    #save new config
    with open( args.new_config_name, "w" ) as handle:
        handle.write( json.dumps( new_config_json, indent=4, sort_keys=True ) )
        
    #save old config with remaining parameters
    with open( "remaining_old_config.json", "w" ) as handle:
        handle.write( json.dumps( old_config_json, indent=4, sort_keys=True ) )
    
    print("Done converting")