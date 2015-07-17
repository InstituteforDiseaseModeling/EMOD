import subprocess
import json
import os

# specify paths
binary_path = "C:\\Users\\ewenger\\LOCAL\\SVN\\Polio-20121223\\Eradication\\x64\\Release\\Eradication.exe"
input_path  = "C:\\Users\\ewenger\\LOCAL\\SVN\\Data_Files\\Netherlands"

config_name   = "Netherlands_config.json"
campaign_name = "campaign.json"

working_directory = "test"

# modify config values
configjson_file = open( config_name )
configjson = json.loads( configjson_file.read() )

configjson["parameters"]["Campaign_Filename"] = campaign_name
configjson["parameters"]["Simulation_Duration"] = 365              # just one year for now
configjson["parameters"]["Base_Population_Scale_Factor"] = 0.001   # scale the 15M down to a manageable 15k
configjson["parameters"]["Base_Individual_Scale_Factor"] = 1       # ...and count every one

configjson["parameters"]["Demographics_Filename"] = "Netherlands_single_node_demographics.compiled.json" # the birth-rate in here corresponds to the per-population rate
configjson["parameters"]["Birth_Rate_Dependence"] = "POPULATION_DEP_RATE"

configjson["parameters"]["Enable_Immunity_Initialization_Distribution"] = 0 # going to use some IPV at the beginning of the sim to get everyone caught up
configjson["parameters"]["Enable_Nondisease_Mortality"] = 1                 # why not?

# debug-level output by module
configjson["parameters"]["logLevel_Node"] = "DEBUG" 
#configjson["parameters"]["logLevel_ContagionPopulation"] = "DEBUG"
#configjson["parameters"]["logLevel_IndividualPolio"] = "DEBUG"
#configjson["parameters"]["logLevel_Individual"] = "DEBUG"

# polio disease parameters
configjson["parameters"]["Acquire_Rate_Default_Polio"] = 1e-6 # why?  1e-6 is default?
#configjson["parameters"]["Specific_Infectivity_WPV3"] = 0.4   # default
#configjson["parameters"]["Evolution_Polio_WPV_Linear_Rate"] = 0.021917809999999999 # this is how the substrain changes from 0 to 16 (random bit flip)

# modify campaign values
campaignjson = json.loads( open( campaign_name ).read() )
start_time = configjson["parameters"]["Start_Time"]
for event in campaignjson["Events"]:
    event["Start_Day"] += start_time # shift campaign start days if our sim doesn't begin at t=0

campaignjson["Events"][0]["Event_Coordinator_Config"]["Demographic_Coverage"] = 1    # IPV coverage (before scaling with pool-weighted individual accessibility)
campaignjson["Events"][1]["Event_Coordinator_Config"]["Number_Distributions"] = 100  # Number of initial outbreak cases (careful of individual sampling)

# write the new configuration files
if not os.path.exists(working_directory):
    os.makedirs(working_directory)

modified_configjson_file = open( os.path.join( working_directory, "config.json"), "w" )
modified_configjson_file.write( json.dumps( configjson, sort_keys=True, indent=4 ) )
modified_configjson_file.close()

modified_campaignjson_file = open( os.path.join( working_directory, "campaign.json"), "w" )
modified_campaignjson_file.write( json.dumps( campaignjson, sort_keys=True, indent=4 ) )
modified_campaignjson_file.close()

# commission job
os.chdir(working_directory)
#logfile = open("stdout.txt", "w")
#p = subprocess.Popen( [ binary_path, "-C", "config.json", "--input", input_path, "--output", "output" ], stdout=logfile )
subprocess.call( [ binary_path, "-C", "config.json", "--input", input_path, "--output", "output" ] )