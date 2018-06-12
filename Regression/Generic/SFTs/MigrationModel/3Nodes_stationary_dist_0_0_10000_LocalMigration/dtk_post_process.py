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

Do the validation for stationary distribution as follows and create the actual reports:
    For a inter-connected 3 Node settings without any transient nodes (i.e. for any Node pair (i,j), there exist a way to
    get from i to j at any simulation time in a finite time steps), it's equivalent to a continuous time Markov Chain model,
    and therefore we should be able to pre-calculated a stationary distribution which it will eventually converged to;
    the validation then is to run the simulation with the said node topology settings and make sure it converge to the
    pre-calculated distribution;

    Also see: https://github.com/InstituteforDiseaseModeling/TestTeam/blob/master/MigrationTest/markov-%20chain-%20stationary-%20distribution.pdf
    base from: https://www.stat.berkeley.edu/~mgoldman/Section0220.pdf


if the SFT passed, the output file (normally scientific_feature_report.txt) will show params for run as well as "Success=True", as follows:
	Simulation parameters: simulation_duration=1000, migration_pattern=RANDOM_WALK_DIFFUSION, migration_model=FIXED_RATE_MIGRATION, demog_filenames=
	:
	Extracting data from Local migration...
		Migration bin read: {1: {2: 0.1, 3: 0.1}, 2: {1: 0.2, 3: 0.1}, 3: {1: 0.3, 2: 0.1}}
		Processing node 1.....
		actual population:5302, expected population:5322.03026582
		Processing node 2.....
		actual population:2546, expected population:2560.92608461
		Processing node 3.....
		actual population:2152, expected population:2117.04364958
	SUMMARY: Success=True

if the SFT failed, the output file (normally scientific_feature_report.txt) will show params for run, "Success=False", as well as which test(s) failed, as follows:
	Simulation parameters: simulation_duration=1000, migration_pattern=RANDOM_WALK_DIFFUSION, migration_model=FIXED_RATE_MIGRATION, demog_filenames=[u'1x3_demographics_migration_heterogeneity.json']
	:
	Extracting data from Local migration...
		Migration bin read: {1: {2: 0.1, 3: 0.1}, 2: {1: 0.2, 3: 0.1}, 3: {1: 0.3, 2: 0.1}}
		Processing node 1.....
		Processing node 2.....
		Processing node 3.....
			!!! Bad actual population 2152, calculated at node 3, expected 2117.04364958
	SUMMARY: Success=False
"""

import dtk_MigrationModel_Support as MM_Support
import dtk_sft
import os


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                insetchart_name="InsetChart.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("insetchart_name: " + insetchart_name + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")
        print("current dir = " + os.getcwd())
    dtk_sft.wait_for_done()
    param_obj, config_json = MM_Support.load_emod_parameters(config_filename)
    if debug:
        print (param_obj)
    node_demog_df, human_migration_df = MM_Support.parse_output_files("output/ReportNodeDemographics.csv",
                                                                     "output/ReportHumanMigrationTracking.csv", debug)
    # for local testing
    # node_demo_df, human_migration_df = MM_Support.parse_output_files("ReportNodeDemographics.csv", "ReportHumanMigrationTracking.csv", debug)

    MM_Support.create_report_file_stationary_distribution(param_obj, node_demog_df, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    application("output")
