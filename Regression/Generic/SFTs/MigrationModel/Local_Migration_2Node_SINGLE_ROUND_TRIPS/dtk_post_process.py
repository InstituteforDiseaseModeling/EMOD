#!/usr/bin/python
"""
SFTs for migration model, this includes test for follows

Migration Mode: LOCAL | REGIONAL | AIR | SEA 

[Migration Mode]_Migration_Filename
[Migration Mode]_Migration_Roundtrip_Duration
[Migration Mode]_Migration_Roundtrip_Probability

Migration_Model: NO_MIGRATION | FIXED_RATE_MIGRATION

Migration_Pattern: RANDOM_WALK_DIFFUSION | SINGLE_ROUND_TRIPS | WAYPOINTS_HOME

Please refer to https://institutefordiseasemodeling.github.io/Documentation/general/parameter-configuration.html#migration for detailed explanation of relevant parameters. 
 
The test methodology is trying to match time to next migration for each agent with a hypothetical exponential distribution with a mean of 1/lambda, where lambda is the individual migration rate given in the migration bin file (see https://institutefordiseasemodeling.github.io/Documentation/general/file-migration.html).

RANDOM_WALK:
- transform df_human_migration to [Time From_Node To_Node, MigrationType] as df_migration_type
	for each Migration type m
		for each time step t
			compare [ count of [df_migration_type] from each Node [i,j] pair (migration from node i->j at time t) / sum of [df_node_demog].NumIndividual(node i population at time t) ] ~= rate of migration for [Node i,j]
- validate each individual's migration internval in df_human_migration match exponential distribution of lambda = migration rate, by:

FOR WAYPOINTS_HOME model:
Check that all agent's travelling pattern should be in the pattern of A->X1->X2->...->Xk-> ..X2->X1->A,, where k is config.Roundtrip_Waypoints;

For SINGLE_ROUND_TRIPS model:
it's just a special case of WAYPOINTS_HOME but with config.Roundtrip_Waypoints = 1 (except that there's no duration limit/return probably as set by *_Roundtrip_Duration/*_Roundtrip_Probability), so we could reuse the above simulation and set config.Roundtrip_Waypoints = 1 accordingly;


if the SFT passed, the output file (normally scientific_feature_report.txt) will show params for run as well as "Success=True", as follows:
	Simulation parameters: simulation_duration=100, migration_pattern=RANDOM_WALK_DIFFUSION, migration_model=FIXED_RATE_MIGRATION, demog_filenames=[u'1x2_demographics_migration_heterogeneity.json']
	:
    SUMMARY: Success=True

if the SFT failed, the output file (normally scientific_feature_report.txt) will show params for run, "Success=False", as well as which test(s) failed, as follows:
	Simulation parameters: simulation_duration=100, migration_pattern=RANDOM_WALK_DIFFUSION, migration_model=FIXED_RATE_MIGRATION, demog_filenames=[u'1x2_demographics_migration_heterogeneity.json']    
	Working on Local migration...
	Processing Source node 1 to Target node 2 .....
		Migration rate read = 0.01
		Migration probability calculated = 0.00995016625083
			validating for time 1
			node population at time 1: 100008, num individuals migration out of node(1) at time 1: 1003,  their division = 0.0100291976642 
			:
			validating for time 53
			node population at time 53: 99903, num individuals migration out of node(1) at time 53: 893,  their division = 0.0089386705104 
			!!! Bad migration probability calculated: 0.0089386705104,  expected probability = 0.00995016625083, difference = 0.00101149574044
			:
	SUMMARY: Success=False

"""

import dtk_test.dtk_MigrationModel_Support as MM_Support
import dtk_test.dtk_sft as sft
import os


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                insetchart_name="InsetChart.json",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("insetchart_name: " + insetchart_name + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")
    print("current dir = " + os.getcwd())
    sft.wait_for_done()
    param_obj, config_json = MM_Support.load_emod_parameters(config_filename)
    if debug:
        print (param_obj)
    node_demo_df, human_migration_df = MM_Support.parse_output_files("output/ReportNodeDemographics.csv",
                                                                     "output/ReportHumanMigrationTracking.csv", debug)
    # for local testing
    # node_demo_df, human_migration_df = MM_Support.parse_output_files("ReportNodeDemographics.csv", "ReportHumanMigrationTracking.csv", debug)

    MM_Support.create_report_file(param_obj, node_demo_df, human_migration_df, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    application("output")
